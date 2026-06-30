/**
 * @file  avl.c
 * @brief Implementação da base da árvore AVL.
 *
 * Padrão: C11
 */

#include "../include/avl.h"

#include <stdlib.h>  /* malloc  */

/* ------------------------------------------------------------------ */
/*  Funções base                                                        */
/* ------------------------------------------------------------------ */

/** Retorna 0 para NULL; campo height caso contrário. */
int32_t avl_height(const AVLNode *node)
{
    return (node == NULL) ? 0 : node->height;
}

/** Retorna o maior de dois inteiros. */
int32_t avl_max(int32_t a, int32_t b)
{
    return (a > b) ? a : b;
}

/**
 * Recalcula height como 1 + max(height(left), height(right)).
 * Deve ser chamada após qualquer alteração nos filhos do nó.
 */
void avl_update_height(AVLNode *node)
{
    if (node == NULL) return;
    node->height = 1 + avl_max(avl_height(node->left),
                                avl_height(node->right));
}

/** Fator de balanceamento = altura esquerda − altura direita. */
int32_t avl_balance_factor(const AVLNode *node)
{
    if (node == NULL) return 0;
    return avl_height(node->left) - avl_height(node->right);
}

/**
 * Aloca um nó com malloc, copia a Request por valor,
 * inicializa filhos como NULL e height como 1 (folha).
 */
AVLNode *avl_create_node(Request req)
{
    AVLNode *node = malloc(sizeof(AVLNode));
    if (node == NULL) return NULL;   /* falha de alocação */

    node->req    = req;   /* cópia por valor — sem ponteiro pendente */
    node->left   = NULL;
    node->right  = NULL;
    node->height = 1;     /* folha recém-criada sempre tem altura 1  */

    return node;
}