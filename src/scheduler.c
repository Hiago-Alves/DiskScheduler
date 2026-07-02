/**
 * @file    scheduler.c
 * @brief   Implementação da infraestrutura do escalonador de disco e
 *          dos algoritmos de escalonamento.
 *
 * Padrão: C11
 */

#include "../include/scheduler.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/*  Fallbacks para constantes (caso não definidas em config.h)         */
/* ------------------------------------------------------------------ */

#ifndef CONFIG_DEADLINE_LIMIT
#define CONFIG_DEADLINE_LIMIT 50
#endif

#ifndef AGING_INTERVAL
#define AGING_INTERVAL 5
#endif

#ifndef AGING_THRESHOLD
#define AGING_THRESHOLD 10
#endif

/* ------------------------------------------------------------------ */
/*  Funções auxiliares internas (static)                               */
/* ------------------------------------------------------------------ */

static AVLNode *scheduler_max_node(AVLNode *node)
{
    while (node->right != NULL) {
        node = node->right;
    }
    return node;
}

/**
 * @brief Percorre a AVL e retorna a primeira requisição expirada.
 */
static AVLNode *scheduler_find_expired(const AVLNode *root, uint64_t current_time)
{
    if (root == NULL) return NULL;

    AVLNode *left = scheduler_find_expired(root->left, current_time);
    if (left != NULL) return left;

    if (current_time - root->req.arrival_time >= CONFIG_DEADLINE_LIMIT) {
        return (AVLNode *)root;
    }

    return scheduler_find_expired(root->right, current_time);
}

/**
 * @brief Percorre a AVL e aplica aging a cada nó.
 */
static void scheduler_apply_aging_rec(AVLNode *node, uint64_t current_time)
{
    if (node == NULL) return;

    scheduler_apply_aging_rec(node->left, current_time);

    uint64_t wait_time = current_time - node->req.arrival_time;
    int increment = (int)(wait_time / AGING_INTERVAL);
    if (increment > 0) {
        node->req.priority += increment;
    }

    scheduler_apply_aging_rec(node->right, current_time);
}

/**
 * @brief Encontra a requisição com a maior prioridade na AVL.
 */
static AVLNode *scheduler_find_highest_priority(const AVLNode *root)
{
    if (root == NULL) return NULL;

    AVLNode *left_best = scheduler_find_highest_priority(root->left);
    AVLNode *right_best = scheduler_find_highest_priority(root->right);

    AVLNode *best = NULL;
    if (left_best != NULL && right_best != NULL) {
        best = (left_best->req.priority >= right_best->req.priority) ? left_best : right_best;
    } else if (left_best != NULL) {
        best = left_best;
    } else if (right_best != NULL) {
        best = right_best;
    }

    if (best == NULL || root->req.priority > best->req.priority) {
        best = (AVLNode *)root;
    }

    return best;
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
    const Request *req_a = (const Request *)a;
    const Request *req_b = (const Request *)b;

    if (req_a->arrival_time < req_b->arrival_time) return -1;
    if (req_a->arrival_time > req_b->arrival_time) return 1;

    if (req_a->id < req_b->id) return -1;
    if (req_a->id > req_b->id) return 1;

    return 0;
}

static uint32_t uint32_abs_diff(uint32_t a, uint32_t b)
{
    return (a >= b) ? (a - b) : (b - a);
}

/* ------------------------------------------------------------------ */
/*  Ciclo de vida                                                       */
/* ------------------------------------------------------------------ */

void scheduler_init(Scheduler *sched)
{
    if (sched == NULL) return;

    sched->root          = NULL;
    sched->request_count = 0;
    sched->total_served  = 0;

    disk_init(&sched->disk);
}

/* ------------------------------------------------------------------ */
/*  Manipulação de requisições                                          */
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
    if (sched == NULL) return true;
    return (sched->request_count == 0);
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
/*  FCFS                                                                */
/* ------------------------------------------------------------------ */

bool scheduler_fcfs(Scheduler *sched)
{
    if (sched == NULL) return false;
    if (sched->request_count == 0) return true;

    uint32_t total = sched->request_count;

    Request *pending = malloc(sizeof(Request) * (size_t)total);
    if (pending == NULL) return false;

    uint32_t collected = 0;
    scheduler_collect_inorder(sched->root, pending, &collected);

    qsort(pending, total, sizeof(Request), scheduler_compare_by_arrival);

    for (uint32_t i = 0; i < total; i++) {
        const Request *current = &pending[i];

        if (!disk_move_head(&sched->disk, current->cylinder)) {
            continue;
        }

        scheduler_remove(sched, current->cylinder, current->id);
    }

    free(pending);
    return true;
}

/* ------------------------------------------------------------------ */
/*  SSTF                                                               */
/* ------------------------------------------------------------------ */

bool scheduler_sstf(Scheduler *sched)
{
    if (sched == NULL) return false;

    while (sched->request_count > 0) {
        uint32_t head = sched->disk.head_position;

        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = NULL;
        ficticio.right = NULL;
        ficticio.height = 1;

        AVLNode *pred = avl_predecessor(sched->root, &ficticio);
        AVLNode *succ = avl_successor(sched->root, &ficticio);

        AVLNode *chosen = NULL;

        if (pred == NULL && succ == NULL) {
            break;
        } else if (pred == NULL) {
            chosen = succ;
        } else if (succ == NULL) {
            chosen = pred;
        } else {
            uint32_t dist_pred = uint32_abs_diff(pred->req.cylinder, head);
            uint32_t dist_succ = uint32_abs_diff(succ->req.cylinder, head);

            if (dist_pred < dist_succ) {
                chosen = pred;
            } else if (dist_succ < dist_pred) {
                chosen = succ;
            } else {
                chosen = (pred->req.cylinder <= succ->req.cylinder) ? pred : succ;
            }
        }

        if (!disk_move_head(&sched->disk, chosen->req.cylinder)) {
            break;
        }

        scheduler_remove(sched, chosen->req.cylinder, chosen->req.id);
    }

    return true;
}

/* ------------------------------------------------------------------ */
/*  SCAN                                                               */
/* ------------------------------------------------------------------ */

bool scheduler_scan(Scheduler *sched)
{
    if (sched == NULL) return false;

    while (sched->request_count > 0) {
        uint32_t head = sched->disk.head_position;
        DiskDirection dir = sched->disk.direction;

        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = NULL;
        ficticio.right = NULL;
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

        if (!disk_move_head(&sched->disk, chosen->req.cylinder)) {
            break;
        }
        scheduler_remove(sched, chosen->req.cylinder, chosen->req.id);
    }

    return true;
}

/* ------------------------------------------------------------------ */
/*  C-LOOK (antigo C-SCAN)                                             */
/* ------------------------------------------------------------------ */

bool scheduler_cscan(Scheduler *sched)
{
    if (sched == NULL) return false;

    sched->disk.direction = DIR_RIGHT;

    while (sched->request_count > 0) {
        uint32_t head = sched->disk.head_position;

        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = NULL;
        ficticio.right = NULL;
        ficticio.height = 1;

        AVLNode *succ = avl_successor(sched->root, &ficticio);

        if (succ != NULL) {
            if (!disk_move_head(&sched->disk, succ->req.cylinder)) {
                break;
            }
            scheduler_remove(sched, succ->req.cylinder, succ->req.id);
        } else {
            AVLNode *first = scheduler_min(sched);
            if (first == NULL) break;

            if (!disk_move_head(&sched->disk, first->req.cylinder)) {
                break;
            }
        }
    }

    return true;
}

/* ------------------------------------------------------------------ */
/*  Aging público                                                       */
/* ------------------------------------------------------------------ */

void scheduler_apply_aging(Scheduler *sched, uint64_t current_time)
{
    if (sched == NULL || sched->root == NULL) return;
    scheduler_apply_aging_rec(sched->root, current_time);
}

/* ------------------------------------------------------------------ */
/*  DEADLINE (com aging)                                               */
/* ------------------------------------------------------------------ */

bool scheduler_deadline(Scheduler *sched)
{
    if (sched == NULL) return false;

    while (sched->request_count > 0) {
        uint64_t current_time = sched->disk.current_time;

        /* Aging */
        scheduler_apply_aging(sched, current_time);

        /* Requisição com prioridade >= limiar */
        AVLNode *aged = scheduler_find_highest_priority(sched->root);
        if (aged != NULL && aged->req.priority >= AGING_THRESHOLD) {
            if (!disk_move_head(&sched->disk, aged->req.cylinder)) break;
            scheduler_remove(sched, aged->req.cylinder, aged->req.id);
            continue;
        }

        /* Requisição expirada */
        AVLNode *expired = scheduler_find_expired(sched->root, current_time);
        if (expired != NULL) {
            if (!disk_move_head(&sched->disk, expired->req.cylinder)) break;
            scheduler_remove(sched, expired->req.cylinder, expired->req.id);
            continue;
        }

        /* SSTF */
        uint32_t head = sched->disk.head_position;
        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = NULL;
        ficticio.right = NULL;
        ficticio.height = 1;

        AVLNode *pred = avl_predecessor(sched->root, &ficticio);
        AVLNode *succ = avl_successor(sched->root, &ficticio);
        AVLNode *chosen = NULL;

        if (pred == NULL && succ == NULL) {
            break;
        } else if (pred == NULL) {
            chosen = succ;
        } else if (succ == NULL) {
            chosen = pred;
        } else {
            uint32_t dist_pred = uint32_abs_diff(pred->req.cylinder, head);
            uint32_t dist_succ = uint32_abs_diff(succ->req.cylinder, head);
            if (dist_pred < dist_succ) {
                chosen = pred;
            } else if (dist_succ < dist_pred) {
                chosen = succ;
            } else {
                chosen = (pred->req.cylinder <= succ->req.cylinder) ? pred : succ;
            }
        }

        if (!disk_move_head(&sched->disk, chosen->req.cylinder)) break;
        scheduler_remove(sched, chosen->req.cylinder, chosen->req.id);
    }

    return true;
}

/* ------------------------------------------------------------------ */
/*  Impressão                                                           */
/* ------------------------------------------------------------------ */

void scheduler_print_stats(const Scheduler *sched)
{
    if (sched == NULL) return;

    printf("=== Scheduler — estado atual ===\n");
    printf("  Requisicoes pendentes : %u\n",  sched->request_count);
    printf("  Requisicoes atendidas : %llu\n",
           (unsigned long long)sched->total_served);

    disk_print_state(&sched->disk);

    printf("================================\n");
}