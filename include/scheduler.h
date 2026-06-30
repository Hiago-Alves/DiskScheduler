/**
 * @file    scheduler.h
 * @brief   Infraestrutura do escalonador de disco.
 *
 * Este módulo define a estrutura central do simulador: o Scheduler.
 * Ele agrupa o estado do disco (Disk) e a fila de requisições pendentes
 * (AVL ordenada por cylinder), oferecendo operações de ciclo de vida e
 * manipulação de requisições.
 *
 * Responsabilidades deste módulo:
 *   - Manter a raiz da AVL com todas as requisições pendentes
 *   - Manter o estado atualizado do disco (via struct Disk)
 *   - Controlar o contador de requisições na fila
 *   - Expor operações de inserção e remoção de requisições
 *   - Expor impressão de estatísticas do estado atual
 *
 * O Scheduler NÃO decide qual requisição atender — essa é
 * responsabilidade dos algoritmos implementados nos módulos futuros
 * (FCFS, SSTF, SCAN, C-SCAN, EDF, etc.), que receberão um ponteiro
 * para esta estrutura e a manipularão por meio das funções aqui
 * declaradas.
 *
 * Padrão: C11
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>   /* uint32_t, uint64_t */
#include <stdbool.h>  /* bool               */
#include "avl.h"      /* AVLNode, avl_*     */
#include "disk.h"     /* Disk, disk_*       */
#include "request.h"  /* Request            */

/* ------------------------------------------------------------------ */
/*  Estrutura principal                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Estado completo do escalonador de disco.
 *
 * Campos:
 *   root            — raiz da AVL de requisições pendentes
 *   disk            — estado físico do disco simulado
 *   request_count   — número de requisições atualmente na fila
 *   total_served    — número total de requisições atendidas desde
 *                     a inicialização (métrica acumulada)
 *
 * Os algoritmos de escalonamento recebem um ponteiro para esta struct
 * e interagem com ela exclusivamente pelas funções declaradas abaixo.
 */
typedef struct {

    /**
     * @brief Raiz da árvore AVL de requisições pendentes.
     *
     * Ordenada pelo campo cylinder de cada Request.
     * NULL quando a fila está vazia.
     * Atualizada pelas funções scheduler_add() e scheduler_remove().
     */
    AVLNode *root;

    /**
     * @brief Estado físico do disco rígido simulado.
     *
     * Mantém posição da cabeça, direção, tempo acumulado e
     * distância total percorrida. Atualizado pelos algoritmos via
     * disk_move_head() após cada decisão de escalonamento.
     */
    Disk disk;

    /**
     * @brief Número de requisições atualmente na fila (AVL).
     *
     * Incrementado em scheduler_add() e decrementado em
     * scheduler_remove(). Permite checagem rápida de fila vazia
     * sem percorrer a árvore.
     */
    uint32_t request_count;

    /**
     * @brief Total de requisições atendidas desde scheduler_init().
     *
     * Não é decrementado pela remoção — acumula ao longo de toda
     * a simulação. Usado nas estatísticas finais para calcular
     * médias de tempo de espera e distância por requisição.
     */
    uint64_t total_served;

} Scheduler;

/* ------------------------------------------------------------------ */
/*  Ciclo de vida                                                       */
/* ------------------------------------------------------------------ */

/**
 * @brief Inicializa o escalonador deixando-o pronto para uso.
 *
 * Define:
 *   - root          ← NULL (fila vazia)
 *   - disk          ← inicializado via disk_init()
 *   - request_count ← 0
 *   - total_served  ← 0
 *
 * Deve ser chamada uma única vez antes de qualquer outra operação
 * sobre o Scheduler.
 *
 * @param  sched  Ponteiro para o Scheduler a ser inicializado.
 *                Não deve ser NULL.
 */
void scheduler_init(Scheduler *sched);

/* ------------------------------------------------------------------ */
/*  Manipulação de requisições                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief Insere uma requisição na fila de pendências (AVL).
 *
 * Delega a inserção para avl_insert(), atualizando a raiz e
 * incrementando request_count.
 *
 * @param  sched  Ponteiro para o Scheduler. Não deve ser NULL.
 * @param  req    Requisição a inserir.
 * @return true  se a inserção foi bem-sucedida.
 *         false se sched for NULL ou avl_create_node() falhar
 *               (falha de alocação de memória).
 */
bool scheduler_add(Scheduler *sched, Request req);

/**
 * @brief Remove uma requisição da fila de pendências (AVL).
 *
 * Delega a remoção para avl_remove(), atualizando a raiz.
 * Decrementa request_count e incrementa total_served.
 *
 * Deve ser chamada pelos algoritmos de escalonamento após
 * mover a cabeça para o cilindro da requisição atendida.
 *
 * @param  sched     Ponteiro para o Scheduler. Não deve ser NULL.
 * @param  cylinder  Cilindro da requisição a remover.
 * @param  id        Id da requisição a remover.
 * @return true  se a remoção foi realizada (nó existia na AVL).
 *         false se sched for NULL ou a requisição não for encontrada.
 */
bool scheduler_remove(Scheduler *sched, uint32_t cylinder, uint32_t id);

/* ------------------------------------------------------------------ */
/*  Consultas de estado                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Informa se a fila de requisições está vazia.
 *
 * Equivale a (sched->request_count == 0), mas encapsula o acesso
 * direto ao campo, permitindo uso seguro pelos algoritmos.
 *
 * @param  sched  Ponteiro para o Scheduler. Não deve ser NULL.
 * @return true  se não há requisições pendentes.
 *         false se houver ao menos uma requisição na fila.
 */
bool scheduler_is_empty(const Scheduler *sched);

/**
 * @brief Retorna o nó da AVL com o menor cylinder na fila.
 *
 * Útil para algoritmos como SCAN (extremo esquerdo) e para
 * inicialização de buscas de vizinho mais próximo no SSTF.
 *
 * @param  sched  Ponteiro para o Scheduler. Não deve ser NULL.
 * @return Ponteiro para o AVLNode de menor cylinder,
 *         ou NULL se a fila estiver vazia.
 */
AVLNode *scheduler_min(const Scheduler *sched);

/**
 * @brief Retorna o nó da AVL com o maior cylinder na fila.
 *
 * Útil para algoritmos como SCAN e C-SCAN (extremo direito).
 *
 * @param  sched  Ponteiro para o Scheduler. Não deve ser NULL.
 * @return Ponteiro para o AVLNode de maior cylinder,
 *         ou NULL se a fila estiver vazia.
 */
AVLNode *scheduler_max(const Scheduler *sched);

/* ------------------------------------------------------------------ */
/*  Impressão e diagnóstico                                             */
/* ------------------------------------------------------------------ */

/**
 * @brief Imprime as estatísticas atuais do escalonador no stdout.
 *
 * Exibe:
 *   - Requisições pendentes na fila (request_count)
 *   - Total de requisições já atendidas (total_served)
 *   - Estado atual do disco via disk_print_state()
 *
 * Útil para depuração entre iterações dos algoritmos e para
 * apresentação dos resultados parciais da simulação.
 *
 * @param  sched  Ponteiro para o Scheduler. Não deve ser NULL.
 */
void scheduler_print_stats(const Scheduler *sched);

#endif /* SCHEDULER_H */