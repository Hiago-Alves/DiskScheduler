/**
 * @file    main.c
 * @brief   Ponto de entrada do simulador.
 *
 * Padrao: C11
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "../include/simulation.h"
#include "../include/config.h"

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
    printf("|  4 - C-LOOK                              |\n");
    printf("|  5 - DEADLINE                            |\n");
    printf("|  6 - TESTE DE FOGO (SSTF vs DEADLINE)   |\n");
    printf("+-------------------------------------------+\n");
    printf("\n");
    printf("Digite o numero do algoritmo: ");
}

static LoadType read_load_type(void)
{
    char input[16];
    uint32_t choice;
    while (1) {
        printf("\nTipo de carga:\n");
        printf("  1 - Aleatoria\n");
        printf("  2 - Sequencial\n");
        printf("  3 - Zipfiana\n");
        printf("Escolha (1-3): ");
        if (!read_line(input, sizeof(input))) continue;
        if (!parse_uint32(input, &choice, 1, 3)) {
            printf("Opcao invalida.\n");
            continue;
        }
        break;
    }
    switch (choice) {
        case 1: return LOAD_RANDOM;
        case 2: return LOAD_SEQUENTIAL;
        case 3: return LOAD_ZIPF;
        default: return LOAD_RANDOM;
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
    printf("Digite a semente (0 para usar time(NULL)): ");
    if (!read_line(input, sizeof(input))) {
        *seed = 0;
        return false;
    }
    if (!parse_uint32(input, &value, 0, UINT32_MAX)) {
        printf("Valor invalido. Usando time(NULL).\n");
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

        uint32_t opcao;
        while (1) {
            print_menu();
            char input[16];
            if (!read_line(input, sizeof(input))) continue;
            if (!parse_uint32(input, &opcao, 1, 6)) {
                printf("Opcao invalida.\n");
                continue;
            }
            break;
        }

        if (opcao == 6) {
            simulation_enable_gnuplot(&sim, true);
            simulation_run_fire_test(&sim);
            simulation_enable_gnuplot(&sim, false);
            continue;
        }

        /* Mapeia opcao para algoritmo */
        switch (opcao) {
            case 1: simulation_select_algorithm(&sim, ALG_FCFS); break;
            case 2: simulation_select_algorithm(&sim, ALG_SSTF); break;
            case 3: simulation_select_algorithm(&sim, ALG_SCAN); break;
            case 4: simulation_select_algorithm(&sim, ALG_CSCAN); break;
            case 5: simulation_select_algorithm(&sim, ALG_DEADLINE); break;
            default: simulation_select_algorithm(&sim, ALG_FCFS);
        }

        uint32_t count = read_request_count();
        uint32_t initial_head = read_initial_head();
        simulation_set_initial_head(&sim, initial_head);

        LoadType load = read_load_type();
        simulation_set_load_type(&sim, load);

        uint32_t seed;
        read_seed(&seed);
        simulation_set_seed(&sim, seed);

        simulation_enable_gnuplot(&sim, true);

        printf("\nGerando %u requisicoes... ", count);
        if (!simulation_generate_requests(&sim, count)) {
            printf("ERRO: Falha ao gerar requisicoes.\n");
            continue;
        }
        printf("OK.\n");

        scheduler_coalesce(&sim.scheduler);

        const char *algo_name;
        switch (sim.algorithm) {
            case ALG_FCFS:  algo_name = "FCFS"; break;
            case ALG_SSTF:  algo_name = "SSTF"; break;
            case ALG_SCAN:  algo_name = "SCAN"; break;
            case ALG_CSCAN: algo_name = "C-LOOK"; break;
            case ALG_DEADLINE: algo_name = "DEADLINE"; break;
            default: algo_name = "Desconhecido";
        }
        printf("Executando algoritmo %s...\n", algo_name);

        if (!simulation_run(&sim)) {
            printf("ERRO: Falha ao executar a simulacao.\n");
            continue;
        }
        printf("Simulacao concluida.\n");

        simulation_print_report(&sim);
        simulation_enable_gnuplot(&sim, false);

    } while (ask_continue());

    printf("\nEncerrando simulador.\n");
    return 0;
}