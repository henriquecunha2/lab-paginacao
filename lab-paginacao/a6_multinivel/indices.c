/*
 * indices.c — decompõe um endereço virtual do x86-64 (Atividade 6, Passo 6).
 *
 * Uso: ./indices 0x7ffd3a5c1e48
 *
 * Dos 64 bits do endereço, o x86-64 usa 48:
 *
 *   | nível 4: 9 bits | nível 3: 9 bits | nível 2: 9 bits | nível 1: 9 bits | deslocamento: 12 bits |
 *
 * Cada índice de 9 bits escolhe uma das 512 entradas da tabela daquele nível.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "uso: %s endereço-em-hexadecimal\n", argv[0]);
        return 1;
    }
    /* strtoull com base 16 aceita o endereço com ou sem o prefixo 0x. */
    uint64_t v = strtoull(argv[1], NULL, 16);

    /* TODO (Atividade 6, Passo 6): extraia os quatro índices de 9 bits e o
       deslocamento de 12 bits com >> e &, e imprima-os. */
    printf("endereço %#llx: (a implementar)\n", (unsigned long long)v);
    return 0;
}
