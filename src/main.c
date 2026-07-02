/**
 * @file    main.c
 * @brief   Ponto de entrada do simulador de escalonamento de disco.
 *
 * Padrao: C11
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "../include/simulation.h"
#include "../include/config.h"   /* Necessario para CONFIG_CYLINDERS */

/* ------------------------------------------------------------------ */
/*  Funcoes auxiliares de interface                                    */
/* ------------------------------------------------------------------ */

static bool read_line(char *buffer, size_t size)
{
    if (fgets(buffer, (int)size, stdin) == NULL) return false;
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = '\0';
    return true;
}

static bool parse_uint32(const char *str, uint32_t *value, uint32_t min, uint32_t max)
{
    char *endptr;
    unsigned long val = strtoul(str, &endptr, 10);
    if (*endptr != '\0' || val < min || val > max) return false;
    *value = (uint32_t)val;
    return true;
}

static void print_menu(void)
{
    printf("\n");
    printf("+-------------------------------------------+\n");
    printf("|   SIMULADOR DE ESCALONAMENTO DE DISCO     |\n");
    printf("+-------------------------------------------+\n");
    printf("|  Selecione o algoritmo:                   |\n");
    printf("|  1 - FCFS                                |\n");
    printf("|  2 - SSTF                                |\n");
    printf("|  3 - SCAN                                |\n");
    printf("|  4 - C-SCAN                              |\n");
    printf("|  5 - DEADLINE                            |\n");
    printf("+-------------------------------------------+\n");
    printf("\n");
    printf("Digite o numero do algoritmo: ");
}

static Algorithm select_algorithm(void)
{
    char input[16];
    uint32_t choice;
    while (1) {
        print_menu();
        if (!read_line(input, sizeof(input))) continue;
        if (!parse_uint32(input, &choice, 1, 5)) {
            printf("Opcao invalida. Digite um numero entre 1 e 4.\n");
            continue;
        }
        break;
    }
    switch (choice) {
        case 1: return ALG_FCFS;
        case 2: return ALG_SSTF;
        case 3: return ALG_SCAN;
        case 4: return ALG_CSCAN;
        case 5: return ALG_DEADLINE;
        default: return ALG_FCFS;
    }
}

static uint32_t read_request_count(void)
{
    char input[32];
    uint32_t count;
    while (1) {
        printf("Digite a quantidade de requisicoes (1 a 100000): ");
        if (!read_line(input, sizeof(input))) continue;
        if (!parse_uint32(input, &count, 1, 100000)) {
            printf("Valor invalido. Digite um numero entre 1 e 100000.\n");
            continue;
        }
        break;
    }
    return count;
}

/**
 * @brief Le a posicao inicial da cabeca do disco.
 *
 * Solicita ao usuario um cilindro dentro dos limites validos.
 * A validacao utiliza CONFIG_CYLINDERS para definir o maximo.
 *
 * @return Cilindro inicial escolhido pelo usuario.
 */
static uint32_t read_initial_head(void)
{
    char input[32];
    uint32_t head;
    uint32_t max_cylinder = CONFIG_CYLINDERS - 1;

    while (1) {
        printf("Digite a posicao inicial da cabeca (0 a %u): ", max_cylinder);
        if (!read_line(input, sizeof(input))) continue;
        if (!parse_uint32(input, &head, 0, max_cylinder)) {
            printf("Valor invalido. Digite um numero entre 0 e %u.\n", max_cylinder);
            continue;
        }
        break;
    }

    return head;
}

static bool read_seed(uint32_t *seed)
{
    char input[32];
    uint32_t value;
    printf("Digite a semente para geracao aleatoria (0 para usar time(NULL)): ");
    if (!read_line(input, sizeof(input))) {
        *seed = 0;
        return false;
    }
    if (!parse_uint32(input, &value, 0, UINT32_MAX)) {
        printf("Valor invalido. Usando semente automatica (time(NULL)).\n");
        *seed = 0;
        return false;
    }
    *seed = value;
    return true;
}

static bool ask_continue(void)
{
    char input[16];
    printf("Deseja realizar outra simulacao? (s/N): ");
    if (!read_line(input, sizeof(input))) return false;
    for (char *p = input; *p; p++) *p = (char)tolower(*p);
    return (strcmp(input, "s") == 0 || strcmp(input, "sim") == 0 ||
            strcmp(input, "y") == 0 || strcmp(input, "yes") == 0);
}

/* ------------------------------------------------------------------ */
/*  Funcao principal                                                   */
/* ------------------------------------------------------------------ */

int main(void)
{
    Simulation sim;
    simulation_init(&sim);

    printf("\n");
    printf("+-------------------------------------------------------+\n");
    printf("|                                                       |\n");
    printf("|       SIMULADOR DE ESCALONAMENTO DE DISCO             |\n");
    printf("|                                                       |\n");
    printf("+-------------------------------------------------------+\n");

    do {
        simulation_reset(&sim);
        Algorithm algo = select_algorithm();
        simulation_select_algorithm(&sim, algo);

        uint32_t count = read_request_count();

        /* NOVA FUNCIONALIDADE: ler posicao inicial da cabeca */
        uint32_t initial_head = read_initial_head();
        simulation_set_initial_head(&sim, initial_head);

        uint32_t seed;
        read_seed(&seed);
        simulation_set_seed(&sim, seed);

        printf("\nGerando %u requisicoes... ", count);
        if (!simulation_generate_requests(&sim, count)) {
            printf("ERRO: Falha ao gerar requisicoes (memoria insuficiente?)\n");
            continue;
        }
        printf("OK.\n");

        printf("Executando algoritmo %s...\n",
               (algo == ALG_FCFS) ? "FCFS" :
               (algo == ALG_SSTF) ? "SSTF" :
               (algo == ALG_SCAN) ? "SCAN" : "C-SCAN");

        if (!simulation_run(&sim)) {
            printf("ERRO: Falha ao executar a simulacao.\n");
            continue;
        }
        printf("Simulacao concluida.\n");
        
        simulation_print_report(&sim);

    } while (ask_continue());

    printf("\nEncerrando simulador.\n");
    return 0;
}