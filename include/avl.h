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

/* ------------------------------------------------------------------ */
/*  Inserção                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief Insere uma requisição na AVL ordenada por cylinder.
 *
 * Inserção recursiva padrão de BST seguida de rebalanceamento
 * automático pelos quatro casos (LL, RR, LR, RL).
 *
 * Cilindros duplicados são permitidos: o novo nó vai para a
 * subárvore direita (cylinder >= raiz vai à direita).
 *
 * @param  root  Raiz atual da subárvore (pode ser NULL).
 * @param  req   Requisição a inserir.
 * @return Nova raiz da subárvore após inserção e balanceamento.
 */
AVLNode *avl_insert(AVLNode *root, Request req);

/* ------------------------------------------------------------------ */
/*  Busca e Remoção                                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Retorna o nó com o menor cylinder na subárvore.
 *
 * Caminha sempre à esquerda até o fim.
 * Usado internamente pela remoção para encontrar o sucessor in-order
 * (menor nó da subárvore direita) como substituto do nó removido.
 *
 * @param  node  Raiz da subárvore (não pode ser NULL).
 * @return Ponteiro para o nó mínimo (nunca NULL se node != NULL).
 */
AVLNode *avl_min_node(AVLNode *node);

/**
 * @brief Busca uma requisição pelo cylinder e pelo id.
 *
 * Desce pela árvore usando cylinder como chave. Quando encontra
 * um nó com o cylinder pedido, compara também o id para distinguir
 * requisições duplicadas no mesmo cilindro.
 *
 * @param  root      Raiz da subárvore.
 * @param  cylinder  Cilindro a localizar.
 * @param  id        Id da requisição a localizar.
 * @return Ponteiro para o nó encontrado, ou NULL se não existir.
 */
AVLNode *avl_search(AVLNode *root, uint32_t cylinder, uint32_t id);

/**
 * @brief Remove a requisição com o cylinder e id fornecidos.
 *
 * Remoção recursiva de BST com três casos:
 *   - Nó folha        → libera e retorna NULL.
 *   - Um filho        → substitui pelo filho existente.
 *   - Dois filhos     → substitui pelo sucessor in-order,
 *                       remove o sucessor da subárvore direita.
 * Após remover, rebalanceia pelo mesmo mecanismo da inserção.
 *
 * @param  root      Raiz atual da subárvore.
 * @param  cylinder  Cilindro do nó a remover.
 * @param  id        Id da requisição a remover.
 * @return Nova raiz da subárvore após remoção e balanceamento.
 */
AVLNode *avl_remove(AVLNode *root, uint32_t cylinder, uint32_t id);

/* ------------------------------------------------------------------ */
/*  Predecessor e Sucessor                                             */
/* ------------------------------------------------------------------ */

/**
 * @brief Retorna o nó predecessor de @p node na árvore.
 *
 * O predecessor é o nó com o maior cylinder estritamente menor
 * que node->req.cylinder — o vizinho imediato à esquerda na
 * travessia in-order.
 *
 * Estratégia (sem ponteiro de pai):
 *   - Se node possui subárvore esquerda: desce nela sempre à
 *     direita (rightmost) — esse é o maior valor menor que node.
 *   - Caso contrário: desce a partir de @p root mantendo um
 *     ponteiro @c candidate atualizado toda vez que viramos à
 *     direita; o último candidate é o predecessor.
 *
 * @param  root  Raiz da árvore inteira. Não deve ser NULL.
 * @param  node  Nó de referência. Não deve ser NULL.
 * @return Ponteiro para o nó predecessor, ou NULL se @p node for
 *         o menor elemento da árvore (não há predecessor).
 *
 * @note Complexidade: O(h), onde h é a altura da árvore.
 */
AVLNode *avl_predecessor(AVLNode *root, const AVLNode *node);

/**
 * @brief Retorna o nó sucessor de @p node na árvore.
 *
 * O sucessor é o nó com o menor cylinder estritamente maior
 * que node->req.cylinder — o vizinho imediato à direita na
 * travessia in-order.
 *
 * Estratégia (sem ponteiro de pai):
 *   - Se node possui subárvore direita: desce nela sempre à
 *     esquerda (leftmost) — esse é o menor valor maior que node.
 *     Essa lógica já existe em @c avl_min_node; reutilizamos aqui.
 *   - Caso contrário: desce a partir de @p root mantendo um
 *     ponteiro @c candidate atualizado toda vez que viramos à
 *     esquerda; o último candidate é o sucessor.
 *
 * @param  root  Raiz da árvore inteira. Não deve ser NULL.
 * @param  node  Nó de referência. Não deve ser NULL.
 * @return Ponteiro para o nó sucessor, ou NULL se @p node for
 *         o maior elemento da árvore (não há sucessor).
 *
 * @note Complexidade: O(h), onde h é a altura da árvore.
 */
AVLNode *avl_successor(AVLNode *root, const AVLNode *node);

#endif /* AVL_H */