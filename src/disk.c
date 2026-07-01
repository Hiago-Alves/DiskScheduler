/**
 * @file    disk.c
 * @brief   Implementação das funções do disco rígido simulado.
 *
 * Padrão: C11
 */

#include "disk.h"

#include <stdio.h>   /* printf           */
#include <stddef.h>  /* NULL             */

/* ------------------------------------------------------------------ */
/*  Funções auxiliares internas (static = visível só neste arquivo)    */
/* ------------------------------------------------------------------ */

/**
 * @brief Calcula o valor absoluto da diferença entre dois uint32_t.
 *
 * Necessário porque a subtração de uint32_t sem cuidado causa
 * underflow silencioso em C (ex.: 3u - 5u = 4294967294u).
 * Comparamos os valores antes de subtrair para garantir resultado
 * correto sem cast para tipo com sinal.
 *
 * @param  a  Primeiro valor.
 * @param  b  Segundo valor.
 * @return |a - b| como uint32_t.
 */
static uint32_t uint32_abs_diff(uint32_t a, uint32_t b)
{
    return (a >= b) ? (a - b) : (b - a);
}

/* ------------------------------------------------------------------ */
/*  Implementação das funções públicas                                  */
/* ------------------------------------------------------------------ */

/**
 * @brief Inicializa o disco com valores padrão de config.h.
 *
 * Zera as métricas acumuladas e posiciona a cabeça conforme
 * CONFIG_INITIAL_HEAD. A direção inicial é DIR_RIGHT por convenção:
 * a cabeça começa se movendo em direção aos cilindros maiores,
 * comportamento padrão dos algoritmos SCAN e C-SCAN.
 */
void disk_init(Disk *disk)
{
    if (disk == NULL) {
        return;  /* proteção defensiva: nunca desreferenciar NULL */
    }

    disk->head_position      = CONFIG_INITIAL_HEAD;
    disk->direction          = DIR_RIGHT;
    disk->current_time       = CONFIG_INITIAL_TIME;
    disk->total_seek_distance = 0u;
}

/**
 * @brief Calcula o seek time até um cilindro alvo.
 *
 * O seek time é linear com a distância: cada cilindro de diferença
 * custa CONFIG_SEEK_TIME_PER_CYLINDER microsegundos. Isso é uma
 * simplificação do modelo real (que tem aceleração e desaceleração),
 * suficiente para fins de simulação e comparação de algoritmos.
 *
 * Exemplos:
 *   head=100, cylinder=150 → distância=50 → seek=50 us
 *   head=100, cylinder=100 → distância=0  → seek=0  us (já no lugar)
 */
uint64_t disk_seek_time(const Disk *disk, uint32_t cylinder)
{
    if (disk == NULL) {
        return 0u;
    }

    /* Distância absoluta em número de cilindros */
    uint32_t distance = uint32_abs_diff(cylinder, disk->head_position);

    /* Custo em microsegundos: distância × tempo por cilindro */
    return (uint64_t)distance * CONFIG_SEEK_TIME_PER_CYLINDER;
}

/**
 * @brief Retorna a latência rotacional média.
 *
 * Em discos reais, a latência varia de 0 a T_rotação dependendo de
 * onde o setor se encontra no momento do seek. A média estatística
 * é T_rotação / 2. Para 7200 RPM: 60s/7200 = 8,33ms → média = 4,16ms.
 * Usamos CONFIG_ROTATIONAL_LATENCY_US = 4000 us como aproximação.
 *
 * O parâmetro `disk` não é usado nesta implementação, mas está
 * presente para consistência da API e extensão futura.
 */
uint64_t disk_rotational_latency(const Disk *disk)
{
    (void)disk;  /* suprime warning de parâmetro não utilizado */
    return CONFIG_ROTATIONAL_LATENCY_US;
}

/**
 * @brief Retorna o tempo fixo de transferência por operação.
 *
 * Representa o tempo físico de leitura ou escrita do setor após
 * o posicionamento. Na prática é proporcional ao tamanho do setor
 * e à densidade do disco; aqui usamos valor fixo por simplificação.
 *
 * O parâmetro `disk` não é usado nesta implementação, mas está
 * presente para consistência da API e extensão futura.
 */
uint64_t disk_transfer_time(const Disk *disk)
{
    (void)disk;  /* suprime warning de parâmetro não utilizado */
    return CONFIG_TRANSFER_TIME_US;
}

/**
 * @brief Calcula o tempo total de acesso a um cilindro (somente leitura).
 *
 * Combina as três fases sequenciais de um acesso a disco:
 *
 *   total = seek_time + rotational_latency + transfer_time
 *
 * Esta função é PURA em relação ao estado do disco: pode ser chamada
 * várias vezes com diferentes cilindros para comparar custos sem
 * alterar nada — útil para os algoritmos SSTF e EDF escolherem
 * a melhor requisição antes de efetivar o movimento.
 */
uint64_t disk_access_time(const Disk *disk, uint32_t cylinder)
{
    if (disk == NULL) {
        return 0u;
    }

    uint64_t seek  = disk_seek_time(disk, cylinder);
    uint64_t rot   = disk_rotational_latency(disk);
    uint64_t trans = disk_transfer_time(disk);

    return seek + rot + trans;
}

/**
 * @brief Move a cabeça para o cilindro alvo, atualizando o estado do disco.
 *
 * Esta é a única função que produz efeitos colaterais no disco.
 * Deve ser chamada pelo escalonador somente após decidir qual
 * requisição atender.
 *
 * Sequência de operações internas:
 *   1. Valida os parâmetros (NULL e cilindro fora do intervalo)
 *   2. Calcula a distância percorrida e acumula em total_seek_distance
 *   3. Soma o tempo total de acesso em current_time
 *   4. Atualiza direction com base no sentido do movimento
 *   5. Atualiza head_position para o cilindro de destino
 *
 * A direção não é atualizada quando cylinder == head_position
 * (movimento nulo), preservando a direção anterior do SCAN.
 */
bool disk_move_head(Disk *disk, uint32_t cylinder)
{
    /* Validação: ponteiro nulo */
    if (disk == NULL) {
        return false;
    }

    /* Validação: cilindro dentro do espaço de endereçamento */
    if (cylinder >= CONFIG_CYLINDERS) {
        return false;
    }

    /* Distância de seek em número de cilindros */
    uint32_t distance = uint32_abs_diff(cylinder, disk->head_position);

    /* Acumula na métrica global de distância percorrida */
    disk->total_seek_distance += distance;

    /* Avança o relógio da simulação com o tempo total da operação */
    disk->current_time += disk_seek_time(disk, cylinder)
                        + disk_rotational_latency(disk)
                        + disk_transfer_time(disk);

    /* Atualiza direção apenas se há movimento real */
    if (cylinder > disk->head_position) {
        disk->direction = DIR_RIGHT;
    } else if (cylinder < disk->head_position) {
        disk->direction = DIR_LEFT;
    }
    /* cylinder == head_position: direção preservada intencionalmente */

    /* Posiciona a cabeça no destino */
    disk->head_position = cylinder;

    return true;
}

/**
 * @brief Imprime o estado atual do disco no stdout.
 *
 * Exibe todos os campos da struct em formato legível.
 * Converte DiskDirection para string para facilitar leitura.
 */
void disk_print_state(const Disk *disk)
{
    if (disk == NULL) {
        printf("[Disk] ERRO: ponteiro nulo recebido.\n");
        return;
    }

    const char *dir_str = (disk->direction == DIR_RIGHT) ? "DIREITA (→)"
                                                         : "ESQUERDA (←)";

    printf("┌─────────────────────────────────────────┐\n");
    printf("│           Estado do Disco                │\n");
    printf("├─────────────────────────────────────────┤\n");
    printf("│  Cabeça:       cilindro %-4u             │\n",
           disk->head_position);
    printf("│  Direção:      %-10s               │\n", dir_str);
    printf("│  Tempo atual:  %-10llu us             │\n",
           (unsigned long long)disk->current_time);
    printf("│  Dist. total:  %-10llu cilindros      │\n",
           (unsigned long long)disk->total_seek_distance);
    printf("└─────────────────────────────────────────┘\n");
}