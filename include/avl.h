/**
 * @file  avl.h
 * @brief Base da árvore AVL para o escalonador de disco.
 *
 * Cada nó armazena uma Request. A chave de ordenação é
 * req.cylinder — decisão dos algoritmos de escalonamento.
 *
 * Padrão: C11
 */

#ifndef AVL_H
#define AVL_H

#include <stdint.h>   /* int32_t          */
#include <stddef.h>   /* NULL             */
#include "request.h"

/* ------------------------------------------------------------------ */
/*  Nó da AVL                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief Nó da árvore AVL.
 *
 * Campos:
 *   req    — dado armazenado (requisição de disco)
 *   left   — filho esquerdo  (cylinder menor)
 *   right  — filho direito   (cylinder maior)
 *   height — altura do nó (folha = 1, NULL = 0)
 */
typedef struct AVLNode {
    Request          req;
    struct AVLNode  *left;
    struct AVLNode  *right;
    int32_t          height;
} AVLNode;

/* ------------------------------------------------------------------ */
/*  Protótipos                                                          */
/* ------------------------------------------------------------------ */

/** Retorna a altura de um nó (0 se NULL). */
int32_t avl_height(const AVLNode *node);

/** Retorna o maior entre dois inteiros. */
int32_t avl_max(int32_t a, int32_t b);

/** Recalcula a altura de um nó a partir de seus filhos. */
void avl_update_height(AVLNode *node);

/**
 * @brief Fator de balanceamento: height(left) - height(right).
 *
 * -1, 0 ou 1 → balanceado.
 * < -1 ou > 1 → desbalanceado (exige rotação).
 */
int32_t avl_balance_factor(const AVLNode *node);

/**
 * @brief Aloca e inicializa um novo nó com a requisição fornecida.
 *
 * @return Ponteiro para o nó, ou NULL em falha de alocação.
 */
AVLNode *avl_create_node(Request req);

/* ------------------------------------------------------------------ */
/*  Rotações                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief Rotação simples à esquerda (caso RR).
 *
 * Uso: bf(node) < -1 e bf(node->right) <= 0
 *
 *      node                 right
 *      /  \                 /   \
 *     T1  right   →      node   T3
 *         /  \           /  \
 *        T2  T3         T1  T2
 *
 * @return Nova raiz da subárvore.
 */
AVLNode *avl_rotate_left(AVLNode *node);

/**
 * @brief Rotação simples à direita (caso LL).
 *
 * Uso: bf(node) > 1 e bf(node->left) >= 0
 *
 *        node              left
 *        /  \              /  \
 *      left  T3   →      T1  node
 *      /  \                   /  \
 *     T1  T2                T2   T3
 *
 * @return Nova raiz da subárvore.
 */
AVLNode *avl_rotate_right(AVLNode *node);

#endif /* AVL_H */