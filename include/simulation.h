/**
 * @file    simulation.h
 * @brief   Camada de controle da simulacao do escalonador de disco.
 *
 * Padrao: C11
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdint.h>
#include <stdbool.h>
#include "scheduler.h"

typedef enum {
    ALG_FCFS  = 1,
    ALG_SSTF  = 2,
    ALG_SCAN  = 3,
    ALG_CSCAN = 4
} Algorithm;

typedef struct {
    Scheduler scheduler;
    Algorithm algorithm;
    uint32_t total_requests;
    uint32_t seed;
    bool executed;
    uint64_t total_time_us;
    uint64_t total_seek_distance;
    uint32_t final_head_position;
    uint64_t total_served;
} Simulation;

void simulation_init(Simulation *sim);
void simulation_reset(Simulation *sim);
bool simulation_generate_requests(Simulation *sim, uint32_t count);
void simulation_set_seed(Simulation *sim, uint32_t seed);
void simulation_select_algorithm(Simulation *sim, Algorithm algorithm);
bool simulation_run(Simulation *sim);
uint64_t simulation_get_total_time(const Simulation *sim);
uint64_t simulation_get_total_distance(const Simulation *sim);
uint32_t simulation_get_final_head(const Simulation *sim);
uint64_t simulation_get_total_served(const Simulation *sim);
void simulation_print_report(const Simulation *sim);

#endif