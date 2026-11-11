/*
 * mapas.h — os dois mapeamentos dos slides, usados nas Atividades 2 e 3.
 *
 * Cada vetor descreve uma tabela de páginas inteira:
 *   posição = número da página virtual (0 a 15);
 *   valor   = moldura em que a página está, ou -1 se está ausente (o X da figura).
 *
 * mapa_slide23 vem da figura do slide 23, lida de baixo para cima (a página 0
 * é a faixa 0K–4K); os exemplos do slide 24 usam esse mapa.
 * mapa_slide26 vem da tabela desenhada dentro da MMU no slide 26.
 */
#ifndef MAPAS_H
#define MAPAS_H
#include "mmu.h"

/*                 página:                   0  1  2  3   4  5   6  7   8   9  10  11  12  13  14  15 */
static const int mapa_slide23[N_PAGINAS] = { 1, 6, 0, 2, -1, 7, -1, 5, -1, -1,  3, -1, -1,  4, -1, -1 };
static const int mapa_slide26[N_PAGINAS] = { 2, 1, 6, 0,  4, 3, -1, -1, -1,  5, -1,  7, -1, -1, -1, -1 };

#endif
