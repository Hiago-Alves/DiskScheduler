/**
 * @file    scheduler.c
 * @brief   Implementacao do escalonador e algoritmos.
 *
 * Padrao: C11
 */

#include "../include/scheduler.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>

/* Garante que as constantes estejam definidas (fallback) */
#ifndef CONFIG_DEADLINE_LIMIT
#define CONFIG_DEADLINE_LIMIT 50
#endif
#ifndef AGING_INTERVAL
#define AGING_INTERVAL 5
#endif
#ifndef AGING_THRESHOLD
#define AGING_THRESHOLD 10
#endif
#ifndef COALESCE_THRESHOLD
#define COALESCE_THRESHOLD 2
#endif

/* ------------------------------------------------------------------ */
/*  Funcoes auxiliares internas                                        */
/* ------------------------------------------------------------------ */

static AVLNode *scheduler_max_node(AVLNode *node)
{
    while (node->right != NULL) node = node->right;
    return node;
}

static void scheduler_collect_inorder(const AVLNode *node,
                                      Request *buffer,
                                      uint32_t *count)
{
    if (node == NULL) return;
    scheduler_collect_inorder(node->left, buffer, count);
    buffer[*count] = node->req;
    (*count)++;
    scheduler_collect_inorder(node->right, buffer, count);
}

static int scheduler_compare_by_arrival(const void *a, const void *b)
{
    const Request *ra = (const Request *)a;
    const Request *rb = (const Request *)b;
    if (ra->arrival_time < rb->arrival_time) return -1;
    if (ra->arrival_time > rb->arrival_time) return 1;
    if (ra->id < rb->id) return -1;
    if (ra->id > rb->id) return 1;
    return 0;
}

static uint32_t uint32_abs_diff(uint32_t a, uint32_t b)
{
    return (a >= b) ? (a - b) : (b - a);
}

static void scheduler_update_wait_stats(Scheduler *sched, uint64_t current_time,
                                        uint64_t arrival_time)
{
    if (sched == NULL) return;
    uint64_t wait = (current_time >= arrival_time) ? (current_time - arrival_time) : 0;
    if (wait > sched->max_wait_time) sched->max_wait_time = wait;
    sched->total_wait_time += wait;
    sched->wait_count++;
}

/* ------------------------------------------------------------------ */
/*  Aging                                                              */
/* ------------------------------------------------------------------ */

static void scheduler_apply_aging_rec(AVLNode *node, uint64_t current_time)
{
    if (node == NULL) return;
    scheduler_apply_aging_rec(node->left, current_time);
    uint64_t wait = current_time - node->req.arrival_time;
    int inc = (int)(wait / AGING_INTERVAL);
    if (inc > 0) node->req.priority += inc;
    scheduler_apply_aging_rec(node->right, current_time);
}

void scheduler_apply_aging(Scheduler *sched, uint64_t current_time)
{
    if (sched == NULL || sched->root == NULL) return;
    scheduler_apply_aging_rec(sched->root, current_time);
}

static AVLNode *scheduler_find_highest_priority(const AVLNode *root)
{
    if (root == NULL) return NULL;
    AVLNode *left_best = scheduler_find_highest_priority(root->left);
    AVLNode *right_best = scheduler_find_highest_priority(root->right);
    AVLNode *best = NULL;
    if (left_best != NULL && right_best != NULL)
        best = (left_best->req.priority >= right_best->req.priority) ? left_best : right_best;
    else if (left_best != NULL) best = left_best;
    else if (right_best != NULL) best = right_best;
    if (best == NULL || root->req.priority > best->req.priority) best = (AVLNode *)root;
    return best;
}

static AVLNode *scheduler_find_expired(const AVLNode *root, uint64_t current_time)
{
    if (root == NULL) return NULL;
    AVLNode *left = scheduler_find_expired(root->left, current_time);
    if (left != NULL) return left;
    if (current_time - root->req.arrival_time >= CONFIG_DEADLINE_LIMIT)
        return (AVLNode *)root;
    return scheduler_find_expired(root->right, current_time);
}

/* ------------------------------------------------------------------ */
/*  Coalescencia                                                        */
/* ------------------------------------------------------------------ */

void scheduler_coalesce(Scheduler *sched)
{
    if (sched == NULL || sched->root == NULL || sched->request_count < 2)
        return;

    uint32_t total = sched->request_count;
    Request *pending = malloc(sizeof(Request) * total);
    if (pending == NULL) return;
    uint32_t count = 0;
    scheduler_collect_inorder(sched->root, pending, &count);

    bool *to_remove = calloc(total, sizeof(bool));
    if (to_remove == NULL) { free(pending); return; }

    uint32_t removed = 0;
    for (uint32_t i = 0; i < total - 1; i++) {
        if (to_remove[i]) continue;
        uint32_t diff = uint32_abs_diff(pending[i].cylinder, pending[i+1].cylinder);
        if (diff <= COALESCE_THRESHOLD) {
            if (pending[i+1].arrival_time < pending[i].arrival_time)
                pending[i].arrival_time = pending[i+1].arrival_time;
            to_remove[i+1] = true;
            removed++;
        }
    }

    for (uint32_t i = 0; i < total; i++) {
        if (to_remove[i]) {
            scheduler_remove(sched, pending[i].cylinder, pending[i].id);
        }
    }
    free(pending);
    free(to_remove);
    if (removed > 0)
        printf("[Coalescencia] %u requisicoes mescladas.\n", removed);
}

/* ------------------------------------------------------------------ */
/*  Logging callback                                                    */
/* ------------------------------------------------------------------ */

void scheduler_set_log_callback(Scheduler *sched,
                                SchedulerLogCallback callback,
                                void *context)
{
    if (sched) {
        sched->log_callback = callback;
        sched->log_context = context;
    }
}

static void scheduler_log_move(Scheduler *sched)
{
    if (sched && sched->log_callback)
        sched->log_callback(sched);
}

/* ------------------------------------------------------------------ */
/*  Ciclo de vida                                                       */
/* ------------------------------------------------------------------ */

void scheduler_init(Scheduler *sched)
{
    if (sched == NULL) return;
    sched->root = NULL;
    sched->request_count = 0;
    sched->total_served = 0;
    sched->max_wait_time = 0;
    sched->total_wait_time = 0;
    sched->wait_count = 0;
    sched->log_callback = NULL;
    sched->log_context = NULL;
    disk_init(&sched->disk);
}

/* ------------------------------------------------------------------ */
/*  Manipulacao de requisicoes                                          */
/* ------------------------------------------------------------------ */

bool scheduler_add(Scheduler *sched, Request req)
{
    if (sched == NULL) return false;
    AVLNode *new_root = avl_insert(sched->root, req);
    if (new_root == NULL) return false;
    sched->root = new_root;
    sched->request_count++;
    return true;
}

bool scheduler_remove(Scheduler *sched, uint32_t cylinder, uint32_t id)
{
    if (sched == NULL) return false;
    if (avl_search(sched->root, cylinder, id) == NULL) return false;
    sched->root = avl_remove(sched->root, cylinder, id);
    sched->request_count--;
    sched->total_served++;
    return true;
}

/* ------------------------------------------------------------------ */
/*  Consultas de estado                                                 */
/* ------------------------------------------------------------------ */

bool scheduler_is_empty(const Scheduler *sched)
{
    return (sched == NULL || sched->request_count == 0);
}

AVLNode *scheduler_min(const Scheduler *sched)
{
    if (sched == NULL || sched->root == NULL) return NULL;
    return avl_min_node(sched->root);
}

AVLNode *scheduler_max(const Scheduler *sched)
{
    if (sched == NULL || sched->root == NULL) return NULL;
    return scheduler_max_node(sched->root);
}

/* ------------------------------------------------------------------ */
/*  Algoritmos de escalonamento                                         */
/* ------------------------------------------------------------------ */

bool scheduler_fcfs(Scheduler *sched)
{
    if (sched == NULL) return false;
    if (sched->request_count == 0) return true;

    uint32_t total = sched->request_count;
    Request *pending = malloc(sizeof(Request) * total);
    if (pending == NULL) return false;

    uint32_t count = 0;
    scheduler_collect_inorder(sched->root, pending, &count);
    qsort(pending, total, sizeof(Request), scheduler_compare_by_arrival);

    for (uint32_t i = 0; i < total; i++) {
        const Request *cur = &pending[i];
        if (!disk_move_head(&sched->disk, cur->cylinder)) continue;
        scheduler_log_move(sched);
        scheduler_update_wait_stats(sched, sched->disk.current_time, cur->arrival_time);
        scheduler_remove(sched, cur->cylinder, cur->id);
    }
    free(pending);
    return true;
}

bool scheduler_sstf(Scheduler *sched)
{
    if (sched == NULL) return false;

    while (sched->request_count > 0) {
        uint32_t head = sched->disk.head_position;
        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = ficticio.right = NULL;
        ficticio.height = 1;

        AVLNode *pred = avl_predecessor(sched->root, &ficticio);
        AVLNode *succ = avl_successor(sched->root, &ficticio);
        AVLNode *chosen = NULL;

        if (pred == NULL && succ == NULL) break;
        else if (pred == NULL) chosen = succ;
        else if (succ == NULL) chosen = pred;
        else {
            uint32_t dp = uint32_abs_diff(pred->req.cylinder, head);
            uint32_t ds = uint32_abs_diff(succ->req.cylinder, head);
            if (dp < ds) chosen = pred;
            else if (ds < dp) chosen = succ;
            else chosen = (pred->req.cylinder <= succ->req.cylinder) ? pred : succ;
        }

        if (!disk_move_head(&sched->disk, chosen->req.cylinder)) break;
        scheduler_log_move(sched);
        scheduler_update_wait_stats(sched, sched->disk.current_time, chosen->req.arrival_time);
        scheduler_remove(sched, chosen->req.cylinder, chosen->req.id);
    }
    return true;
}

bool scheduler_scan(Scheduler *sched)
{
    if (sched == NULL) return false;

    while (sched->request_count > 0) {
        uint32_t head = sched->disk.head_position;
        DiskDirection dir = sched->disk.direction;
        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = ficticio.right = NULL;
        ficticio.height = 1;

        AVLNode *chosen = NULL;
        if (dir == DIR_RIGHT) {
            chosen = avl_successor(sched->root, &ficticio);
            if (chosen == NULL) {
                sched->disk.direction = DIR_LEFT;
                continue;
            }
        } else {
            chosen = avl_predecessor(sched->root, &ficticio);
            if (chosen == NULL) {
                sched->disk.direction = DIR_RIGHT;
                continue;
            }
        }

        if (!disk_move_head(&sched->disk, chosen->req.cylinder)) break;
        scheduler_log_move(sched);
        scheduler_update_wait_stats(sched, sched->disk.current_time, chosen->req.arrival_time);
        scheduler_remove(sched, chosen->req.cylinder, chosen->req.id);
    }
    return true;
}

bool scheduler_cscan(Scheduler *sched)  /* C-LOOK */
{
    if (sched == NULL) return false;
    sched->disk.direction = DIR_RIGHT;

    while (sched->request_count > 0) {
        uint32_t head = sched->disk.head_position;
        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = ficticio.right = NULL;
        ficticio.height = 1;

        AVLNode *succ = avl_successor(sched->root, &ficticio);
        if (succ != NULL) {
            if (!disk_move_head(&sched->disk, succ->req.cylinder)) break;
            scheduler_log_move(sched);
            scheduler_update_wait_stats(sched, sched->disk.current_time, succ->req.arrival_time);
            scheduler_remove(sched, succ->req.cylinder, succ->req.id);
        } else {
            AVLNode *first = scheduler_min(sched);
            if (first == NULL) break;
            if (!disk_move_head(&sched->disk, first->req.cylinder)) break;
            scheduler_log_move(sched);
            /* O salto nao remove requisicao, apenas reposiciona */
        }
    }
    return true;
}

bool scheduler_deadline(Scheduler *sched)
{
    if (sched == NULL) return false;

    while (sched->request_count > 0) {
        uint64_t current_time = sched->disk.current_time;

        scheduler_apply_aging(sched, current_time);

        AVLNode *aged = scheduler_find_highest_priority(sched->root);
        if (aged != NULL && aged->req.priority >= AGING_THRESHOLD) {
            if (!disk_move_head(&sched->disk, aged->req.cylinder)) break;
            scheduler_log_move(sched);
            scheduler_update_wait_stats(sched, current_time, aged->req.arrival_time);
            scheduler_remove(sched, aged->req.cylinder, aged->req.id);
            continue;
        }

        AVLNode *expired = scheduler_find_expired(sched->root, current_time);
        if (expired != NULL) {
            if (!disk_move_head(&sched->disk, expired->req.cylinder)) break;
            scheduler_log_move(sched);
            scheduler_update_wait_stats(sched, current_time, expired->req.arrival_time);
            scheduler_remove(sched, expired->req.cylinder, expired->req.id);
            continue;
        }

        /* SSTF fallback */
        uint32_t head = sched->disk.head_position;
        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = ficticio.right = NULL;
        ficticio.height = 1;

        AVLNode *pred = avl_predecessor(sched->root, &ficticio);
        AVLNode *succ = avl_successor(sched->root, &ficticio);
        AVLNode *chosen = NULL;

        if (pred == NULL && succ == NULL) break;
        else if (pred == NULL) chosen = succ;
        else if (succ == NULL) chosen = pred;
        else {
            uint32_t dp = uint32_abs_diff(pred->req.cylinder, head);
            uint32_t ds = uint32_abs_diff(succ->req.cylinder, head);
            if (dp < ds) chosen = pred;
            else if (ds < dp) chosen = succ;
            else chosen = (pred->req.cylinder <= succ->req.cylinder) ? pred : succ;
        }

        if (!disk_move_head(&sched->disk, chosen->req.cylinder)) break;
        scheduler_log_move(sched);
        scheduler_update_wait_stats(sched, current_time, chosen->req.arrival_time);
        scheduler_remove(sched, chosen->req.cylinder, chosen->req.id);
    }
    return true;
}

/* ------------------------------------------------------------------ */
/*  Impressao                                                          */
/* ------------------------------------------------------------------ */

void scheduler_print_stats(const Scheduler *sched)
{
    if (sched == NULL) return;
    printf("=== Scheduler - estado atual ===\n");
    printf("  Requisicoes pendentes : %u\n", sched->request_count);
    printf("  Requisicoes atendidas : %llu\n", (unsigned long long)sched->total_served);
    disk_print_state(&sched->disk);
    printf("================================\n");
}