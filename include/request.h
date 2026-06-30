/**
 * @file    request.h
 * @brief   Definição da estrutura de requisição de acesso ao disco.
 *
 * Este módulo representa a unidade básica de trabalho do escalonador:
 * uma requisição feita por um processo para acessar um cilindro do disco.
 *
 * Campos presentes:
 *   - id, cylinder, process_id, arrival_time, operation  (base)
 *   - deadline   → usado pelo algoritmo EDF (Earliest Deadline First)
 *   - priority   → usado pelo mecanismo de Aging (anti-starvation)
 *
 * Padrão: C11
 */

#ifndef REQUEST_H
#define REQUEST_H

#include <stdint.h>   /* uint32_t, uint64_t          */
#include <stdbool.h>  /* bool (usado em request_is_valid) */

/* ------------------------------------------------------------------ */
/*  Constantes                                                          */
/* ------------------------------------------------------------------ */

/** Número máximo de cilindros suportados pelo disco simulado. */
#define DISK_MAX_CYLINDERS 1024u

/* ------------------------------------------------------------------ */
/*  Tipos auxiliares                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Tipo da operação solicitada ao disco.
 *
 * Usar enum garante legibilidade e evita "números mágicos" no código.
 */
typedef enum {
    OP_READ  = 0,   /**< Operação de leitura  */
    OP_WRITE = 1    /**< Operação de escrita  */
} OperationType;

/* ------------------------------------------------------------------ */
/*  Estrutura principal                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Representa uma requisição de acesso a um cilindro do disco.
 *
 * Cada instância desta struct descreve completamente uma solicitação
 * de I/O feita por um processo ao disco rígido.
 */
typedef struct {

    /**
     * @brief Identificador único desta requisição.
     *
     * Atribuído sequencialmente pelo sistema no momento da criação.
     * Permite rastrear e diferenciar requisições com os mesmos
     * parâmetros (ex.: dois processos pedindo o mesmo cilindro).
     */
    uint32_t id;

    /**
     * @brief Número do cilindro que deve ser acessado.
     *
     * Deve estar no intervalo [0, DISK_MAX_CYLINDERS - 1].
     * É o campo central para os algoritmos de escalonamento:
     * SSTF, SCAN e C-SCAN tomam decisões baseados neste valor.
     */
    uint32_t cylinder;

    /**
     * @brief Identificador do processo que gerou esta requisição.
     *
     * Em um SO real, seria o PID. Aqui é um inteiro simples que
     * permite identificar qual processo será atendido.
     */
    uint32_t process_id;

    /**
     * @brief Instante de chegada da requisição (em "ticks" simulados).
     *
     * Representa o momento em que a requisição entrou na fila.
     * Útil para calcular tempo de espera e métricas de justiça
     * (starvation) entre requisições.
     */
    uint64_t arrival_time;

    /**
     * @brief Tipo da operação: leitura (OP_READ) ou escrita (OP_WRITE).
     *
     * Alguns algoritmos avançados podem priorizar leituras sobre
     * escritas. Por ora, o campo é armazenado para fins de registro
     * e futura extensão.
     */
    OperationType operation;

    /**
     * @brief Prazo limite para atendimento da requisição (em ticks simulados).
     *
     * Utilizado pelo algoritmo EDF (Earliest Deadline First):
     * requisições com menor deadline são priorizadas para evitar que
     * estourem seu prazo. Um valor de 0 pode indicar "sem deadline".
     */
    uint64_t deadline;

    /**
     * @brief Prioridade dinâmica da requisição (usada pelo mecanismo de Aging).
     *
     * Valor inicial definido no momento da criação. O mecanismo de Aging
     * incrementa este campo a cada tick que a requisição permanece na fila,
     * evitando starvation de requisições de baixa prioridade.
     *
     * Quanto maior o valor, maior a prioridade.
     */
    int priority;

} Request;

/* ------------------------------------------------------------------ */
/*  Protótipos de funções                                               */
/* ------------------------------------------------------------------ */

/**
 * @brief  Cria e inicializa uma nova requisição de disco.
 *
 * @param  id            Identificador único da requisição.
 * @param  cylinder      Cilindro alvo (deve ser < DISK_MAX_CYLINDERS).
 * @param  process_id    Identificador do processo solicitante.
 * @param  arrival_time  Instante de chegada (tick do simulador).
 * @param  operation     Tipo de operação (OP_READ ou OP_WRITE).
 * @param  deadline      Prazo limite de atendimento (tick); 0 = sem prazo.
 * @param  priority      Prioridade inicial da requisição (Aging).
 * @return Request       Struct preenchida e pronta para uso.
 */
Request request_create(uint32_t id,
                       uint32_t cylinder,
                       uint32_t process_id,
                       uint64_t arrival_time,
                       OperationType operation,
                       uint64_t deadline,
                       int priority);

/**
 * @brief  Verifica se os campos de uma requisição são válidos.
 *
 * Checa se o cilindro está dentro do intervalo permitido e se
 * o tipo de operação é um valor conhecido do enum.
 *
 * @param  req  Ponteiro para a requisição a ser validada.
 * @return true se válida, false caso contrário.
 */
bool request_is_valid(const Request *req);

/**
 * @brief  Imprime os campos de uma requisição no stdout.
 *
 * Útil para depuração e para demonstração em apresentações.
 *
 * @param  req  Ponteiro para a requisição a ser exibida.
 */
void request_print(const Request *req);

#endif /* REQUEST_H */