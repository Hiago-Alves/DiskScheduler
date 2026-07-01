/**
 * @file    simulation.c
 * @brief   Implementacao do modulo de controle da simulacao.
 *
 * Padrao: C11
 */

#include "../include/simulation.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "config.h"

static uint32_t random_uint32(uint32_t max)
{
    return (uint32_t)(rand() % max);
}

void simulation_init(Simulation *sim)
{
    if (sim == NULL) return;
    scheduler_init(&sim->scheduler);
    sim->algorithm = ALG_FCFS;
    sim->total_requests = 0;
    sim->seed = 0;
    sim->executed = false;
    sim->total_time_us = 0;
    sim->total_seek_distance = 0;
    sim->final_head_position = 0;
    sim->total_served = 0;
}

void simulation_reset(Simulation *sim)
{
    if (sim == NULL) return;
    while (sim->scheduler.request_count > 0) {
        AVLNode *min_node = scheduler_min(&sim->scheduler);
        if (min_node == NULL) break;
        scheduler_remove(&sim->scheduler,
                         min_node->req.cylinder,
                         min_node->req.id);
    }
    disk_init(&sim->scheduler.disk);
    sim->scheduler.total_served = 0;
    sim->total_requests = 0;
    sim->executed = false;
    sim->total_time_us = 0;
    sim->total_seek_distance = 0;
    sim->final_head_position = 0;
    sim->total_served = 0;
}

bool simulation_generate_requests(Simulation *sim, uint32_t count)
{
    if (sim == NULL || count == 0) return false;

    if (sim->seed == 0) {
        srand((unsigned int)time(NULL));
    } else {
        srand((unsigned int)sim->seed);
    }

    uint32_t inserted = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t cylinder = random_uint32(CONFIG_CYLINDERS);
        Request req = request_create(i + 1, cylinder, 1, i, OP_READ, 0, 0);
        if (!scheduler_add(&sim->scheduler, req)) {
            return false;
        }
        inserted++;
    }
    sim->total_requests = inserted;
    return true;
}

void simulation_set_seed(Simulation *sim, uint32_t seed)
{
    if (sim == NULL) return;
    sim->seed = seed;
}

void simulation_select_algorithm(Simulation *sim, Algorithm algorithm)
{
    if (sim == NULL) return;
    sim->algorithm = algorithm;
}

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

uint64_t simulation_get_total_time(const Simulation *sim)
{
    return (sim == NULL) ? 0 : sim->total_time_us;
}

uint64_t simulation_get_total_distance(const Simulation *sim)
{
    return (sim == NULL) ? 0 : sim->total_seek_distance;
}

uint32_t simulation_get_final_head(const Simulation *sim)
{
    return (sim == NULL) ? 0 : sim->final_head_position;
}

uint64_t simulation_get_total_served(const Simulation *sim)
{
    return (sim == NULL) ? 0 : sim->total_served;
}

void simulation_print_report(const Simulation *sim)
{
    if (sim == NULL) {
        printf("Erro: Simulation nula.\n");
        return;
    }

    const char *algo_str = "Desconhecido";
    switch (sim->algorithm) {
        case ALG_FCFS:  algo_str = "FCFS"; break;
        case ALG_SSTF:  algo_str = "SSTF"; break;
        case ALG_SCAN:  algo_str = "SCAN"; break;
        case ALG_CSCAN: algo_str = "C-SCAN"; break;
    }

    printf("\n");
    printf("+-------------------------------------------------------+\n");
    printf("|              RELATORIO DA SIMULACAO                   |\n");
    printf("+-------------------------------------------------------+\n");
    printf("| Algoritmo          : %-26s |\n", algo_str);
    printf("| Requisicoes geradas: %-26u |\n", sim->total_requests);
    printf("| Requisicoes atendidas: %-25llu |\n",
           (unsigned long long)sim->total_served);
    printf("| Distancia total    : %-26llu |\n",
           (unsigned long long)sim->total_seek_distance);
    printf("| Tempo total (us)   : %-26llu |\n",
           (unsigned long long)sim->total_time_us);
    printf("| Posicao final      : cilindro %-17u |\n",
           sim->final_head_position);
    printf("+-------------------------------------------------------+\n");
    printf("\n");
}