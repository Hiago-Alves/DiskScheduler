/**
 * @file    scheduler.h
 * @brief   Infraestrutura do escalonador de disco e algoritmos.
 *
 * Padrao: C11
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include "avl.h"
#include "disk.h"
#include "request.h"

/* ------------------------------------------------------------------ */
/*  Forward declaration da estrutura principal                         */
/* ------------------------------------------------------------------ */

typedef struct Scheduler Scheduler;

/* ------------------------------------------------------------------ */
/*  Tipos auxiliares                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Funcao de callback para logging de posicao da cabeca.
 */
typedef void (*SchedulerLogCallback)(const Scheduler *sched);

/* ------------------------------------------------------------------ */
/*  Estrutura principal (definicao completa)                           */
/* ------------------------------------------------------------------ */

struct Scheduler {
    AVLNode *root;
    Disk disk;
    uint32_t request_count;
    uint64_t total_served;

    /* Estatisticas de tempo de espera (para testes) */
    uint64_t max_wait_time;
    uint64_t total_wait_time;
    uint32_t wait_count;

    /* Callback e contexto para logging */
    SchedulerLogCallback log_callback;
    void *log_context;
};

/* ------------------------------------------------------------------ */
/*  Prototipos das funcoes                                             */
/* ------------------------------------------------------------------ */

void scheduler_init(Scheduler *sched);

bool scheduler_add(Scheduler *sched, Request req);
bool scheduler_remove(Scheduler *sched, uint32_t cylinder, uint32_t id);

bool scheduler_is_empty(const Scheduler *sched);
AVLNode *scheduler_min(const Scheduler *sched);
AVLNode *scheduler_max(const Scheduler *sched);

/* Algoritmos */
bool scheduler_fcfs(Scheduler *sched);
bool scheduler_sstf(Scheduler *sched);
bool scheduler_scan(Scheduler *sched);
bool scheduler_cscan(Scheduler *sched);   /* C-LOOK */
bool scheduler_deadline(Scheduler *sched);

/* Coalescencia */
void scheduler_coalesce(Scheduler *sched);

/* Logging e callback */
void scheduler_set_log_callback(Scheduler *sched,
                                SchedulerLogCallback callback,
                                void *context);

/* Aging */
void scheduler_apply_aging(Scheduler *sched, uint64_t current_time);

/* Impressao */
void scheduler_print_stats(const Scheduler *sched);

#endif /* SCHEDULER_H */