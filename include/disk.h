/**
 * @file    disk.h
 * @brief   Definição da estrutura e funções do disco rígido simulado.
 *
 * Este módulo representa o estado físico do disco durante a simulação:
 * posição da cabeça, direção de movimento e tempo corrente.
 *
 * Responsabilidades deste módulo:
 *   - Manter o estado atualizado do disco (cabeça, direção, tempo)
 *   - Calcular os três componentes de latência de acesso ao disco:
 *       1. Seek time        (tempo de posicionamento radial)
 *       2. Latência rotacional (tempo de espera pelo setor)
 *       3. Tempo de transferência (tempo de leitura/escrita)
 *   - Executar o movimento da cabeça, atualizando o tempo simulado
 *
 * O módulo NÃO decide qual requisição atender — essa é responsabilidade
 * dos algoritmos de escalonamento implementados nos módulos futuros.
 *
 * Padrão: C11
 */

#ifndef DISK_H
#define DISK_H

#include <stdint.h>   /* uint32_t, uint64_t  */
#include <stdbool.h>  /* bool                */
#include "config.h"   /* constantes físicas  */

/* ------------------------------------------------------------------ */
/*  Tipos auxiliares                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Direção de movimento da cabeça de leitura/escrita.
 *
 * Utilizado pelos algoritmos SCAN e C-SCAN para determinar
 * para qual lado a cabeça deve se mover a seguir.
 */
typedef enum {
    DIR_LEFT  = -1,  /**< Movendo-se em direção ao cilindro 0          */
    DIR_RIGHT =  1   /**< Movendo-se em direção ao cilindro máximo      */
} DiskDirection;

/* ------------------------------------------------------------------ */
/*  Estrutura principal                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Representa o estado físico do disco rígido simulado.
 *
 * Todos os campos são atualizados pelo módulo a cada operação.
 * Módulos externos (escalonadores) consultam esses campos para
 * tomar decisões, mas não os modificam diretamente.
 */
typedef struct {

    /**
     * @brief Posição atual da cabeça de leitura/escrita.
     *
     * Sempre no intervalo [0, CONFIG_CYLINDERS - 1].
     * É o ponto de partida para cálculo de seek time de qualquer
     * requisição pendente.
     */
    uint32_t head_position;

    /**
     * @brief Direção atual do movimento da cabeça.
     *
     * DIR_RIGHT: cabeça avançando para cilindros maiores.
     * DIR_LEFT:  cabeça recuando para cilindros menores.
     *
     * Utilizado pelos algoritmos SCAN e C-SCAN. Para FCFS e SSTF,
     * este campo é ignorado pelo escalonador.
     */
    DiskDirection direction;

    /**
     * @brief Tempo acumulado da simulação (em microsegundos).
     *
     * Avança a cada chamada de disk_move_head(), somando o tempo
     * total da operação (seek + rotacional + transferência).
     * Permite calcular: tempo de espera por requisição, violações
     * de deadline (EDF) e métricas gerais de desempenho.
     */
    uint64_t current_time;

    /**
     * @brief Número total de cilindros percorridos durante a simulação.
     *
     * Métrica acumulada usada para comparar eficiência entre
     * algoritmos de escalonamento (menos cilindros = melhor).
     */
    uint64_t total_seek_distance;

} Disk;

/* ------------------------------------------------------------------ */
/*  Protótipos de funções                                               */
/* ------------------------------------------------------------------ */

/**
 * @brief  Inicializa o disco com os parâmetros definidos em config.h.
 *
 * Define a posição inicial da cabeça, direção padrão (DIR_RIGHT),
 * tempo inicial e distância percorrida zerada.
 *
 * @param  disk  Ponteiro para a struct Disk a ser inicializada.
 *
 * @note   Deve ser chamada uma única vez antes de qualquer simulação.
 */
void disk_init(Disk *disk);

/**
 * @brief  Calcula o tempo de seek para mover a cabeça até um cilindro.
 *
 * O seek time é proporcional à distância entre a posição atual da
 * cabeça e o cilindro alvo.
 *
 * Fórmula:
 *   seek_time = |cylinder - head_position| * CONFIG_SEEK_TIME_PER_CYLINDER
 *
 * @param  disk      Ponteiro para o disco (fornece head_position).
 * @param  cylinder  Cilindro de destino.
 * @return Tempo de seek em microsegundos (uint64_t).
 */
uint64_t disk_seek_time(const Disk *disk, uint32_t cylinder);

/**
 * @brief  Retorna a latência rotacional média constante.
 *
 * Em um disco real, a latência rotacional varia entre 0 (setor já
 * posicionado) e o tempo de uma rotação completa. Usamos a média
 * estatística (metade de uma rotação) como aproximação padrão.
 *
 * @return CONFIG_ROTATIONAL_LATENCY_US (constante em microsegundos).
 *
 * @note   Não depende de parâmetros: é sempre a mesma constante.
 *         A assinatura recebe Disk* por consistência da API e
 *         possível extensão futura (ex.: discos com RPM variável).
 */
uint64_t disk_rotational_latency(const Disk *disk);

/**
 * @brief  Retorna o tempo de transferência constante por operação.
 *
 * Representa o tempo necessário para ler ou gravar o setor após
 * o posicionamento radial e rotacional.
 *
 * @return CONFIG_TRANSFER_TIME_US (constante em microsegundos).
 *
 * @note   Mesma justificativa de assinatura que disk_rotational_latency.
 */
uint64_t disk_transfer_time(const Disk *disk);

/**
 * @brief  Calcula o tempo total de acesso para um cilindro alvo.
 *
 * Soma as três fases:
 *   total = seek_time(cylinder) + rotational_latency + transfer_time
 *
 * Esta função é somente leitura: não altera o estado do disco.
 * Use disk_move_head() para efetivar o movimento.
 *
 * @param  disk      Ponteiro para o disco.
 * @param  cylinder  Cilindro de destino.
 * @return Tempo total de acesso em microsegundos.
 */
uint64_t disk_access_time(const Disk *disk, uint32_t cylinder);

/**
 * @brief  Move a cabeça para um cilindro e atualiza o estado do disco.
 *
 * Esta é a única função que modifica o disco. Ela:
 *   1. Valida que o cilindro está dentro dos limites
 *   2. Calcula e acumula a distância de seek em total_seek_distance
 *   3. Soma o tempo total de acesso em current_time
 *   4. Atualiza head_position para o cilindro alvo
 *   5. Atualiza direction conforme o sentido do movimento
 *
 * @param  disk      Ponteiro para o disco (será modificado).
 * @param  cylinder  Cilindro de destino.
 * @return true  se o movimento foi realizado com sucesso.
 *         false se o cilindro for inválido ou disk for NULL.
 */
bool disk_move_head(Disk *disk, uint32_t cylinder);

/**
 * @brief  Imprime o estado atual do disco no stdout.
 *
 * Exibe: posição da cabeça, direção, tempo acumulado e
 * distância total percorrida. Útil para depuração e apresentações.
 *
 * @param  disk  Ponteiro para o disco a ser exibido.
 */
void disk_print_state(const Disk *disk);

#endif /* DISK_H */