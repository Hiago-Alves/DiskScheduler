/**
 * @file    config.h
 * @brief   Constantes de configuração do simulador de disco.
 *
 * Padrão: C11
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ------------------------------------------------------------------ */
/*  Geometria do disco                                                  */
/* ------------------------------------------------------------------ */

#define CONFIG_CYLINDERS        1024u
#define CONFIG_INITIAL_HEAD     0u

/* ------------------------------------------------------------------ */
/*  Parâmetros de tempo (em microsegundos)                             */
/* ------------------------------------------------------------------ */

#define CONFIG_SEEK_TIME_PER_CYLINDER   1u
#define CONFIG_ROTATIONAL_LATENCY_US    4000u
#define CONFIG_TRANSFER_TIME_US         500u

/* ------------------------------------------------------------------ */
/*  Configurações do simulador                                          */
/* ------------------------------------------------------------------ */

#define CONFIG_INITIAL_TIME     0u

/* ------------------------------------------------------------------ */
/*  Deadline (para o algoritmo DEADLINE)                               */
/* ------------------------------------------------------------------ */

#define CONFIG_DEADLINE_LIMIT 50

/* ------------------------------------------------------------------ */
/*  Aging (anti-starvation)                                            */
/* ------------------------------------------------------------------ */

#define AGING_INTERVAL 1000      
#define AGING_THRESHOLD 100      

/* ------------------------------------------------------------------ */
/*  Coalescência de requisições adjacentes                             */
/* ------------------------------------------------------------------ */

#define COALESCE_THRESHOLD 2

/* ------------------------------------------------------------------ */
/*  Cargas sintéticas                                                  */
/* ------------------------------------------------------------------ */

typedef enum {
    LOAD_RANDOM,
    LOAD_SEQUENTIAL,
    LOAD_ZIPF
} LoadType;

#define ZIPF_ALPHA 1.0

#endif /* CONFIG_H */