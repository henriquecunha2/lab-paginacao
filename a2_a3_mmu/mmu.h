/*
 * mmu.h — a máquina simulada dos slides 23 a 26.
 *
 *   endereço virtual: 16 bits = 4 bits de página + 12 bits de deslocamento
 *   endereço físico : 15 bits = 3 bits de moldura + 12 bits de deslocamento
 *
 * Com 12 bits de deslocamento, cada página tem 2^12 = 4096 bytes (4 KB).
 */
#ifndef MMU_H
#define MMU_H
#include <stdint.h>

#define BITS_DESLOC 12                    /* bits do deslocamento                  */
#define TAM_PAGINA  (1u << BITS_DESLOC)   /* 2^12 = 4096 bytes por página          */
#define N_PAGINAS   16                    /* 2^4 páginas virtuais (16 bits)        */
#define N_MOLDURAS  8                     /* 2^3 molduras físicas (15 bits)        */

/* Permissões de uma página. São bits; combine-os com | (PROT_L | PROT_E). */
#define PROT_L 1                          /* pode ser lida                         */
#define PROT_E 2                          /* pode ser escrita                      */

/*
 * Uma entrada da tabela de páginas (slide 32). No hardware, todos os campos
 * cabem em uma única palavra; aqui cada campo ocupa um byte, para o código
 * ficar legível.
 */
typedef struct {
    uint8_t moldura;        /* número da moldura; só vale se presente == 1        */
    uint8_t presente;       /* presente/ausente: 1 = na memória; 0 = falta        */
    uint8_t referenciada;   /* bit R: a página foi acessada (leitura ou escrita)  */
    uint8_t modificada;     /* bit M: a página foi escrita; a cópia em disco      */
                            /*        está desatualizada                          */
    uint8_t prot;           /* proteção: PROT_L, ou PROT_L | PROT_E               */
} pte_t;

/*
 * Faz o trabalho da MMU para um endereço virtual.
 *   tp   : tabela de páginas, com N_PAGINAS entradas; o índice é o número da página
 *   virt : endereço virtual de 16 bits
 *   fis  : saída; recebe o endereço físico de 15 bits
 * Retorna 0 se traduziu, ou -1 se a página está ausente (falta de página);
 * nesse caso, *fis não é alterado.
 */
int traduz(const pte_t tp[], uint16_t virt, uint16_t *fis);

/*
 * Preenche a tabela a partir de um mapeamento simples.
 *   mapa[p] : moldura da página p, ou -1 se a página está ausente
 * Todas as entradas ficam com R = M = 0 e com permissão de leitura e escrita.
 */
void carrega(pte_t tp[], const int mapa[]);

#endif
