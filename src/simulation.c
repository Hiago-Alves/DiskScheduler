/**
 * @file    simulation.c
 * @brief   Implementação da camada de controle da simulação.
 *
 * Padrão: C11
 */

#include "../include/simulation.h"

#include <stdlib.h>   /* rand, srand */
#include <stdio.h>    /* printf      */
#include <time.h>     /* time        */

/* ------------------------------------------------------------------ */
/*  Funções auxiliares internas (static)                               */
/* ------------------------------------------------------------------ */

/**
 * @brief Gera um número aleatório entre min e max (inclusive).
 *
 * @param min  Valor mínimo.
 * @param max  Valor máximo.
 * @return Número aleatório no intervalo [min, max].
 */
static uint32_t random_range(uint32_t min, uint32_t max)
{
    if (min > max) {
        /* Troca os valores para garantir min <= max */
        uint32_t temp = min;
        min = max;
        max = temp;
    }
    return min + (uint32_t)(rand() % (max - min + 1));
}

/**
 * @brief Gera um valor booleano aleatório (verdadeiro ou falso).
 *
 * @return true ou false aleatoriamente.
 */
static bool random_bool(void)
{
    return (rand() % 2) == 1;
}

/* ------------------------------------------------------------------ */
/*  Implementação das funções públicas                                  */
/* ------------------------------------------------------------------ */

void simulation_init(Simulation *sim)
{
    if (sim == NULL) {
        return;
    }

    /* Inicializa o Scheduler internamente */
    scheduler_init(&sim->scheduler);

    /* Define o algoritmo padrão como FCFS */
    sim->algorithm = ALG_FCFS;

    /* Zera todas as métricas */
    sim->total_requests       = 0;
    sim->total_time           = 0;
    sim->total_seek_distance  = 0;
    sim->final_head_position  = 0;
    sim->total_served         = 0;
}

bool simulation_generate_requests(Simulation *sim, uint32_t count, unsigned int seed)
{
    if (sim == NULL || count == 0) {
        return false;
    }

    /* Inicializa a semente do gerador de números aleatórios */
    srand(seed);

    uint64_t arrival_time = 0;

    for (uint32_t id = 1; id <= count; id++) {
        /* Gera dados aleatórios para a requisição */
        uint32_t cylinder   = random_range(0, CONFIG_CYLINDERS - 1);
        uint32_t process_id = random_range(1, 10);
        OperationType op    = random_bool() ? OP_READ : OP_WRITE;

        /* O arrival_time é incremental: cada requisição chega com um atraso
         * aleatório de 0 a 50 ticks, garantindo que as chegadas sejam
         * distribuídas no tempo. */
        arrival_time += random_range(0, 50);

        /* Cria a requisição (deadline = 0 indica sem prazo, priority = 0) */
        Request req = request_create(id, cylinder, process_id,
                                     arrival_time, op, 0, 0);

        /* Insere no Scheduler */
        if (!scheduler_add(&sim->scheduler, req)) {
            /* Falha de alocação: interrompe a geração e retorna false */
            return false;
        }
    }

    /* Armazena o número total de requisições geradas */
    sim->total_requests = count;

    return true;
}

void simulation_select_algorithm(Simulation *sim, SchedulingAlgorithm algorithm)
{
    if (sim == NULL) {
        return;
    }

    sim->algorithm = algorithm;
}

bool simulation_run(Simulation *sim)
{
    if (sim == NULL) {
        return false;
    }

    bool success = false;

    /* Executa o algoritmo selecionado */
    switch (sim->algorithm) {
        case ALG_FCFS:
            success = scheduler_fcfs(&sim->scheduler);
            break;
        case ALG_SSTF:
            success = scheduler_sstf(&sim->scheduler);
            break;
        case ALG_SCAN:
            success = scheduler_scan(&sim->scheduler);
            break;
        case ALG_CSCAN:
            success = scheduler_cscan(&sim->scheduler);
            break;
        default:
            /* Algoritmo inválido */
            return false;
    }

    /* Se a execução foi bem-sucedida, calcula as estatísticas finais */
    if (success) {
        simulation_compute_stats(sim);
    }

    return success;
}

void simulation_compute_stats(Simulation *sim)
{
    if (sim == NULL) {
        return;
    }

    /* Extrai as métricas do Scheduler e do Disk */
    Scheduler *sched = &sim->scheduler;

    sim->total_time          = sched->disk.current_time;
    sim->total_seek_distance = sched->disk.total_seek_distance;
    sim->final_head_position = sched->disk.head_position;
    sim->total_served        = sched->total_served;
}

void simulation_print_report(const Simulation *sim)
{
    if (sim == NULL) {
        printf("ERRO: Simulation nula.\n");
        return;
    }

    /* Converte o enum para string legível */
    const char *algo_str;
    switch (sim->algorithm) {
        case ALG_FCFS:  algo_str = "FCFS"; break;
        case ALG_SSTF:  algo_str = "SSTF"; break;
        case ALG_SCAN:  algo_str = "SCAN"; break;
        case ALG_CSCAN: algo_str = "C-SCAN"; break;
        default:        algo_str = "DESCONHECIDO"; break;
    }

    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("                   RELATÓRIO DA SIMULAÇÃO                     \n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Algoritmo utilizado           : %s\n", algo_str);
    printf("  Requisições geradas           : %u\n", sim->total_requests);
    printf("  Requisições atendidas         : %llu\n", (unsigned long long)sim->total_served);
    printf("  Distância total percorrida    : %llu cilindros\n", (unsigned long long)sim->total_seek_distance);
    printf("  Tempo total de execução       : %llu us\n", (unsigned long long)sim->total_time);
    printf("  Posição final da cabeça       : cilindro %u\n", sim->final_head_position);
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("\n");
}

void simulation_reset(Simulation *sim)
{
    if (sim == NULL) {
        return;
    }

    /* Reinicializa o Scheduler, o que zera o disco e a fila */
    scheduler_init(&sim->scheduler);

    /* Mantém o algoritmo selecionado (não o altera) */

    /* Zera as métricas, mantendo o total_requests (opcional) */
    sim->total_time          = 0;
    sim->total_seek_distance = 0;
    sim->final_head_position = 0;
    sim->total_served        = 0;
    /* total_requests é mantido para referência, mas pode ser zerado se desejar */
    /* sim->total_requests = 0; */ /* Deixamos como está para fins de relatório */
}