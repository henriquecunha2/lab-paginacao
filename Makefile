# Makefile — compila todos os programas do laboratório.
#
#   make          compila tudo; cada executável fica na pasta da sua atividade
#   make clean    apaga os executáveis
#
# -Wall -Wextra ligam os avisos do compilador; -g inclui informações para o gdb.

CC     = gcc
CFLAGS = -Wall -Wextra -g

PROGRAMAS = a1_paginas/paginas \
            a2_a3_mmu/traducao a2_a3_mmu/falta a2_a3_mmu/faltas_linux \
            a5_tlb/tlb \
            a6_multinivel/multinivel a6_multinivel/indices \
            desafio/pagemap

all: $(PROGRAMAS)

# Os dois simuladores da MMU usam as funções de mmu.c.
a2_a3_mmu/traducao: a2_a3_mmu/traducao.c a2_a3_mmu/mmu.c a2_a3_mmu/mmu.h a2_a3_mmu/mapas.h
	$(CC) $(CFLAGS) -o $@ a2_a3_mmu/traducao.c a2_a3_mmu/mmu.c

a2_a3_mmu/falta: a2_a3_mmu/falta.c a2_a3_mmu/mmu.c a2_a3_mmu/mmu.h a2_a3_mmu/mapas.h
	$(CC) $(CFLAGS) -o $@ a2_a3_mmu/falta.c a2_a3_mmu/mmu.c

a6_multinivel/multinivel: a6_multinivel/multinivel.c a6_multinivel/multinivel.h
	$(CC) $(CFLAGS) -o $@ a6_multinivel/multinivel.c

# Regra geral para os programas de um único arquivo: pasta/nome a partir de pasta/nome.c
%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGRAMAS)

.PHONY: all clean
