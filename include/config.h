/**
 * @file    config.h
 * @brief   Constantes de configuração do simulador de disco.
 *
 * Centraliza todos os parâmetros físicos e de simulação em um único
 * lugar. Alterar um valor aqui afeta todo o projeto de forma consistente,
 * sem necessidade de busca e substituição espalhada pelo código.
 *
 * Os valores padrão simulam um disco rígido magnético de entrada,
 * compatível com HDDs de 7200 RPM amplamente usados como referência
 * em literatura de Sistemas Operacionais.
 *
 * Padrão: C11
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ------------------------------------------------------------------ */
/*  Geometria do disco                                                  */
/* ------------------------------------------------------------------ */

/**
 * @brief Número total de cilindros do disco simulado.
 *
 * Define o espaço de endereçamento: cilindros válidos são [0, 1023].
 * Deve ser idêntico a DISK_MAX_CYLINDERS em request.h.
 */
#define CONFIG_CYLINDERS        1024u

/**
 * @brief Limite de tempo de espera para considerar uma requisição "expirada".
 *
 * Se uma requisição esperar por este valor (em ticks) ou mais,
 * ela será atendida com prioridade máxima pelo algoritmo DEADLINE.
 */
#define CONFIG_DEADLINE_LIMIT 50

/**
 * @brief Posição inicial da cabeça de leitura/escrita ao ligar o disco.
 *
 * Representa o cilindro onde a cabeça repousa antes da simulação.
 * Valor típico: 0 (borda externa) ou metade do disco.
 */
#define CONFIG_INITIAL_HEAD     0u

/* ------------------------------------------------------------------ */
/*  Parâmetros de tempo (em microsegundos — us)                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Tempo de seek por cilindro percorrido (em microsegundos).
 *
 * Cada cilindro de distância adiciona este valor ao tempo de seek.
 * HDDs reais: ~0,3 ms por trilha ≈ 300 us. Usamos 1 us para
 * simplificar a visualização dos resultados na simulação.
 */
#define CONFIG_SEEK_TIME_PER_CYLINDER   1u

/**
 * @brief Latência rotacional média (em microsegundos).
 *
 * Tempo médio para o setor correto girar até a cabeça após o seek.
 * Para um disco de 7200 RPM: 1 rotação = ~8,3 ms → média = ~4,15 ms.
 * Aqui usamos 4000 us como aproximação inteira.
 */
#define CONFIG_ROTATIONAL_LATENCY_US    4000u

/**
 * @brief Tempo de transferência por operação (em microsegundos).
 *
 * Tempo para ler ou gravar um setor após o posicionamento.
 * Valor típico para HDD de 7200 RPM: ~500 us por setor.
 */
#define CONFIG_TRANSFER_TIME_US         500u

/* ------------------------------------------------------------------ */
/*  Configurações do simulador                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief Tempo inicial do simulador (em ticks).
 *
 * A simulação começa neste instante. Normalmente 0, mas pode ser
 * ajustado para testes que partem de um estado intermediário.
 */
#define CONFIG_INITIAL_TIME     0u

#endif /* CONFIG_H */