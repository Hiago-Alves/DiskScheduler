/**
 * @file    example_scheduler.c
 * @brief   Exemplo de uso do escalonador de disco com FCFS e SSTF.
 */

#include "../include/scheduler.h"
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/*  Função auxiliar para imprimir a árvore em-ordem                   */
/* ------------------------------------------------------------------ */

static void avl_print_inorder_rec(const AVLNode *node)
{
    if (node == NULL) return;
    avl_print_inorder_rec(node->left);
    printf("[cilindro: %u] ", node->req.cylinder);
    avl_print_inorder_rec(node->right);
}

static void avl_print_inorder(const AVLNode *root)
{
    printf("AVL (ordem crescente de cilindro): ");
    avl_print_inorder_rec(root);
    printf("\n");
}

/* ------------------------------------------------------------------ */
/*  Principal                                                          */
/* ------------------------------------------------------------------ */

int main(void)
{
    const uint32_t ids[]        = {1, 2, 3, 4, 5, 6, 7, 8};
    const uint32_t cylinders[]  = {15, 180, 40, 90, 5, 160, 75, 130};
    const uint64_t arrivals[]   = {0, 5, 10, 15, 20, 25, 30, 35};
    const uint32_t num_reqs     = 8;

    /* --- FCFS --- */
    printf("=== TESTE FCFS ===\n\n");
    Scheduler sched_fcfs;
    scheduler_init(&sched_fcfs);

    for (uint32_t i = 0; i < num_reqs; i++) {
        Request req = request_create(ids[i], cylinders[i], 1,
                                     arrivals[i], OP_READ, 0, 0);
        if (!scheduler_add(&sched_fcfs, req)) {
            fprintf(stderr, "Falha ao adicionar req %u\n", ids[i]);
            return EXIT_FAILURE;
        }
    }

    printf("Antes do FCFS:\n");
    scheduler_print_stats(&sched_fcfs);
    avl_print_inorder(sched_fcfs.root);

    printf("\nExecutando FCFS...\n");
    scheduler_fcfs(&sched_fcfs);

    printf("\nDepois do FCFS:\n");
    scheduler_print_stats(&sched_fcfs);
    avl_print_inorder(sched_fcfs.root);

    /* --- SSTF --- */
    printf("\n\n=== TESTE SSTF ===\n\n");
    Scheduler sched_sstf;
    scheduler_init(&sched_sstf);

    for (uint32_t i = 0; i < num_reqs; i++) {
        Request req = request_create(ids[i], cylinders[i], 1,
                                     arrivals[i], OP_READ, 0, 0);
        if (!scheduler_add(&sched_sstf, req)) {
            fprintf(stderr, "Falha ao adicionar req %u\n", ids[i]);
            return EXIT_FAILURE;
        }
    }

    printf("Antes do SSTF:\n");
    scheduler_print_stats(&sched_sstf);
    avl_print_inorder(sched_sstf.root);

    printf("\nExecutando SSTF...\n");
    scheduler_sstf(&sched_sstf);

    printf("\nDepois do SSTF:\n");
    scheduler_print_stats(&sched_sstf);
    avl_print_inorder(sched_sstf.root);

    return EXIT_SUCCESS;
}