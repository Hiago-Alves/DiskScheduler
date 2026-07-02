/**
 * @file    scheduler.h
 * @brief   Infraestrutura do escalonador de disco e algoritmos de
 *          escalonamento.
 *
 * Padrão: C11
 */
 
#ifndef SCHEDULER_H
#define SCHEDULER_H
 
#include <stdint.h>
#include <stdbool.h>
#include "avl.h"
#include "disk.h"
#include "request.h"
 
/* ------------------------------------------------------------------ */
/*  Estrutura principal                                                 */
/* ------------------------------------------------------------------ */
 
typedef struct {
    AVLNode *root;
    Disk disk;
    uint32_t request_count;
    uint64_t total_served;
} Scheduler;
 
/* ------------------------------------------------------------------ */
/*  Ciclo de vida                                                       */
/* ------------------------------------------------------------------ */
 
void scheduler_init(Scheduler *sched);
 
/* ------------------------------------------------------------------ */
/*  Manipulação de requisições                                          */
/* ------------------------------------------------------------------ */
 
bool scheduler_add(Scheduler *sched, Request req);
bool scheduler_remove(Scheduler *sched, uint32_t cylinder, uint32_t id);
 
/* ------------------------------------------------------------------ */
/*  Consultas de estado                                                 */
/* ------------------------------------------------------------------ */
 
bool scheduler_is_empty(const Scheduler *sched);
AVLNode *scheduler_min(const Scheduler *sched);
AVLNode *scheduler_max(const Scheduler *sched);
 
/* ------------------------------------------------------------------ */
/*  Algoritmos de escalonamento                                         */
/* ------------------------------------------------------------------ */
 
bool scheduler_fcfs(Scheduler *sched);
bool scheduler_sstf(Scheduler *sched);
bool scheduler_scan(Scheduler *sched);
bool scheduler_cscan(Scheduler *sched);   /* agora C-LOOK */
bool scheduler_deadline(Scheduler *sched);

/**
 * @brief Aplica aging a todas as requisições pendentes.
 *
 * Percorre a AVL e atualiza o campo `priority` de cada requisição com base
 * no tempo de espera: priority += (current_time - arrival_time) / AGING_INTERVAL.
 *
 * @param sched         Ponteiro para o Scheduler.
 * @param current_time  Tempo atual da simulação.
 */
void scheduler_apply_aging(Scheduler *sched, uint64_t current_time);
 
/* ------------------------------------------------------------------ */
/*  Impressão                                                           */
/* ------------------------------------------------------------------ */
 
void scheduler_print_stats(const Scheduler *sched);
 
#endif /* SCHEDULER_H */