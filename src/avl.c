/**
 * @file  avl.c
 * @brief Implementação da base da árvore AVL.
 *
 * Padrão: C11
 */

#include "../include/avl.h"

#include <stdlib.h>  /* malloc, free */

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

/* ------------------------------------------------------------------ */
/*  Rotações                                                            */
/* ------------------------------------------------------------------ */

/**
 * Rotação à esquerda (caso RR).
 *
 * `right` sobe e toma o lugar de `node`.
 * O filho esquerdo de `right` (T2) passa a ser filho direito de `node`,
 * preservando a propriedade de BST (T2 > node e T2 < right).
 * Alturas de `node` e `right` são recalculadas de baixo para cima.
 */
AVLNode *avl_rotate_left(AVLNode *node)
{
    AVLNode *right = node->right;  /* novo pai              */
    AVLNode *t2    = right->left;  /* subárvore realocada   */

    right->left = node;   /* node desce para filho esquerdo   */
    node->right = t2;     /* T2 migra para direita de node    */

    avl_update_height(node);   /* node primeiro — está mais baixo  */
    avl_update_height(right);  /* right depois  — depende de node  */

    return right;  /* nova raiz da subárvore */
}

/**
 * Rotação à direita (caso LL).
 *
 * `left` sobe e toma o lugar de `node`.
 * O filho direito de `left` (T2) passa a ser filho esquerdo de `node`,
 * preservando a propriedade de BST (T2 > left e T2 < node).
 * Alturas de `node` e `left` são recalculadas de baixo para cima.
 */
AVLNode *avl_rotate_right(AVLNode *node)
{
    AVLNode *left = node->left;   /* novo pai              */
    AVLNode *t2   = left->right;  /* subárvore realocada   */

    left->right = node;   /* node desce para filho direito    */
    node->left  = t2;     /* T2 migra para esquerda de node   */

    avl_update_height(node);   /* node primeiro — está mais baixo  */
    avl_update_height(left);   /* left depois   — depende de node  */

    return left;   /* nova raiz da subárvore */
}

/* ------------------------------------------------------------------ */
/*  Inserção                                                            */
/* ------------------------------------------------------------------ */

/**
 * Insere recursivamente uma Request na AVL ordenada por cylinder.
 *
 * Etapas por chamada recursiva:
 *   1. Caso base: cria o nó folha quando root == NULL.
 *   2. Desce à esquerda se cylinder < raiz, à direita caso contrário.
 *   3. Atualiza a altura do nó corrente na volta da recursão.
 *   4. Calcula o fator de balanceamento e aplica a rotação adequada:
 *
 *      bf >  1 e key <  left->cylinder  → LL → rotateRight
 *      bf >  1 e key >= left->cylinder  → LR → rotateLeft(left) + rotateRight
 *      bf < -1 e key >= right->cylinder → RR → rotateLeft
 *      bf < -1 e key <  right->cylinder → RL → rotateRight(right) + rotateLeft
 *
 *   5. Retorna a (nova) raiz da subárvore para o nível acima.
 */
AVLNode *avl_insert(AVLNode *root, Request req)
{
    /* --- passo 1: inserção BST normal --- */
    if (root == NULL)
        return avl_create_node(req);   /* folha nova */

    if (req.cylinder < root->req.cylinder)
        root->left  = avl_insert(root->left,  req);
    else                                           /* >= vai à direita */
        root->right = avl_insert(root->right, req);

    /* --- passo 2: atualiza altura na volta da recursão --- */
    avl_update_height(root);

    /* --- passo 3: verifica balanceamento --- */
    int32_t bf = avl_balance_factor(root);

    /* LL — subárvore esquerda pesada, inserção foi à esquerda */
    if (bf > 1 && req.cylinder < root->left->req.cylinder)
        return avl_rotate_right(root);

    /* RR — subárvore direita pesada, inserção foi à direita */
    if (bf < -1 && req.cylinder >= root->right->req.cylinder)
        return avl_rotate_left(root);

    /* LR — subárvore esquerda pesada, inserção foi à direita dela */
    if (bf > 1 && req.cylinder >= root->left->req.cylinder) {
        root->left = avl_rotate_left(root->left);
        return avl_rotate_right(root);
    }

    /* RL — subárvore direita pesada, inserção foi à esquerda dela */
    if (bf < -1 && req.cylinder < root->right->req.cylinder) {
        root->right = avl_rotate_right(root->right);
        return avl_rotate_left(root);
    }

    return root;   /* balanceado: retorna a raiz sem alteração */
}

/* ------------------------------------------------------------------ */
/*  Busca e Remoção                                                     */
/* ------------------------------------------------------------------ */

/** Caminha à esquerda até o fim; retorna o nó de menor cylinder. */
AVLNode *avl_min_node(AVLNode *node)
{
    while (node->left != NULL)
        node = node->left;
    return node;
}

/**
 * Busca por cylinder + id.
 *
 * Desce como BST usando cylinder. Ao encontrar um nó com o cylinder
 * correto, verifica o id para resolver duplicatas:
 *   - id bate  → achou, retorna o nó.
 *   - id maior → continua à direita (duplicatas vão à direita).
 *   - id menor → continua à esquerda (não deve ocorrer em uso normal,
 *                mas tratado defensivamente).
 */
AVLNode *avl_search(AVLNode *root, uint32_t cylinder, uint32_t id)
{
    if (root == NULL) return NULL;   /* não encontrado */

    if (cylinder < root->req.cylinder)
        return avl_search(root->left,  cylinder, id);

    if (cylinder > root->req.cylinder)
        return avl_search(root->right, cylinder, id);

    /* cylinder == root: compara id para distinguir duplicatas */
    if (id == root->req.id)   return root;
    if (id >  root->req.id)   return avl_search(root->right, cylinder, id);
                               return avl_search(root->left,  cylinder, id);
}

/**
 * Remove o nó com (cylinder, id) e rebalanceia na volta da recursão.
 *
 * Caso 1 — nó folha: libera e retorna NULL.
 * Caso 2 — um filho: retorna o filho sobrevivente (free no nó atual).
 * Caso 3 — dois filhos: copia o conteúdo do sucessor in-order para
 *           o nó atual e remove o sucessor da subárvore direita.
 *           Isso evita a troca de ponteiros e simplifica o código.
 *
 * Rebalanceamento idêntico ao da inserção, mas o critério usa o
 * fator de balanceamento dos filhos (não a chave inserida), pois
 * qualquer remoção pode desbalancear qualquer um dos lados.
 */
AVLNode *avl_remove(AVLNode *root, uint32_t cylinder, uint32_t id)
{
    if (root == NULL) return NULL;   /* nó não encontrado */

    /* --- desce na direção correta --- */
    if (cylinder < root->req.cylinder) {
        root->left  = avl_remove(root->left,  cylinder, id);
    } else if (cylinder > root->req.cylinder) {
        root->right = avl_remove(root->right, cylinder, id);
    } else {
        /* cylinder bate: verifica id para duplicatas */
        if (id != root->req.id) {
            /* id maior → está à direita; senão à esquerda */
            if (id > root->req.id)
                root->right = avl_remove(root->right, cylinder, id);
            else
                root->left  = avl_remove(root->left,  cylinder, id);
            goto rebalance;
        }

        /* --- nó encontrado: remove --- */

        /* caso 1 e 2: zero ou um filho */
        if (root->left == NULL || root->right == NULL) {
            AVLNode *child = (root->left != NULL) ? root->left
                                                  : root->right;
            free(root);
            return child;   /* child pode ser NULL (folha) */
        }

        /* caso 3: dois filhos — substitui pelo sucessor in-order */
        AVLNode *successor = avl_min_node(root->right);
        root->req   = successor->req;   /* copia conteúdo por valor  */
        /* remove o sucessor da subárvore direita */
        root->right = avl_remove(root->right,
                                 successor->req.cylinder,
                                 successor->req.id);
    }

rebalance:
    /* --- atualiza altura e rebalanceia --- */
    avl_update_height(root);

    int32_t bf = avl_balance_factor(root);

    /* LL */
    if (bf > 1 && avl_balance_factor(root->left) >= 0)
        return avl_rotate_right(root);

    /* LR */
    if (bf > 1 && avl_balance_factor(root->left) < 0) {
        root->left = avl_rotate_left(root->left);
        return avl_rotate_right(root);
    }

    /* RR */
    if (bf < -1 && avl_balance_factor(root->right) <= 0)
        return avl_rotate_left(root);

    /* RL */
    if (bf < -1 && avl_balance_factor(root->right) > 0) {
        root->right = avl_rotate_right(root->right);
        return avl_rotate_left(root);
    }

    return root;
}

/* ------------------------------------------------------------------ */
/*  Predecessor e Sucessor                                             */
/* ------------------------------------------------------------------ */

/**
 * Predecessor — maior cylinder estritamente menor que node->req.cylinder.
 *
 * Caso 1: node possui subárvore esquerda.
 *
 *      node
 *      /
 *    [L]                 ← desce aqui
 *      \
 *       ...
 *         \
 *       [predecessor]   ← rightmost de L
 *
 * Basta caminhar sempre à direita dentro de node->left.
 * Esse nó é garantidamente o maior valor abaixo de node->cylinder.
 *
 * Caso 2: node NÃO possui subárvore esquerda.
 *
 * Descemos a partir da raiz como em uma busca BST normal.
 * Toda vez que viramos à DIREITA (current->cylinder < node->cylinder),
 * o nó atual É um candidato a predecessor — ele é menor que node
 * e é o maior que já vimos nesse caminho.
 * Quando chegamos ao nó alvo, o último candidate guardado é
 * o predecessor correto.
 *
 *      [candidate] ← atualizado aqui (viramos à direita)
 *            \
 *            ...
 *              \
 *             [node]  (sem filho esquerdo)
 */
AVLNode *avl_predecessor(AVLNode *root, const AVLNode *node)
{
    /* Caso 1: subárvore esquerda existe — rightmost dela é o predecessor. */
    if (node->left != NULL) {
        AVLNode *current = node->left;

        /* Desce sempre à direita até o fim. */
        while (current->right != NULL) {
            current = current->right;
        }

        return current;
    }

    /* Caso 2: sem subárvore esquerda — busca a partir da raiz. */
    AVLNode *candidate = NULL;  /* melhor predecessor visto até agora */
    AVLNode *current   = root;

    while (current != NULL) {

        if (node->req.cylinder > current->req.cylinder) {
            /*
             * current é menor que node: é um candidato válido.
             * Viramos à direita para encontrar um valor ainda maior
             * (mas ainda menor que node).
             */
            candidate = current;
            current   = current->right;

        } else if (node->req.cylinder < current->req.cylinder) {
            /*
             * current é maior que node: não pode ser predecessor.
             * Viramos à esquerda para encontrar valores menores.
             */
            current = current->left;

        } else {
            /*
             * cylinder igual: distingue pelo id (igual à lógica de
             * avl_search). Se o id de node for maior, current ainda
             * pode ser predecessor — atualiza candidate e vai à direita.
             * Se for menor, vai à esquerda sem atualizar candidate.
             */
            if (node->req.id > current->req.id) {
                candidate = current;
                current   = current->right;
            } else if (node->req.id < current->req.id) {
                current = current->left;
            } else {
                /* nó exato encontrado — para a busca */
                break;
            }
        }
    }

    return candidate;   /* NULL se node é o menor elemento */
}

/**
 * Sucessor — menor cylinder estritamente maior que node->req.cylinder.
 *
 * Caso 1: node possui subárvore direita.
 *
 *      node
 *          \
 *          [R]               ← desce aqui
 *          /
 *        ...
 *        /
 *     [successor]            ← leftmost de R
 *
 * Reutilizamos avl_min_node(node->right), que já faz exatamente isso.
 *
 * Caso 2: node NÃO possui subárvore direita.
 *
 * Descemos a partir da raiz como em uma busca BST normal.
 * Toda vez que viramos à ESQUERDA (current->cylinder > node->cylinder),
 * o nó atual É um candidato a sucessor — ele é maior que node
 * e é o menor que já vimos nesse caminho.
 * Quando chegamos ao nó alvo, o último candidate guardado é
 * o sucessor correto.
 *
 *      [candidate] ← atualizado aqui (viramos à esquerda)
 *            /
 *           ...
 *           /
 *         [node]  (sem filho direito)
 */
AVLNode *avl_successor(AVLNode *root, const AVLNode *node)
{
    /* Caso 1: subárvore direita existe — leftmost dela é o sucessor. */
    if (node->right != NULL) {
        return avl_min_node(node->right);   /* reutiliza função existente */
    }

    /* Caso 2: sem subárvore direita — busca a partir da raiz. */
    AVLNode *candidate = NULL;  /* melhor sucessor visto até agora */
    AVLNode *current   = root;

    while (current != NULL) {

        if (node->req.cylinder < current->req.cylinder) {
            /*
             * current é maior que node: é um candidato válido.
             * Viramos à esquerda para encontrar um valor ainda menor
             * (mas ainda maior que node).
             */
            candidate = current;
            current   = current->left;

        } else if (node->req.cylinder > current->req.cylinder) {
            /*
             * current é menor que node: não pode ser sucessor.
             * Viramos à direita para encontrar valores maiores.
             */
            current = current->right;

        } else {
            /*
             * cylinder igual: distingue pelo id (espelho de avl_predecessor).
             * Se o id de node for menor, current ainda pode ser sucessor —
             * atualiza candidate e vai à esquerda.
             * Se for maior, vai à direita sem atualizar candidate.
             */
            if (node->req.id < current->req.id) {
                candidate = current;
                current   = current->left;
            } else if (node->req.id > current->req.id) {
                current = current->right;
            } else {
                /* nó exato encontrado — para a busca */
                break;
            }
        }
    }

    return candidate;   /* NULL se node é o maior elemento */
}