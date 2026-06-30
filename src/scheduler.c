/**
 * @file    scheduler.c
 * @brief   Implementação da infraestrutura do escalonador de disco.
 *
 * Padrão: C11
 */

#include "../include/scheduler.h"

#include <stdio.h>    /* printf  */

/* ------------------------------------------------------------------ */
/*  Funções auxiliares internas (static)                               */
/* ------------------------------------------------------------------ */

/**
 * Caminha sempre à direita até o fim da subárvore.
 * Retorna o nó com o maior cylinder — espelho de avl_min_node().
 *
 * Declarada static pois é detalhe de implementação deste módulo;
 * avl.h não precisa expor uma avl_max_node() pública por ora.
 *
 * @param  node  Raiz da subárvore (não pode ser NULL).
 * @return Ponteiro para o nó de maior cylinder.
 */
static AVLNode *scheduler_max_node(AVLNode *node)
{
    while (node->right != NULL) {
        node = node->right;
    }
    return node;
}

/* ------------------------------------------------------------------ */
/*  Ciclo de vida                                                       */
/* ------------------------------------------------------------------ */

/**
 * Zera todos os campos do Scheduler e delega a inicialização do disco
 * para disk_init(). Após esta chamada o escalonador está pronto para
 * receber requisições via scheduler_add().
 */
void scheduler_init(Scheduler *sched)
{
    if (sched == NULL) return;

    sched->root          = NULL;   /* fila começa vazia              */
    sched->request_count = 0;
    sched->total_served  = 0;

    disk_init(&sched->disk);       /* delega estado do disco ao módulo Disk */
}

/* ------------------------------------------------------------------ */
/*  Manipulação de requisições                                          */
/* ------------------------------------------------------------------ */

/**
 * Delega a inserção para avl_insert(), que retorna a nova raiz
 * (podendo diferir da raiz anterior após rotações de rebalanceamento).
 * Incrementa request_count apenas se a árvore de fato cresceu.
 *
 * A detecção de falha de alocação compara o ponteiro retornado por
 * avl_insert() com NULL: se a raiz era NULL e continua NULL após a
 * chamada, houve falha em avl_create_node().
 */
bool scheduler_add(Scheduler *sched, Request req)
{
    if (sched == NULL) return false;

    AVLNode *new_root = avl_insert(sched->root, req);

    /* avl_insert() só retorna NULL se avl_create_node() falhar e
     * a árvore estava vazia. Em qualquer outro caso, retorna a raiz. */
    if (new_root == NULL) return false;

    sched->root = new_root;
    sched->request_count++;

    return true;
}

/**
 * Antes de remover, verifica se o nó existe via avl_search().
 * Isso evita decrementar request_count e incrementar total_served
 * para uma remoção que não aconteceu de fato.
 *
 * Após confirmar a existência, delega para avl_remove() e atualiza
 * os contadores.
 */
bool scheduler_remove(Scheduler *sched, uint32_t cylinder, uint32_t id)
{
    if (sched == NULL) return false;

    /* Confirma existência antes de alterar contadores. */
    if (avl_search(sched->root, cylinder, id) == NULL) return false;

    sched->root = avl_remove(sched->root, cylinder, id);

    sched->request_count--;
    sched->total_served++;

    return true;
}

/* ------------------------------------------------------------------ */
/*  Consultas de estado                                                 */
/* ------------------------------------------------------------------ */

/** Checagem direta do contador — O(1). */
bool scheduler_is_empty(const Scheduler *sched)
{
    if (sched == NULL) return true;
    return (sched->request_count == 0);
}

/**
 * Delega para avl_min_node(), que caminha sempre à esquerda.
 * Retorna NULL se a fila estiver vazia.
 */
AVLNode *scheduler_min(const Scheduler *sched)
{
    if (sched == NULL || sched->root == NULL) return NULL;
    return avl_min_node(sched->root);
}

/**
 * Delega para scheduler_max_node() (static), que caminha sempre
 * à direita. Retorna NULL se a fila estiver vazia.
 */
AVLNode *scheduler_max(const Scheduler *sched)
{
    if (sched == NULL || sched->root == NULL) return NULL;
    return scheduler_max_node(sched->root);
}

/* ------------------------------------------------------------------ */
/*  Impressão e diagnóstico                                             */
/* ------------------------------------------------------------------ */

/**
 * Imprime um bloco formatado com o estado atual do escalonador.
 * A linha do disco é delegada para disk_print_state(), mantendo
 * a responsabilidade de formatação dentro do módulo correto.
 */
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