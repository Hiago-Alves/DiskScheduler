/**
 * @file    simulation.h
 * @brief   Camada de controle da simulacao.
 *
 * Padrao: C11
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "scheduler.h"
#include "config.h"

/* ------------------------------------------------------------------ */
/*  Algoritmos                                                          */
/* ------------------------------------------------------------------ */

typedef enum {
    ALG_FCFS = 1,
    ALG_SSTF = 2,
    ALG_SCAN = 3,
    ALG_CSCAN = 4,   /* C-LOOK */
    ALG_DEADLINE = 5
} Algorithm;

/* ------------------------------------------------------------------ */
/*  Estrutura principal                                                 */
/* ------------------------------------------------------------------ */

typedef struct {
    Scheduler scheduler;
    Algorithm algorithm;
    LoadType load_type;
    uint32_t total_requests;
    uint32_t seed;
    bool executed;

    uint32_t initial_head_position;

    uint64_t total_time_us;
    uint64_t total_seek_distance;
    uint32_t final_head_position;
    uint64_t total_served;

    FILE *gnuplot_file;

} Simulation;

/* ------------------------------------------------------------------ */
/*  Ciclo de vida                                                       */
/* ------------------------------------------------------------------ */

void simulation_init(Simulation *sim);
void simulation_reset(Simulation *sim);

/* ------------------------------------------------------------------ */
/*  Configuracao                                                        */
/* ------------------------------------------------------------------ */

void simulation_set_initial_head(Simulation *sim, uint32_t head);
void simulation_set_seed(Simulation *sim, uint32_t seed);
void simulation_select_algorithm(Simulation *sim, Algorithm algorithm);
void simulation_set_load_type(Simulation *sim, LoadType load);
void simulation_enable_gnuplot(Simulation *sim, bool enable);

/* ------------------------------------------------------------------ */
/*  Geracao de requisicoes                                              */
/* ------------------------------------------------------------------ */

bool simulation_generate_requests(Simulation *sim, uint32_t count);
bool simulation_generate_fire_test(Simulation *sim,
                                   uint32_t center_count,
                                   uint32_t center_start,
                                   uint32_t center_end,
                                   uint32_t peripheral_count,
                                   uint32_t peripheral_cylinder);

/* ------------------------------------------------------------------ */
/*  Execucao                                                            */
/* ------------------------------------------------------------------ */

bool simulation_run(Simulation *sim);
void simulation_run_fire_test(Simulation *sim);

/* ------------------------------------------------------------------ */
/*  Estatisticas                                                        */
/* ------------------------------------------------------------------ */

uint64_t simulation_get_total_time(const Simulation *sim);
uint64_t simulation_get_total_distance(const Simulation *sim);
uint32_t simulation_get_final_head(const Simulation *sim);
uint64_t simulation_get_total_served(const Simulation *sim);

void simulation_print_report(const Simulation *sim);

#endif /* SIMULATION_H */