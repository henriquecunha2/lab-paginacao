/*
 * mmu.c — implementação das funções declaradas em mmu.h (Atividade 2).
 *
 * Este arquivo é um esqueleto: ele compila, mas ainda não traduz nada.
 * Complete as duas funções nos pontos marcados com TODO.
 */
#include "mmu.h"

int traduz(const pte_t tp[], uint16_t virt, uint16_t *fis) {
    /*
     * TODO (Atividade 2, Passo 1):
     *   1. separe virt em número de página (bits de cima) e deslocamento
     *      (os BITS_DESLOC bits de baixo), usando apenas >> e &;
     *   2. se a entrada da página não está presente, retorne -1;
     *   3. monte o endereço físico juntando a moldura e o deslocamento,
     *      com << e |, grave-o em *fis e retorne 0.
     */
    (void)tp; (void)virt; (void)fis;    /* remova esta linha ao implementar */
    return -1;
}

void carrega(pte_t tp[], const int mapa[]) {
    /*
     * TODO (Atividade 2, Passo 1): para cada página p de 0 a N_PAGINAS - 1,
     *   - zere todos os campos da entrada (R = M = 0, ausente);
     *   - dê permissão de leitura e escrita (PROT_L | PROT_E);
     *   - se mapa[p] >= 0, guarde a moldura e marque a página como presente.
     */
    (void)tp; (void)mapa;               /* remova esta linha ao implementar */
}
