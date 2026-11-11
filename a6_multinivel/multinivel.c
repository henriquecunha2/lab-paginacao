/*
 * multinivel.c — tabela de páginas de dois níveis (Atividade 6).
 *
 * Este arquivo é um esqueleto: complete mapeia, traduz2 e o main.
 */
#include <stdio.h>
#include <stdlib.h>
#include "multinivel.h"

void mapeia(tp2_t *t, uint32_t virt, uint32_t moldura) {
    /*
     * TODO (Atividade 6, Passo 1):
     *   1. extraia PT1 (10 bits de cima) e PT2 (10 bits do meio) de virt;
     *   2. se t->nivel2[PT1] é NULL, aloque uma tabela de 1024 entradas com
     *      calloc (que as zera) e some 1 a t->tabelas;
     *   3. grave a moldura na entrada PT2 dessa tabela e marque-a presente.
     */
    (void)t; (void)virt; (void)moldura;     /* remova esta linha ao implementar */
}

int traduz2(const tp2_t *t, uint32_t virt, uint32_t *fis) {
    /*
     * TODO (Atividade 6, Passo 1):
     *   1. extraia PT1, PT2 e o deslocamento (12 bits de baixo);
     *   2. sem tabela de 2º nível para PT1, ou com a página ausente: retorne -1;
     *   3. monte o endereço físico com a moldura e o deslocamento e retorne 0.
     */
    (void)t; (void)virt; (void)fis;         /* remova esta linha ao implementar */
    return -1;
}

int main(void) {
    /* A tabela do processo. calloc zera tudo: nenhum ponteiro de 2º nível
       existe ainda. A contagem começa em 1 por causa da tabela de 1º nível. */
    tp2_t *t = calloc(1, sizeof *t);
    t->tabelas = 1;

    /* TODO (Passos 2 a 5): monte o processo do slide 31, traduza os endereços
       do roteiro, faça os dados crescerem e teste o caso das páginas espalhadas. */

    printf("tabelas: %u (%u KiB)\n", t->tabelas, t->tabelas * 4);
    return 0;
}
