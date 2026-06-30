/**
 * @file    request.c
 * @brief   Implementação das funções do módulo de requisição de disco.
 *
 * Padrão: C11
 */

#include "request.h"

#include <stdio.h>    /* printf                      */
#include <assert.h>   /* assert                      */

/* ------------------------------------------------------------------ */
/*  Implementação das funções                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief Cria e retorna uma struct Request completamente inicializada.
 *
 * A função não aloca memória dinamicamente — a struct é retornada
 * por valor, o que é seguro e eficiente para estruturas pequenas.
 *
 * O atendimento da requisição é sinalizado pela sua remoção da AVL,
 * por isso não há campo `served` nesta versão.
 */
Request request_create(uint32_t id,
                       uint32_t cylinder,
                       uint32_t process_id,
                       uint64_t arrival_time,
                       OperationType operation,
                       uint64_t deadline,
                       int priority)
{
    /* Garante em tempo de execução que o cilindro é válido */
    assert(cylinder < DISK_MAX_CYLINDERS);

    Request req = {
        .id           = id,
        .cylinder     = cylinder,
        .process_id   = process_id,
        .arrival_time = arrival_time,
        .operation    = operation,
        .deadline     = deadline,   /* 0 = sem prazo definido         */
        .priority     = priority    /* valor inicial; Aging irá ajustar */
    };

    return req;
}

/**
 * @brief Valida se uma requisição possui dados coerentes.
 *
 * Retorna false nos seguintes casos:
 *  - Ponteiro nulo recebido
 *  - Cilindro fora do intervalo permitido
 *  - Tipo de operação não reconhecido
 */
bool request_is_valid(const Request *req)
{
    /* Ponteiro nulo é sempre inválido */
    if (req == NULL) {
        return false;
    }

    /* Cilindro deve estar dentro dos limites do disco simulado */
    if (req->cylinder >= DISK_MAX_CYLINDERS) {
        return false;
    }

    /* Operação deve ser um dos valores definidos no enum */
    if (req->operation != OP_READ && req->operation != OP_WRITE) {
        return false;
    }

    return true;
}

/**
 * @brief Exibe todos os campos de uma requisição de forma legível.
 *
 * Formato de saída:
 *   [Request #<id>] Cilindro: <n> | PID: <p> | Chegada: <t>
 *                   Deadline: <d> | Prioridade: <p> | Operação: <LEITURA|ESCRITA>
 *
 * Protege contra ponteiro nulo antes de acessar os campos.
 */
void request_print(const Request *req)
{
    if (req == NULL) {
        printf("[Request] ERRO: ponteiro nulo recebido.\n");
        return;
    }

    /* Converte o enum para string legível */
    const char *op_str = (req->operation == OP_READ) ? "LEITURA" : "ESCRITA";

    /* Exibe deadline como "N/A" quando o valor for 0 (sem prazo) */
    if (req->deadline == 0) {
        printf(
            "[Request #%u] Cilindro: %4u | PID: %3u | Chegada: %6llu"
            " | Deadline:    N/A | Prioridade: %3d | Operação: %s\n",
            req->id,
            req->cylinder,
            req->process_id,
            (unsigned long long)req->arrival_time,
            req->priority,
            op_str
        );
    } else {
        printf(
            "[Request #%u] Cilindro: %4u | PID: %3u | Chegada: %6llu"
            " | Deadline: %6llu | Prioridade: %3d | Operação: %s\n",
            req->id,
            req->cylinder,
            req->process_id,
            (unsigned long long)req->arrival_time,
            (unsigned long long)req->deadline,
            req->priority,
            op_str
        );
    }
}