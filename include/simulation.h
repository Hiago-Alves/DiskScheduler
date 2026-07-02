/**
 * @file    simulation.h
 * @brief   Camada de controle da simulacao do escalonador de disco.
 *
 * Padrao: C11
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdint.h>
#include <stdbool.h>
#include "scheduler.h"

/* ------------------------------------------------------------------ */
/*  Tipos auxiliares                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Algoritmos de escalonamento suportados pela simulacao.
 */
typedef enum {
    ALG_FCFS   = 1,
    ALG_SSTF   = 2,
    ALG_SCAN   = 3,
    ALG_CSCAN  = 4,
    ALG_DEADLINE = 5 
} Algorithm;
/* ------------------------------------------------------------------ */
/*  Estrutura principal                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Estado completo de uma simulacao.
 */
typedef struct {
    /**
     * @brief Escalonador que sera utilizado na simulacao.
     */
    Scheduler scheduler;

    /**
     * @brief Algoritmo selecionado para esta simulacao.
     */
    Algorithm algorithm;

    /**
     * @brief Numero de requisicoes geradas para esta simulacao.
     */
    uint32_t total_requests;

    /**
     * @brief Semente utilizada na geracao aleatoria.
     */
    uint32_t seed;

    /**
     * @brief Flag que indica se a simulacao ja foi executada.
     */
    bool executed;

    /**
     * @brief Posicao inicial da cabeca do disco (cilindro).
     *
     * Este valor e configurado pelo usuario antes da simulacao.
     * Durante simulation_reset(), a cabeca e posicionada neste cilindro.
     */
    uint32_t initial_head_position;

    /**
     * @brief Tempo total de execucao (microssegundos).
     */
    uint64_t total_time_us;

    /**
     * @brief Distancia total percorrida pela cabeca (cilindros).
     */
    uint64_t total_seek_distance;

    /**
     * @brief Posicao final da cabeca (cilindro).
     */
    uint32_t final_head_position;

    /**
     * @brief Numero total de requisicoes atendidas.
     */
    uint64_t total_served;

} Simulation;

/* ------------------------------------------------------------------ */
/*  Funcoes de ciclo de vida                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief Inicializa uma estrutura Simulation com valores padrao.
 *
 * Inicializa o Scheduler interno via scheduler_init() e zera todos os
 * campos de estatisticas. A posicao inicial da cabeca e definida como
 * CONFIG_INITIAL_HEAD (mantendo compatibilidade com o codigo atual).
 *
 * @param sim  Ponteiro para a Simulation a ser inicializada.
 */
void simulation_init(Simulation *sim);

/**
 * @brief Reseta a simulacao, mantendo a mesma estrutura.
 *
 * Remove todas as requisicoes pendentes e reinicializa o estado do disco,
 * posicionando a cabeca no cilindro definido por initial_head_position.
 * Mantem a semente e o algoritmo previamente configurados.
 *
 * @param sim  Ponteiro para a Simulation a ser resetada.
 */
void simulation_reset(Simulation *sim);

/* ------------------------------------------------------------------ */
/*  Configuracao da simulacao                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief Define a posicao inicial da cabeca do disco.
 *
 * Este valor sera utilizado em simulation_reset() para posicionar a cabeca.
 * O valor deve estar no intervalo [0, CONFIG_CYLINDERS - 1].
 * A validacao e responsabilidade do chamador (main.c).
 *
 * @param sim   Ponteiro para a Simulation.
 * @param head  Cilindro onde a cabeca deve iniciar.
 */
void simulation_set_initial_head(Simulation *sim, uint32_t head);

/**
 * @brief Define a semente para a geracao aleatoria.
 *
 * Se seed == 0, a semente sera definida como time(NULL) no momento
 * da geracao (via simulation_generate_requests).
 *
 * @param sim   Ponteiro para a Simulation.
 * @param seed  Semente a ser utilizada.
 */
void simulation_set_seed(Simulation *sim, uint32_t seed);

/**
 * @brief Define qual algoritmo sera executado na simulacao.
 *
 * @param sim        Ponteiro para a Simulation.
 * @param algorithm  Algoritmo a ser utilizado (ALG_FCFS, ALG_SSTF, etc.).
 */
void simulation_select_algorithm(Simulation *sim, Algorithm algorithm);

/* ------------------------------------------------------------------ */
/*  Geracao de requisicoes                                              */
/* ------------------------------------------------------------------ */

/**
 * @brief Gera um conjunto de requisicoes aleatorias e as insere no Scheduler.
 *
 * As requisicoes sao geradas com:
 *   - id: sequencial (1..n)
 *   - cylinder: aleatorio uniforme em [0, CONFIG_CYLINDERS - 1]
 *   - process_id: 1 (fixo)
 *   - arrival_time: sequencial (0, 1, 2, ...)
 *   - operation: OP_READ (fixo)
 *   - deadline: 0 (sem prazo)
 *   - priority: 0 (prioridade inicial)
 *
 * A semente e definida por sim->seed. Se seed == 0, usa time(NULL).
 *
 * @param sim    Ponteiro para a Simulation.
 * @param count  Numero de requisicoes a gerar.
 * @return true  se todas as requisicoes foram inseridas com sucesso.
 *         false se houve falha de alocacao ou sim for NULL.
 */
bool simulation_generate_requests(Simulation *sim, uint32_t count);

/* ------------------------------------------------------------------ */
/*  Execucao                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief Executa a simulacao com o algoritmo previamente selecionado.
 *
 * Apos a execucao, preenche as estatisticas finais na estrutura Simulation.
 *
 * @param sim  Ponteiro para a Simulation (ja com requisicoes inseridas).
 * @return true  se a simulacao foi executada com sucesso.
 *         false se sim for NULL, nao houver requisicoes ou algoritmo invalido.
 */
bool simulation_run(Simulation *sim);

/* ------------------------------------------------------------------ */
/*  Estatisticas e relatorio                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief Retorna o tempo total de execucao da simulacao.
 *
 * @param sim  Ponteiro para a Simulation.
 * @return Tempo total em microssegundos, ou 0 se sim for NULL.
 */
uint64_t simulation_get_total_time(const Simulation *sim);

/**
 * @brief Retorna a distancia total percorrida pela cabeca.
 *
 * @param sim  Ponteiro para a Simulation.
 * @return Distancia total em cilindros, ou 0 se sim for NULL.
 */
uint64_t simulation_get_total_distance(const Simulation *sim);

/**
 * @brief Retorna a posicao final da cabeca do disco.
 *
 * @param sim  Ponteiro para a Simulation.
 * @return Cilindro final, ou 0 se sim for NULL.
 */
uint32_t simulation_get_final_head(const Simulation *sim);

/**
 * @brief Retorna o numero total de requisicoes atendidas.
 *
 * @param sim  Ponteiro para a Simulation.
 * @return Numero de requisicoes atendidas, ou 0 se sim for NULL.
 */
uint64_t simulation_get_total_served(const Simulation *sim);

/**
 * @brief Imprime um relatorio completo da simulacao no stdout.
 *
 * O relatorio inclui:
 *   - Algoritmo utilizado
 *   - Posicao inicial da cabeca
 *   - Numero de requisicoes geradas e atendidas
 *   - Distancia total percorrida
 *   - Tempo total de execucao
 *   - Posicao final da cabeca
 *
 * @param sim  Ponteiro para a Simulation.
 */
void simulation_print_report(const Simulation *sim);

#endif /* SIMULATION_H */