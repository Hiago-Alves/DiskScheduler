/**
 * @file    scheduler.h
 * @brief   Infraestrutura do escalonador de disco e algoritmos de
 *          escalonamento.
 *
 * Este módulo define a estrutura central do simulador: o Scheduler.
 * Ele agrupa o estado do disco (Disk) e a fila de requisições pendentes
 * (AVL ordenada por cylinder), oferecendo operações de ciclo de vida,
 * manipulação de requisições e, a partir deste arquivo, os próprios
 * algoritmos de escalonamento.
 *
 * Responsabilidades deste módulo:
 *   - Manter a raiz da AVL com todas as requisições pendentes
 *   - Manter o estado atualizado do disco (via struct Disk)
 *   - Controlar o contador de requisições na fila
 *   - Expor operações de inserção e remoção de requisições
 *   - Expor impressão de estatísticas do estado atual
 *   - Expor os algoritmos de escalonamento (FCFS, e futuramente
 *     SSTF, SCAN, C-SCAN, EDF, Aging etc.)
 *
 * IMPORTANTE — por que a AVL não basta sozinha para o FCFS:
 * A árvore AVL do Scheduler é ordenada pelo campo `cylinder` de cada
 * Request (ver avl.h), pois essa é a chave útil para os algoritmos
 * baseados em posição física da cabeça (SSTF, SCAN, C-SCAN). O FCFS,
 * por outro lado, precisa da ORDEM DE CHEGADA (`arrival_time`), que
 * não tem nenhuma relação com `cylinder`. Por isso o algoritmo FCFS
 * não pode simplesmente caminhar a AVL em-ordem: ele precisa primeiro
 * extrair todas as requisições pendentes, reordená-las por
 * `arrival_time` e só então atender uma a uma. Os detalhes dessa
 * extração ficam encapsulados em scheduler.c (funções estáticas),
 * mantendo scheduler.h limpo, com apenas a assinatura pública.
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
/*  Algoritmos de escalonamento                                         */
/* ------------------------------------------------------------------ */
 
/**
 * @brief Executa o algoritmo FCFS (First-Come, First-Served).
 *
 * O FCFS é o mais simples dos algoritmos de escalonamento de disco:
 * as requisições são atendidas EXATAMENTE na ordem em que chegaram
 * à fila, sem nenhuma reordenação baseada em cilindro. Não há
 * otimização de movimento da cabeça — se as requisições chegarem em
 * uma ordem que force a cabeça a "pular" de um lado a outro do disco,
 * o FCFS simplesmente aceita esse custo.
 *
 * Algoritmo, passo a passo:
 *
 *   1. Verifica se o Scheduler é válido e se há requisições
 *      pendentes; se a fila estiver vazia, não há trabalho a fazer
 *      e a função retorna sucesso imediatamente.
 *
 *   2. Extrai TODAS as requisições pendentes da AVL para um vetor
 *      auxiliar, através de uma travessia em-ordem (in-order). Essa
 *      extração é necessária porque a AVL está ordenada por
 *      `cylinder`, não por `arrival_time` — logo, uma simples
 *      travessia em-ordem da árvore NÃO produz a ordem de chegada.
 *
 *   3. Ordena o vetor auxiliar por `arrival_time` (critério de
 *      desempate: `id`, assumindo que ids menores foram atribuídos
 *      primeiro). Após esse passo, o vetor reflete exatamente a
 *      ordem cronológica de chegada das requisições — a ordem que
 *      o FCFS deve respeitar.
 *
 *   4. Percorre o vetor ordenado, do início ao fim, e para cada
 *      requisição:
 *        a. Move a cabeça do disco até o cilindro da requisição
 *           via disk_move_head(), o que atualiza head_position,
 *           direction, current_time e total_seek_distance dentro
 *           da struct Disk.
 *        b. Remove a requisição da fila via scheduler_remove(),
 *           o que atualiza request_count (decrementa) e
 *           total_served (incrementa) dentro da struct Scheduler.
 *
 *   5. Libera o vetor auxiliar e retorna true.
 *
 * Complexidade:
 *   - Extração em-ordem da AVL:        O(n)
 *   - Ordenação por arrival_time:      O(n log n)  [qsort]
 *   - n remoções na AVL (cada O(log n)): O(n log n)
 *   - Total: O(n log n), onde n = número de requisições pendentes.
 *
 * Estado alterado por esta função:
 *   - sched->disk           (via disk_move_head, uma vez por requisição)
 *   - sched->root            (via scheduler_remove, esvaziado ao final)
 *   - sched->request_count  (decrementado até chegar a 0)
 *   - sched->total_served   (incrementado uma vez por requisição atendida)
 *
 * @param  sched  Ponteiro para o Scheduler. Não deve ser NULL.
 * @return true   se a simulação FCFS foi executada com sucesso
 *                (inclui o caso de fila vazia, que não é um erro).
 *         false  se sched for NULL, ou se a alocação do vetor
 *                auxiliar de extração falhar (malloc retornou NULL).
 *                Nesse caso a fila permanece intacta, sem nenhuma
 *                requisição atendida.
 */
bool scheduler_fcfs(Scheduler *sched);

/**
 * @brief Executa o algoritmo SSTF (Shortest Seek Time First).
 *
 * O SSTF seleciona a requisição pendente com a menor distância de seek
 * em relação à posição atual da cabeça do disco. A cada iteração, a cabeça
 * se move para o cilindro da requisição mais próxima, atendendo-a e
 * removendo-a da fila. Esse processo é repetido até que a fila se esvazie.
 *
 * Algoritmo, passo a passo:
 *
 *   1. Enquanto houver requisições pendentes:
 *        a. Determina o predecessor e o sucessor da posição atual da cabeça
 *           na AVL (utilizando avl_predecessor() e avl_successor() com um
 *           nó fictício).
 *        b. Se apenas um dos dois existir, escolhe esse.
 *        c. Caso contrário, calcula a distância absoluta da cabeça até cada
 *           um e escolhe o que tem a menor distância.
 *        d. Em caso de empate nas distâncias, escolhe o de menor cylinder.
 *        e. Move a cabeça até o cilindro escolhido via disk_move_head().
 *        f. Remove a requisição escolhida da fila via scheduler_remove().
 *
 *   2. A fila estará vazia ao final.
 *
 * Complexidade:
 *   - Cada iteração realiza duas buscas na AVL (predecessor/sucessor) e uma
 *     remoção, todas O(log n). Portanto, O(n log n) para n requisições.
 *
 * Estado alterado:
 *   - sched->disk           (via disk_move_head, uma vez por requisição)
 *   - sched->root            (via scheduler_remove, esvaziado ao final)
 *   - sched->request_count  (decrementado até chegar a 0)
 *   - sched->total_served   (incrementado uma vez por requisição atendida)
 *
 * @param  sched  Ponteiro para o Scheduler. Não deve ser NULL.
 * @return true   se a simulação SSTF foi executada com sucesso
 *                (inclui o caso de fila vazia, que não é um erro).
 *         false  se sched for NULL (não há alocação de memória nesta
 *                função, portanto não há falha de alocação a ser reportada).
 */
bool scheduler_sstf(Scheduler *sched);
 
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