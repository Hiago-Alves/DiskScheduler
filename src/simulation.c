/**
 * @file    simulation.c
 * @brief   Implementacao do modulo simulation.
 *
 * Padrao: C11
 */

#include "../include/simulation.h"
#include "../include/config.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Funcoes auxiliares internas                                        */
/* ------------------------------------------------------------------ */

static uint32_t random_zipf(uint32_t max, double alpha)
{
    if (max == 0) return 0;
    double x = (double)rand() / RAND_MAX;
    double c = 0.0;
    for (uint32_t i = 1; i <= max; i++)
        c += 1.0 / pow(i, alpha);
    double cumulative = 0.0;
    for (uint32_t i = 1; i <= max; i++) {
        cumulative += (1.0 / pow(i, alpha)) / c;
        if (x <= cumulative) return i - 1;
    }
    return max - 1;
}

/* ------------------------------------------------------------------ */
/*  Callback de logging                                                */
/* ------------------------------------------------------------------ */

static void simulation_log_callback(const Scheduler *sched)
{
    if (sched == NULL || sched->log_context == NULL) return;
    Simulation *sim = (Simulation *)sched->log_context;
    if (sim->gnuplot_file == NULL) return;
    fprintf(sim->gnuplot_file, "%llu %u\n",
            (unsigned long long)sched->disk.current_time,
            sched->disk.head_position);
    fflush(sim->gnuplot_file);
}

/* ------------------------------------------------------------------ */
/*  Ciclo de vida                                                       */
/* ------------------------------------------------------------------ */

void simulation_init(Simulation *sim)
{
    if (sim == NULL) return;
    scheduler_init(&sim->scheduler);
    sim->algorithm = ALG_FCFS;
    sim->load_type = LOAD_RANDOM;
    sim->total_requests = 0;
    sim->seed = 0;
    sim->executed = false;
    sim->initial_head_position = CONFIG_INITIAL_HEAD;
    sim->total_time_us = 0;
    sim->total_seek_distance = 0;
    sim->final_head_position = 0;
    sim->total_served = 0;
    sim->gnuplot_file = NULL;

    /* Registra callback de logging */
    scheduler_set_log_callback(&sim->scheduler,
                               simulation_log_callback,
                               sim);
}

void simulation_reset(Simulation *sim)
{
    if (sim == NULL) return;

    /* Remove todas as requisicoes da AVL */
    while (sim->scheduler.request_count > 0) {
        AVLNode *min = scheduler_min(&sim->scheduler);
        if (min == NULL) break;
        scheduler_remove(&sim->scheduler, min->req.cylinder, min->req.id);
    }

    disk_init(&sim->scheduler.disk);
    sim->scheduler.disk.head_position = sim->initial_head_position;

    /* Reseta estatisticas, mas mantem configuracoes */
    sim->total_requests = 0;
    sim->executed = false;
    sim->total_time_us = 0;
    sim->total_seek_distance = 0;
    sim->final_head_position = 0;
    sim->total_served = 0;
    sim->scheduler.max_wait_time = 0;
    sim->scheduler.total_wait_time = 0;
    sim->scheduler.wait_count = 0;

    if (sim->gnuplot_file) {
        fclose(sim->gnuplot_file);
        sim->gnuplot_file = NULL;
    }
}

/* ------------------------------------------------------------------ */
/*  Configuracao                                                        */
/* ------------------------------------------------------------------ */

void simulation_set_initial_head(Simulation *sim, uint32_t head)
{
    if (sim) sim->initial_head_position = head;
}

void simulation_set_seed(Simulation *sim, uint32_t seed)
{
    if (sim) sim->seed = seed;
}

void simulation_select_algorithm(Simulation *sim, Algorithm algorithm)
{
    if (sim) sim->algorithm = algorithm;
}

void simulation_set_load_type(Simulation *sim, LoadType load)
{
    if (sim) sim->load_type = load;
}

void simulation_enable_gnuplot(Simulation *sim, bool enable)
{
    if (sim == NULL) return;
    if (enable && sim->gnuplot_file == NULL) {
        sim->gnuplot_file = fopen("head_position.dat", "w");
        if (sim->gnuplot_file)
            fprintf(sim->gnuplot_file, "# time cylinder\n");
    } else if (!enable && sim->gnuplot_file != NULL) {
        fclose(sim->gnuplot_file);
        sim->gnuplot_file = NULL;
    }
}

/* ------------------------------------------------------------------ */
/*  Geracao de requisicoes                                              */
/* ------------------------------------------------------------------ */

bool simulation_generate_requests(Simulation *sim, uint32_t count)
{
    if (sim == NULL || count == 0) return false;

    if (sim->seed == 0) srand((unsigned int)time(NULL));
    else srand((unsigned int)sim->seed);

    uint32_t inserted = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t cylinder;
        switch (sim->load_type) {
            case LOAD_SEQUENTIAL:
                cylinder = i % CONFIG_CYLINDERS;
                break;
            case LOAD_ZIPF:
                cylinder = random_zipf(CONFIG_CYLINDERS, ZIPF_ALPHA);
                break;
            default: /* LOAD_RANDOM */
                cylinder = rand() % CONFIG_CYLINDERS;
                break;
        }
        Request req = request_create(i + 1, cylinder, 1, i, OP_READ, 0, 0);
        if (!scheduler_add(&sim->scheduler, req)) return false;
        inserted++;
    }
    sim->total_requests = inserted;
    return true;
}

bool simulation_generate_fire_test(Simulation *sim,
                                   uint32_t center_count,
                                   uint32_t center_start,
                                   uint32_t center_end,
                                   uint32_t peripheral_count,
                                   uint32_t peripheral_cylinder)
{
    if (sim == NULL) return false;

    if (sim->seed == 0) srand((unsigned int)time(NULL));
    else srand((unsigned int)sim->seed);

    uint32_t total = 0;
    for (uint32_t i = 0; i < center_count; i++) {
        uint32_t cylinder = center_start + (rand() % (center_end - center_start + 1));
        Request req = request_create(i + 1, cylinder, 1, i, OP_READ, 0, 0);
        if (!scheduler_add(&sim->scheduler, req)) return false;
        total++;
    }
    for (uint32_t i = 0; i < peripheral_count; i++) {
        uint32_t id = center_count + i + 1;
        Request req = request_create(id, peripheral_cylinder, 1, id, OP_READ, 0, 0);
        if (!scheduler_add(&sim->scheduler, req)) return false;
        total++;
    }
    sim->total_requests = total;
    return true;
}

/* ------------------------------------------------------------------ */
/*  Execucao                                                            */
/* ------------------------------------------------------------------ */

bool simulation_run(Simulation *sim)
{
    if (sim == NULL) return false;
    if (scheduler_is_empty(&sim->scheduler)) return false;

    uint64_t initial_time = sim->scheduler.disk.current_time;
    uint64_t initial_distance = sim->scheduler.disk.total_seek_distance;

    bool success = false;
    switch (sim->algorithm) {
        case ALG_FCFS:  success = scheduler_fcfs(&sim->scheduler); break;
        case ALG_SSTF:  success = scheduler_sstf(&sim->scheduler); break;
        case ALG_SCAN:  success = scheduler_scan(&sim->scheduler); break;
        case ALG_CSCAN: success = scheduler_cscan(&sim->scheduler); break;
        case ALG_DEADLINE: success = scheduler_deadline(&sim->scheduler); break;
        default: return false;
    }
    if (!success) return false;

    sim->total_time_us = sim->scheduler.disk.current_time - initial_time;
    sim->total_seek_distance = sim->scheduler.disk.total_seek_distance - initial_distance;
    sim->final_head_position = sim->scheduler.disk.head_position;
    sim->total_served = sim->scheduler.total_served;
    sim->executed = true;
    return true;
}

void simulation_run_fire_test(Simulation *sim)
{
    if (sim == NULL) return;

    uint32_t center_count = 1000;
    uint32_t center_start = 500;
    uint32_t center_end = 600;
    uint32_t peripheral_count = 1;
    uint32_t peripheral_cylinder = CONFIG_CYLINDERS - 1;

    printf("\n=== TESTE DE FOGO (STARVATION x AGING) ===\n");
    printf("Gerando %u reqs no centro [%u-%u] e %u no extremo (%u)\n",
           center_count, center_start, center_end,
           peripheral_count, peripheral_cylinder);

    /* --- SSTF --- */
    simulation_reset(sim);
    simulation_select_algorithm(sim, ALG_SSTF);
    simulation_generate_fire_test(sim, center_count, center_start, center_end,
                                  peripheral_count, peripheral_cylinder);
    printf("\nExecutando SSTF...\n");
    if (!simulation_run(sim)) {
        printf("ERRO no SSTF.\n");
        return;
    }
    uint64_t sstf_dist = sim->total_seek_distance;
    uint64_t sstf_time = sim->total_time_us;
    uint64_t sstf_max_wait = sim->scheduler.max_wait_time;
    uint64_t sstf_avg_wait = sim->scheduler.wait_count > 0 ?
                             sim->scheduler.total_wait_time / sim->scheduler.wait_count : 0;

    /* --- DEADLINE --- */
    simulation_reset(sim);
    simulation_select_algorithm(sim, ALG_DEADLINE);
    simulation_generate_fire_test(sim, center_count, center_start, center_end,
                                  peripheral_count, peripheral_cylinder);
    printf("\nExecutando DEADLINE com aging...\n");
    if (!simulation_run(sim)) {
        printf("ERRO no DEADLINE.\n");
        return;
    }
    uint64_t deadline_dist = sim->total_seek_distance;
    uint64_t deadline_time = sim->total_time_us;
    uint64_t deadline_max_wait = sim->scheduler.max_wait_time;
    uint64_t deadline_avg_wait = sim->scheduler.wait_count > 0 ?
                                 sim->scheduler.total_wait_time / sim->scheduler.wait_count : 0;

    /* --- Relatorio comparativo --- */
    printf("\n=== COMPARACAO ===\n");
    printf("Metrica          | SSTF       | DEADLINE (aging)\n");
    printf("-----------------+------------+------------------\n");
    printf("Distancia        | %-10llu | %-16llu\n",
           (unsigned long long)sstf_dist, (unsigned long long)deadline_dist);
    printf("Tempo total (us) | %-10llu | %-16llu\n",
           (unsigned long long)sstf_time, (unsigned long long)deadline_time);
    printf("Espera maxima    | %-10llu | %-16llu\n",
           (unsigned long long)sstf_max_wait, (unsigned long long)deadline_max_wait);
    printf("Espera media     | %-10llu | %-16llu\n",
           (unsigned long long)sstf_avg_wait, (unsigned long long)deadline_avg_wait);

    if (sstf_max_wait > deadline_max_wait * 2) {
        printf("\n[OK] Aging funcionou: espera maxima reduzida drasticamente.\n");
    } else {
        printf("\n[Atencao] Aging nao reduziu significativamente a espera maxima.\n");
    }
    printf("\n");
}

/* ------------------------------------------------------------------ */
/*  Estatisticas                                                        */
/* ------------------------------------------------------------------ */

uint64_t simulation_get_total_time(const Simulation *sim)
{
    return sim ? sim->total_time_us : 0;
}

uint64_t simulation_get_total_distance(const Simulation *sim)
{
    return sim ? sim->total_seek_distance : 0;
}

uint32_t simulation_get_final_head(const Simulation *sim)
{
    return sim ? sim->final_head_position : 0;
}

uint64_t simulation_get_total_served(const Simulation *sim)
{
    return sim ? sim->total_served : 0;
}

void simulation_print_report(const Simulation *sim)
{
    if (sim == NULL) {
        printf("Erro: Simulation nula.\n");
        return;
    }
    const char *algo_str;
    switch (sim->algorithm) {
        case ALG_FCFS:  algo_str = "FCFS"; break;
        case ALG_SSTF:  algo_str = "SSTF"; break;
        case ALG_SCAN:  algo_str = "SCAN"; break;
        case ALG_CSCAN: algo_str = "C-LOOK"; break;
        case ALG_DEADLINE: algo_str = "DEADLINE"; break;
        default: algo_str = "Desconhecido";
    }

    printf("\n");
    printf("+-------------------------------------------------------+\n");
    printf("|              RELATORIO DA SIMULACAO                   |\n");
    printf("+-------------------------------------------------------+\n");
    printf("| Algoritmo          : %-26s |\n", algo_str);
    printf("| Posicao inicial    : cilindro %-17u |\n", sim->initial_head_position);
    printf("| Requisicoes geradas: %-26u |\n", sim->total_requests);
    printf("| Requisicoes atendidas: %-25llu |\n",
           (unsigned long long)sim->total_served);
    printf("| Distancia total    : %-26llu |\n",
           (unsigned long long)sim->total_seek_distance);
    printf("| Tempo total (us)   : %-26llu |\n",
           (unsigned long long)sim->total_time_us);
    printf("| Posicao final      : cilindro %-17u |\n",
           sim->final_head_position);
    if (sim->scheduler.wait_count > 0) {
        printf("| Espera maxima (us) : %-26llu |\n",
               (unsigned long long)sim->scheduler.max_wait_time);
        printf("| Espera media (us)  : %-26llu |\n",
               (unsigned long long)(sim->scheduler.total_wait_time / sim->scheduler.wait_count));
    }
    printf("+-------------------------------------------------------+\n");
    printf("\n");
}