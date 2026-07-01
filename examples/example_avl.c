/**
 * @file    examples/example_avl.c
 * @brief   Teste visual de todas as operações da árvore AVL.
 *
 * Cobre, em ordem:
 *   1. Inserção de 7 requisições com cilindros variados
 *   2. Busca por cilindro existente
 *   3. Predecessor do nó encontrado
 *   4. Sucessor    do nó encontrado
 *   5. Remoção do nó
 *   6. Busca após remoção (confirmação)
 *
 * Cada etapa imprime PASSOU ou FALHOU para facilitar a leitura.
 *
 * Padrão: C11
 */

#include <stdio.h>    /* printf  */
#include <stdint.h>   /* uint32_t */

#include "../include/request.h"
#include "../include/avl.h"

/* ------------------------------------------------------------------ */
/*  Helpers de impressão                                               */
/* ------------------------------------------------------------------ */

/** Separador visual entre seções do teste. */
static void print_separator(void)
{
    printf("--------------------------------------------------\n");
}

/** Imprime o resultado de um teste com alinhamento fixo. */
static void print_result(const char *label, int passed)
{
    printf("  %-40s %s\n", label, passed ? "[ PASSOU ]" : "[ FALHOU ]");
}

/**
 * Imprime os campos relevantes de um AVLNode de forma compacta.
 * Usado para exibir predecessor e sucessor sem poluir a saída.
 */
static void print_node(const char *prefix, const AVLNode *node)
{
    if (node == NULL) {
        printf("  %s → (nulo)\n", prefix);
    } else {
        printf("  %s → cylinder=%-4u  id=%-3u  height=%d\n",
               prefix,
               node->req.cylinder,
               node->req.id,
               node->height);
    }
}

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */

int main(void)
{
    printf("\n");
    printf("==================================================\n");
    printf("  Teste da Arvore AVL — DiskScheduler             \n");
    printf("==================================================\n\n");

    /* ---------------------------------------------------------- */
    /*  1. Criar as Requests                                       */
    /* ---------------------------------------------------------- */

    print_separator();
    printf("  [1] Criando requisicoes\n");
    print_separator();

    /*
     * Cilindros a inserir: 50, 20, 70, 10, 30, 60, 80
     * IDs atribuídos sequencialmente a partir de 1.
     * Os demais campos (process_id, arrival_time, etc.) são
     * valores de exemplo — o teste foca na AVL, não na Request.
     */
    uint32_t cylinders[] = { 50, 20, 70, 10, 30, 60, 80 };
    uint32_t n           = sizeof(cylinders) / sizeof(cylinders[0]);

    Request reqs[7];
    for (uint32_t i = 0; i < n; i++) {
        reqs[i] = request_create(
            i + 1,          /* id             */
            cylinders[i],   /* cylinder       */
            100,            /* process_id     */
            0,              /* arrival_time   */
            OP_READ,        /* operation      */
            0,              /* deadline       */
            0               /* priority       */
        );

        int valid = request_is_valid(&reqs[i]);
        char label[64];
        snprintf(label, sizeof(label),
                 "request_create(cylinder=%u)", cylinders[i]);
        print_result(label, valid);
    }

    /* ---------------------------------------------------------- */
    /*  2. Inserir na AVL                                          */
    /* ---------------------------------------------------------- */

    printf("\n");
    print_separator();
    printf("  [2] Inserindo na AVL\n");
    print_separator();

    AVLNode *root = NULL;

    for (uint32_t i = 0; i < n; i++) {
        AVLNode *prev_root = root;
        root = avl_insert(root, reqs[i]);

        int ok = (root != NULL);

        /* Se a raiz era NULL antes e continua NULL, houve falha. */
        if (prev_root == NULL && root == NULL) ok = 0;

        char label[64];
        snprintf(label, sizeof(label),
                 "avl_insert(cylinder=%u)", reqs[i].cylinder);
        print_result(label, ok);
    }

    printf("\n  Altura da raiz apos insercoes: %d\n", avl_height(root));
    printf("  Fator de balanceamento da raiz: %d\n",
           avl_balance_factor(root));

    /* ---------------------------------------------------------- */
    /*  3. Buscar o cilindro 30                                    */
    /* ---------------------------------------------------------- */

    printf("\n");
    print_separator();
    printf("  [3] Buscando cilindro 30\n");
    print_separator();

    /*
     * O id da Request com cylinder=30 foi atribuído como 5
     * (quinta inserção na sequência 50, 20, 70, 10, 30).
     */
    AVLNode *node30 = avl_search(root, 30, 5);

    print_result("avl_search(cylinder=30, id=5) != NULL", node30 != NULL);

    if (node30 != NULL) {
        print_result("node30->req.cylinder == 30",
                     node30->req.cylinder == 30);
        print_result("node30->req.id       == 5",
                     node30->req.id == 5);
        printf("\n  Detalhes do no encontrado:\n");
        print_node("node30", node30);
    }

    /* ---------------------------------------------------------- */
    /*  4. Predecessor de 30                                       */
    /* ---------------------------------------------------------- */

    printf("\n");
    print_separator();
    printf("  [4] Predecessor de 30  (esperado: cylinder=20)\n");
    print_separator();

    AVLNode *pred = NULL;

    if (node30 != NULL) {
        pred = avl_predecessor(root, node30);
    }

    print_result("avl_predecessor(node30) != NULL", pred != NULL);

    if (pred != NULL) {
        print_result("pred->req.cylinder == 20",
                     pred->req.cylinder == 20);
        printf("\n  Detalhes do predecessor:\n");
        print_node("predecessor", pred);
    }

    /* ---------------------------------------------------------- */
    /*  5. Sucessor de 30                                          */
    /* ---------------------------------------------------------- */

    printf("\n");
    print_separator();
    printf("  [5] Sucessor de 30  (esperado: cylinder=50)\n");
    print_separator();

    AVLNode *succ = NULL;

    if (node30 != NULL) {
        succ = avl_successor(root, node30);
    }

    print_result("avl_successor(node30) != NULL", succ != NULL);

    if (succ != NULL) {
        print_result("succ->req.cylinder == 50",
                     succ->req.cylinder == 50);
        printf("\n  Detalhes do sucessor:\n");
        print_node("sucessor", succ);
    }

    /* ---------------------------------------------------------- */
    /*  6. Remover o cilindro 30                                   */
    /* ---------------------------------------------------------- */

    printf("\n");
    print_separator();
    printf("  [6] Removendo cilindro 30\n");
    print_separator();

    AVLNode *root_antes = root;
    root = avl_remove(root, 30, 5);

    /*
     * avl_remove() retorna a nova raiz (pode ser diferente da
     * anterior após rebalanceamento). Sucesso = não retornou NULL
     * (a menos que a árvore tenha ficado vazia, o que não é o caso).
     */
    print_result("avl_remove(cylinder=30, id=5) — raiz != NULL",
                 root != NULL);
    print_result("avl_remove alterou a arvore",
                 root != root_antes || avl_search(root, 30, 5) == NULL);

    printf("\n  Altura da raiz apos remocao: %d\n", avl_height(root));
    printf("  Fator de balanceamento da raiz: %d\n",
           avl_balance_factor(root));

    /* ---------------------------------------------------------- */
    /*  7. Buscar cilindro 30 após remoção                         */
    /* ---------------------------------------------------------- */

    printf("\n");
    print_separator();
    printf("  [7] Buscando cilindro 30 apos remocao\n");
    print_separator();

    AVLNode *ghost = avl_search(root, 30, 5);

    print_result("avl_search(cylinder=30) == NULL (confirmado removido)",
                 ghost == NULL);

    /* ---------------------------------------------------------- */
    /*  Verificações extras: vizinhos após remoção                 */
    /* ---------------------------------------------------------- */

    printf("\n");
    print_separator();
    printf("  [+] Verificacoes extras apos remocao de 30\n");
    print_separator();

    /*
     * Com 30 removido, o predecessor de 50 deve continuar sendo 20
     * e o sucessor de 20 deve passar a ser 50 diretamente.
     */
    AVLNode *node50 = avl_search(root, 50, 1);
    AVLNode *node20 = avl_search(root, 20, 2);

    if (node50 != NULL) {
        AVLNode *pred50 = avl_predecessor(root, node50);
        print_result("predecessor(50) == 20 (sem o 30)",
                     pred50 != NULL && pred50->req.cylinder == 20);
        print_node("predecessor(50)", pred50);
    }

    if (node20 != NULL) {
        AVLNode *succ20 = avl_successor(root, node20);
        print_result("sucessor(20) == 50 (sem o 30)",
                     succ20 != NULL && succ20->req.cylinder == 50);
        print_node("sucessor(20)", succ20);
    }

    /* ---------------------------------------------------------- */
    /*  Resumo final                                               */
    /* ---------------------------------------------------------- */

    printf("\n");
    printf("==================================================\n");
    printf("  Teste concluido.                                \n");
    printf("==================================================\n\n");

    return 0;
}