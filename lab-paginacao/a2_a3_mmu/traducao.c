/*
 * traducao.c — programa de teste da Atividade 2.
 *
 * Carrega cada um dos dois mapeamentos dos slides e traduz uma lista de
 * endereços virtuais, mostrando o caminho da tradução como no slide 26.
 * Complete a função mostra_traducao (Passo 2); o main já faz o Passo 3.
 */
#include <stdio.h>
#include "mmu.h"
#include "mapas.h"

/* Imprime os n bits menos significativos de v, do mais alto para o mais baixo. */
static void binario(unsigned v, int n) {
    for (int i = n - 1; i >= 0; i--)
        putchar(((v >> i) & 1) ? '1' : '0');
}

/*
 * Traduz virt com a tabela tp e imprime uma linha no formato do roteiro:
 *
 *  8196 = 0010 000000000100  página  2 desloc    4 -> moldura 6  110 000000000100 = 24580
 *
 * ou, se a página estiver ausente, termina a linha com FALTA DE PÁGINA.
 */
static void mostra_traducao(const pte_t tp[], uint16_t virt) {
    /* TODO (Atividade 2, Passo 2): use traduz() e binario(). */
    (void)tp; (void)binario;            /* remova esta linha ao implementar */
    printf("%5u = (a implementar)\n", (unsigned)virt);
}

int main(void) {
    pte_t tp[N_PAGINAS];                /* a tabela de páginas da máquina simulada */

    /* Passo 3, primeira parte: o mapeamento do slide 26. */
    const uint16_t com26[] = { 0, 8192, 8196, 20500, 4095, 4096, 32780, 45100 };
    carrega(tp, mapa_slide26);
    puts("mapa do slide 26");
    for (unsigned i = 0; i < sizeof com26 / sizeof *com26; i++)
        mostra_traducao(tp, com26[i]);

    /* Passo 3, segunda parte: o mapeamento dos slides 23 e 24. */
    const uint16_t com23[] = { 8192, 8196, 20500, 16500, 4095, 4096 };
    carrega(tp, mapa_slide23);
    puts("mapa do slide 23");
    for (unsigned i = 0; i < sizeof com23 / sizeof *com23; i++)
        mostra_traducao(tp, com23[i]);

    return 0;
}
