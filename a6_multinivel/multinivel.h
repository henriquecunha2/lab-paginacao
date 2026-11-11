/*
 * multinivel.h — tabela de páginas de dois níveis (slide 31), para endereços de 32 bits:
 *
 *   | PT1: 10 bits | PT2: 10 bits | deslocamento: 12 bits |
 *
 * PT1 escolhe uma entrada da tabela de 1º nível; essa entrada aponta para uma
 * tabela de 2º nível; PT2 escolhe, dentro dela, a entrada da página.
 * Cada tabela de 2º nível cobre 1024 páginas de 4 KB, isto é, 4 MB.
 */
#ifndef MULTINIVEL_H
#define MULTINIVEL_H
#include <stdint.h>

/* Entrada de uma tabela de 2º nível: descreve uma página. */
typedef struct {
    uint32_t moldura;       /* número da moldura; só vale se presente == 1 */
    uint8_t  presente;      /* 1 = página na memória; 0 = ausente          */
} pte_t;

/* A tabela completa de um processo. */
typedef struct {
    pte_t   *nivel2[1024];  /* 1º nível: nivel2[PT1] aponta para a tabela de 2º   */
                            /* nível daquela região de 4 MB, ou é NULL se a       */
                            /* região nunca foi usada e a tabela não existe       */
    unsigned tabelas;       /* quantas tabelas existem, contando a de 1º nível;   */
                            /* comece com 1 e some 1 a cada tabela de 2º nível    */
} tp2_t;

/*
 * Registra que a página que contém o endereço virt está na moldura dada.
 * Se a tabela de 2º nível da região ainda não existe, aloca uma, com as
 * 1024 entradas zeradas (todas ausentes), e incrementa t->tabelas.
 */
void mapeia(tp2_t *t, uint32_t virt, uint32_t moldura);

/*
 * Traduz virt percorrendo os dois níveis.
 * Retorna 0 e preenche *fis, ou -1 em caso de falta de página: quando não
 * existe tabela de 2º nível para a região ou quando a página está ausente.
 */
int traduz2(const tp2_t *t, uint32_t virt, uint32_t *fis);

#endif
