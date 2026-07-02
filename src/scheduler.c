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

#ifndef CONFIG_DEADLINE_LIMIT
#define CONFIG_DEADLINE_LIMIT 50
#endif
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
/**
 * @brief Percorre a AVL em ordem e retorna a primeira requisição expirada.
 *
 * Uma requisição é considerada expirada se:
 *   current_time - req.arrival_time >= CONFIG_DEADLINE_LIMIT
 *
 * A busca é feita em ordem (esquerda, nó, direita) para priorizar cilindros
 * menores, mas qualquer ordem serve; o critério é apenas encontrar uma.
 *
 * @param  root          Raiz da subárvore (pode ser NULL).
 * @param  current_time  Tempo atual da simulação.
 * @return Ponteiro para o nó expirado, ou NULL se nenhum for encontrado.
 */
static AVLNode *scheduler_find_expired(const AVLNode *root, uint64_t current_time)
{
    if (root == NULL) {
        return NULL;
    }

    /* Primeiro, verifica subárvore esquerda (cilindros menores) */
    AVLNode *left = scheduler_find_expired(root->left, current_time);
    if (left != NULL) {
        return left;
    }

    /* Verifica o nó atual */
    if (current_time - root->req.arrival_time >= CONFIG_DEADLINE_LIMIT) {
        return (AVLNode *)root;   /* Cast para remover const – retornamos apenas para leitura */
    }

    /* Depois, subárvore direita (cilindros maiores) */
    return scheduler_find_expired(root->right, current_time);
}
/**
 * @brief Travessia em-ordem da AVL que copia cada Request para um vetor.
 *
 * Esta função existe exclusivamente para dar suporte ao FCFS (e, no
 * futuro, a qualquer outro algoritmo que precise processar "todas as
 * requisições pendentes" fora da ordem de cylinder imposta pela AVL).
 *
 * Por que uma travessia em-ordem e não pré-ordem ou pós-ordem?
 * Não importa, na verdade: a ORDEM em que os elementos são copiados
 * para `buffer` é irrelevante para o FCFS, pois o vetor inteiro será
 * reordenado por arrival_time logo em seguida (ver scheduler_fcfs).
 * Usamos em-ordem apenas por ser a travessia mais natural e didática
 * de uma BST/AVL — o mesmo padrão que avl_min_node()/avl_search() já
 * usam implicitamente ao decidir para qual lado descer.
 *
 * Pré-condição fundamental: `buffer` deve ter espaço para pelo menos
 * `sched->request_count` elementos — é responsabilidade de quem chama
 * (scheduler_fcfs) garantir isso alocando o vetor com o tamanho certo
 * antes de iniciar a travessia.
 *
 * @param  node    Raiz da subárvore atual (pode ser NULL — caso base
 *                  da recursão, não faz nada).
 * @param  buffer  Vetor de destino onde cada Request será copiada.
 * @param  count   Ponteiro para um contador externo que indica a
 *                  próxima posição livre em `buffer`. É incrementado
 *                  a cada Request copiada, permitindo que a função
 *                  saiba onde escrever mesmo entre chamadas
 *                  recursivas (esquerda, nó atual, direita).
 */
static void scheduler_collect_inorder(const AVLNode *node,
                                       Request *buffer,
                                       uint32_t *count)
{
    /* Caso base: subárvore vazia — nada a copiar. */
    if (node == NULL) {
        return;
    }

    /* 1) Visita primeiro toda a subárvore esquerda. */
    scheduler_collect_inorder(node->left, buffer, count);

    /* 2) Copia a Request do nó atual para a próxima posição livre.
     *    A cópia é por valor (struct Request inteira), assim como já
     *    é feito em avl_create_node() — não há ponteiros pendentes. */
    buffer[*count] = node->req;
    (*count)++;

    /* 3) Por fim, visita toda a subárvore direita. */
    scheduler_collect_inorder(node->right, buffer, count);
}

/**
 * @brief Função de comparação usada por qsort() para ordenar
 *        requisições pela ordem cronológica de chegada (FCFS).
 *
 * Critério principal: arrival_time crescente — quem chegou primeiro
 * é atendido primeiro, que é exatamente a definição do FCFS.
 *
 * Critério de desempate: id crescente. Duas requisições podem ter o
 * mesmo arrival_time (por exemplo, se o gerador de carga admitir
 * múltiplas chegadas no mesmo tick simulado); nesse caso, usamos o id
 * — atribuído sequencialmente no momento da criação da requisição —
 * como critério de desempate estável e determinístico.
 *
 * Assinatura compatível com qsort(): recebe dois `const void *`,
 * que internamente são reinterpretados como `const Request *`.
 * Retorna:
 *   < 0  se a  deve vir antes de b
 *   > 0  se a  deve vir depois de b
 *   0    se são equivalentes para fins de ordenação
 *
 * @param  a  Ponteiro genérico para a primeira Request.
 * @param  b  Ponteiro genérico para a segunda Request.
 * @return Resultado da comparação, no formato exigido por qsort().
 */
static int scheduler_compare_by_arrival(const void *a, const void *b)
{
    const Request *req_a = (const Request *)a;
    const Request *req_b = (const Request *)b;

    /* Critério principal: quem chegou antes vem primeiro. */
    if (req_a->arrival_time < req_b->arrival_time) {
        return -1;
    }
    if (req_a->arrival_time > req_b->arrival_time) {
        return 1;
    }

    /* Empate em arrival_time: desempata pelo id (ordem de criação). */
    if (req_a->id < req_b->id) {
        return -1;
    }
    if (req_a->id > req_b->id) {
        return 1;
    }

    /* Praticamente impossível (ids são únicos), mas mantido por
     * completude: se tudo é igual, a ordem entre elas não importa. */
    return 0;
}

/**
 * @brief Calcula o valor absoluto da diferença entre dois uint32_t.
 *
 * Necessário para evitar underflow na subtração.
 *
 * @param  a  Primeiro valor.
 * @param  b  Segundo valor.
 * @return |a - b| como uint32_t.
 */
static uint32_t uint32_abs_diff(uint32_t a, uint32_t b)
{
    return (a >= b) ? (a - b) : (b - a);
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
/*  Algoritmos de escalonamento                                         */
/* ------------------------------------------------------------------ */

/**
 * Implementação do FCFS descrita em detalhe no Doxygen de scheduler.h.
 * Aqui comentamos apenas as decisões de implementação que não cabem
 * na documentação de interface (por serem detalhes internos).
 */
bool scheduler_fcfs(Scheduler *sched)
{
    /* Guarda de nulidade: sem Scheduler, não há o que fazer. */
    if (sched == NULL) {
        return false;
    }

    /* Fila vazia não é um erro — é apenas um trabalho trivialmente
     * concluído. Retornamos sucesso sem alocar nada. */
    if (sched->request_count == 0) {
        return true;
    }

    /* --------------------------------------------------------------
     * PASSO 1 — Extrair todas as requisições pendentes da AVL.
     *
     * Alocamos um vetor com espaço exato para `request_count`
     * requisições. Usamos sched->request_count (mantido sempre
     * sincronizado com o número real de nós da AVL por scheduler_add()
     * e scheduler_remove()) em vez de contar os nós manualmente,
     * evitando uma travessia extra só para descobrir o tamanho.
     * ------------------------------------------------------------ */
    uint32_t total = sched->request_count;

    Request *pending = malloc(sizeof(Request) * (size_t)total);
    if (pending == NULL) {
        /* Falha de alocação: nada foi alterado no Scheduler até aqui,
         * então basta reportar o erro ao chamador. */
        return false;
    }

    /* O contador começa em 0 e é incrementado dentro da travessia
     * recursiva, uma vez para cada Request copiada. Ao final da
     * chamada, `collected` deve ser igual a `total`. */
    uint32_t collected = 0;
    scheduler_collect_inorder(sched->root, pending, &collected);

    /* --------------------------------------------------------------
     * PASSO 2 — Ordenar o vetor por ordem de chegada (arrival_time).
     *
     * Esta é a etapa que efetivamente transforma "requisições
     * ordenadas por cylinder" em "requisições ordenadas por
     * chegada" — a essência do FCFS.
     * ------------------------------------------------------------ */
    qsort(pending, total, sizeof(Request), scheduler_compare_by_arrival);

    /* --------------------------------------------------------------
     * PASSO 3 — Atender as requisições, uma a uma, na ordem de
     * chegada, exatamente como o vetor `pending` está ordenado agora.
     * ------------------------------------------------------------ */
    for (uint32_t i = 0; i < total; i++) {

        const Request *current = &pending[i];

        /* Move a cabeça do disco até o cilindro desta requisição.
         * disk_move_head() atualiza, dentro de sched->disk:
         *   - head_position       (novo cilindro)
         *   - direction           (sentido do movimento)
         *   - current_time        (+= seek + rotação + transferência)
         *   - total_seek_distance (+= distância percorrida)
         *
         * disk_move_head() retorna false apenas se `disk` for NULL
         * ou se `cylinder` estiver fora de [0, CONFIG_CYLINDERS).
         * Como toda Request extraída veio de dentro da própria AVL
         * do Scheduler (ou seja, já foi aceita por scheduler_add()),
         * este caso não deveria ocorrer em uso normal. Ainda assim,
         * tratamos defensivamente: se o movimento falhar, pulamos a
         * remoção desta requisição específica (ela permanece na fila
         * para uma tentativa futura) e seguimos para a próxima,
         * em vez de interromper toda a simulação por causa de um
         * único dado inconsistente. */
        if (!disk_move_head(&sched->disk, current->cylinder)) {
            continue;
        }

        /* Remove a requisição da AVL agora que foi efetivamente
         * atendida. scheduler_remove() decrementa request_count e
         * incrementa total_served internamente. */
        scheduler_remove(sched, current->cylinder, current->id);
    }

    /* --------------------------------------------------------------
     * PASSO 4 — Liberar o vetor auxiliar.
     *
     * `pending` foi alocado apenas para esta execução do algoritmo;
     * o estado permanente do Scheduler continua vivendo na AVL
     * (sched->root), que já foi devidamente esvaziada pelas chamadas
     * a scheduler_remove() dentro do laço acima.
     * ------------------------------------------------------------ */
    free(pending);

    return true;
}

/**
 * Implementação do SSTF descrita em detalhe no Doxygen de scheduler.h.
 */
bool scheduler_sstf(Scheduler *sched)
{
    if (sched == NULL) {
        return false;
    }

    while (sched->request_count > 0) {
        uint32_t head = sched->disk.head_position;

        /* Cria um nó fictício para usar com avl_predecessor e avl_successor.
         * O id é definido como 0, assumindo que ids de requisição começam em 1;
         * isso garante que o fictício seja considerado menor que qualquer
         * requisição existente para o mesmo cylinder, fazendo com que
         * avl_successor encontre o nó com menor id entre os de cylinder igual,
         * e avl_predecessor tenda a NULL ou a um predecessor menor. */
        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = NULL;
        ficticio.right = NULL;
        ficticio.height = 1;

        AVLNode *pred = avl_predecessor(sched->root, &ficticio);
        AVLNode *succ = avl_successor(sched->root, &ficticio);

        AVLNode *chosen = NULL;

        /* Seleciona o nó mais próximo */
        if (pred == NULL && succ == NULL) {
            /* Inconsistência: fila não vazia mas sem pred/succ? Não deve ocorrer. */
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
                /* Empate: escolhe o menor cylinder */
                chosen = (pred->req.cylinder <= succ->req.cylinder) ? pred : succ;
            }
        }

        /* Move a cabeça para o cilindro escolhido */
        if (!disk_move_head(&sched->disk, chosen->req.cylinder)) {
            /* Falha inesperada: cilindro inválido; interrompe para evitar loop */
            break;
        }

        /* Remove a requisição atendida */
        scheduler_remove(sched, chosen->req.cylinder, chosen->req.id);
    }

    return true;
}

/**
 * Implementação do SCAN descrita em detalhe no Doxygen de scheduler.h.
 */
bool scheduler_scan(Scheduler *sched)
{
    if (sched == NULL) {
        return false;
    }

    while (sched->request_count > 0) {
        uint32_t head = sched->disk.head_position;
        DiskDirection dir = sched->disk.direction;

        /* Cria um nó fictício para usar com avl_successor e avl_predecessor.
         * O id é definido como 0 (menor que qualquer id real) para que,
         * no caso de cylinder igual a head, tanto o sucessor quanto o
         * predecessor funcionem corretamente. */
        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = NULL;
        ficticio.right = NULL;
        ficticio.height = 1;

        AVLNode *chosen = NULL;

        if (dir == DIR_RIGHT) {
            /* Procura a primeira requisição com cylinder >= head_position */
            chosen = avl_successor(sched->root, &ficticio);

            /* Se não houver requisição à frente, inverte a direção */
            if (chosen == NULL) {
                sched->disk.direction = DIR_LEFT;
                /* Continua o laço para tentar novamente com a nova direção */
                continue;
            }
        } else { /* DIR_LEFT */
            /* Procura a primeira requisição com cylinder <= head_position */
            chosen = avl_predecessor(sched->root, &ficticio);

            /* Se não houver requisição atrás, inverte a direção */
            if (chosen == NULL) {
                sched->disk.direction = DIR_RIGHT;
                /* Continua o laço para tentar novamente com a nova direção */
                continue;
            }
        }

        /* Move a cabeça para o cilindro escolhido e remove a requisição */
        if (!disk_move_head(&sched->disk, chosen->req.cylinder)) {
            /* Falha inesperada: cilindro inválido; interrompe para evitar loop */
            break;
        }
        scheduler_remove(sched, chosen->req.cylinder, chosen->req.id);
    }

    return true;
}

/**
 * Implementação do C-SCAN descrita em detalhe no Doxygen de scheduler.h.
 */
bool scheduler_cscan(Scheduler *sched)
{
    if (sched == NULL) {
        return false;
    }

    /* C-SCAN sempre se move para a direita; garante que a direção esteja
     * configurada corretamente, embora não seja estritamente necessária. */
    sched->disk.direction = DIR_RIGHT;

    while (sched->request_count > 0) {
        uint32_t head = sched->disk.head_position;

        /* Cria nó fictício para sucessor */
        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = NULL;
        ficticio.right = NULL;
        ficticio.height = 1;

        /* Procura o sucessor (primeira requisição com cylinder >= head) */
        AVLNode *succ = avl_successor(sched->root, &ficticio);

        if (succ != NULL) {
            /* Existe requisição à frente: atende normalmente */
            if (!disk_move_head(&sched->disk, succ->req.cylinder)) {
                break;
            }
            scheduler_remove(sched, succ->req.cylinder, succ->req.id);
        } else {
            /*
             * Não há requisições à frente: faz o "salto circular"
             * 1) Move para o cilindro máximo (CONFIG_CYLINDERS - 1)
             * 2) Move para o cilindro 0
             *
             * Essas movimentações não removem requisições; apenas reposicionam
             * a cabeça para o início do disco, mantendo a métrica de distância
             * correta. A direção permanece DIR_RIGHT.
             */
            uint32_t max_cylinder = CONFIG_CYLINDERS - 1;

            /* Move para o extremo direito */
            if (!disk_move_head(&sched->disk, max_cylinder)) {
                break;
            }
            /* Move para o cilindro 0 */
            if (!disk_move_head(&sched->disk, 0)) {
                break;
            }

            /* Após o salto, a cabeça está em 0 e o laço continua,
             * encontrando agora as requisições que estavam no início. */
        }
    }

    return true;
}
/**
 * @brief Implementação do algoritmo DEADLINE.
 *
 * Conforme descrito em scheduler.h.
 */
bool scheduler_deadline(Scheduler *sched)
{
    if (sched == NULL) {
        return false;
    }

    while (sched->request_count > 0) {
        uint64_t current_time = sched->disk.current_time;

        /* 1) Verifica se há alguma requisição expirada */
        AVLNode *expired = scheduler_find_expired(sched->root, current_time);

        if (expired != NULL) {
            /* Atende a requisição expirada imediatamente */
            if (!disk_move_head(&sched->disk, expired->req.cylinder)) {
                break;
            }
            scheduler_remove(sched, expired->req.cylinder, expired->req.id);
            continue;   /* Volta ao início do loop para verificar novamente */
        }

        /*
         * 2) Nenhuma expirada: executa exatamente a lógica do SSTF
         *    (código copiado da função scheduler_sstf).
         */
        uint32_t head = sched->disk.head_position;

        /* Nó fictício para buscar predecessor e sucessor */
        AVLNode ficticio;
        ficticio.req.cylinder = head;
        ficticio.req.id = 0;
        ficticio.left = NULL;
        ficticio.right = NULL;
        ficticio.height = 1;

        AVLNode *pred = avl_predecessor(sched->root, &ficticio);
        AVLNode *succ = avl_successor(sched->root, &ficticio);
        AVLNode *chosen = NULL;

        /* Seleciona o nó mais próximo */
        if (pred == NULL && succ == NULL) {
            break;   /* Inconsistência – não deve ocorrer */
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
                /* Empate: escolhe o menor cylinder */
                chosen = (pred->req.cylinder <= succ->req.cylinder) ? pred : succ;
            }
        }

        /* Move a cabeça e remove a requisição */
        if (!disk_move_head(&sched->disk, chosen->req.cylinder)) {
            break;
        }
        scheduler_remove(sched, chosen->req.cylinder, chosen->req.id);
    }

    return true;
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