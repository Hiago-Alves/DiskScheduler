/**
 * @file    simulation.h
 * @brief   Camada de controle da simulação do escalonador de disco.
 *
 * Este módulo coordena a execução da simulação, integrando o Scheduler,
 * a geração de requisições e a coleta de estatísticas. Oferece uma
 * interface simples para configurar, executar e analisar uma simulação
 * completa do escalonador de disco.
 *
 * Padrão: C11
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdint.h>   /* uint32_t, uint64_t */
#include <stdbool.h>  /* bool               */
#include "scheduler.h"
#include "config.h"

/* ------------------------------------------------------------------ */
/*  Tipos auxiliares                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Algoritmos de escalonamento de disco suportados.
 */
typedef enum {
    ALG_FCFS  = 0,   /**< First-Come, First-Served                     */
    ALG_SSTF  = 1,   /**< Shortest Seek Time First                     */
    ALG_SCAN  = 2,   /**< SCAN (Elevator)                             */
    ALG_CSCAN = 3    /**< Circular SCAN                               */
} SchedulingAlgorithm;

/* ------------------------------------------------------------------ */
/*  Estrutura principal                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Estado completo de uma simulação.
 *
 * Contém o Scheduler (com disco e fila), o algoritmo selecionado,
 * o número total de requisições geradas e métricas de desempenho.
 */
typedef struct {
    /**
     * @brief Scheduler que gerencia a fila e o disco.
     */
    Scheduler scheduler;

    /**
     * @brief Algoritmo de escalonamento selecionado para esta simulação.
     */
    SchedulingAlgorithm algorithm;

    /**
     * @brief Número total de requisições geradas na simulação.
     */
    uint32_t total_requests;

    /**
     * @brief Tempo total da simulação (em microsegundos).
     *
     * Corresponde a sched->disk.current_time ao final da execução.
     */
    uint64_t total_time;

    /**
     * @brief Distância total percorrida pela cabeça (em cilindros).
     *
     * Corresponde a sched->disk.total_seek_distance ao final.
     */
    uint64_t total_seek_distance;

    /**
     * @brief Posição final da cabeça.
     */
    uint32_t final_head_position;

    /**
     * @brief Número total de requisições atendidas.
     */
    uint64_t total_served;
} Simulation;

/* ------------------------------------------------------------------ */
/*  Protótipos de funções                                               */
/* ------------------------------------------------------------------ */

/**
 * @brief Inicializa a estrutura Simulation com valores padrão.
 *
 * Inicializa internamente o Scheduler via scheduler_init() e zera
 * todas as métricas. O algoritmo padrão é ALG_FCFS.
 *
 * @param sim  Ponteiro para a Simulation a ser inicializada.
 */
void simulation_init(Simulation *sim);

/**
 * @brief Gera um conjunto de requisições aleatórias e as insere no Scheduler.
 *
 * Cada requisição recebe:
 *   - id: sequencial (1, 2, 3, ...)
 *   - cylinder: aleatório entre 0 e CONFIG_CYLINDERS-1
 *   - process_id: aleatório entre 1 e 10
 *   - arrival_time: incremental (a partir de 0, com passo aleatório entre 0 e 50)
 *   - operation: OP_READ ou OP_WRITE (aleatório)
 *   - deadline: 0 (sem prazo)
 *   - priority: 0 (inicial)
 *
 * As requisições são inseridas via scheduler_add().
 *
 * @param sim         Ponteiro para a Simulation.
 * @param count       Número de requisições a gerar.
 * @param seed        Semente para o gerador de números aleatórios.
 * @return true       se todas as requisições foram inseridas com sucesso,
 *         false      se houve falha de alocação ou parâmetros inválidos.
 */
bool simulation_generate_requests(Simulation *sim, uint32_t count, unsigned int seed);

/**
 * @brief Seleciona qual algoritmo de escalonamento será executado.
 *
 * Define o campo `algorithm` da Simulation. Não executa a simulação.
 *
 * @param sim        Ponteiro para a Simulation.
 * @param algorithm  Algoritmo a ser usado (ALG_FCFS, ALG_SSTF, ALG_SCAN, ALG_CSCAN).
 */
void simulation_select_algorithm(Simulation *sim, SchedulingAlgorithm algorithm);

/**
 * @brief Executa a simulação completa.
 *
 * Chama a função correspondente ao algoritmo selecionado em sched->algorithm.
 * Após a execução, atualiza as métricas finais (total_time, total_seek_distance,
 * final_head_position, total_served).
 *
 * @param sim  Ponteiro para a Simulation.
 * @return true   se a simulação foi executada com sucesso,
 *         false  se o Scheduler for NULL ou o algoritmo for inválido.
 */
bool simulation_run(Simulation *sim);

/**
 * @brief Calcula e retorna as estatísticas finais da simulação.
 *
 * As estatísticas são armazenadas nos campos da própria Simulation.
 * Esta função pode ser chamada após simulation_run() para atualizar
 * as métricas caso não tenham sido preenchidas.
 *
 * @param sim  Ponteiro para a Simulation.
 */
void simulation_compute_stats(Simulation *sim);

/**
 * @brief Imprime um relatório organizado da simulação.
 *
 * Exibe:
 *   - Algoritmo utilizado
 *   - Total de requisições atendidas
 *   - Distância total percorrida pela cabeça
 *   - Tempo total de execução
 *   - Posição final da cabeça
 *
 * @param sim  Ponteiro para a Simulation.
 */
void simulation_print_report(const Simulation *sim);

/**
 * @brief Reinicializa a simulação para uma nova execução.
 *
 * Reseta o Scheduler (via scheduler_init()) e zera as métricas,
 * mantendo o mesmo algoritmo selecionado.
 *
 * @param sim  Ponteiro para a Simulation.
 */
void simulation_reset(Simulation *sim);

#endif /* SIMULATION_H */