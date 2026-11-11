/*
 * pagemap.c — mostra a tradução que o Linux fez para páginas deste processo.
 *
 * /proc/self/pagemap é um arquivo binário mantido pelo núcleo. Ele tem uma
 * entrada de 64 bits (8 bytes) para cada página virtual do processo, na ordem
 * dos números de página. Dentro de cada entrada:
 *   bit 63      : 1 se a página está presente na memória física;
 *   bits 0 a 54 : número da moldura (PFN, page frame number), quando presente.
 * O núcleo só entrega o número da moldura ao administrador; para um usuário
 * comum, esse campo vem zerado.
 */
#define _GNU_SOURCE             /* libera MAP_ANONYMOUS e pread */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>              /* open */
#include <unistd.h>             /* pread, sysconf */
#include <sys/mman.h>           /* mmap */

/*
 * Lê a entrada do pagemap que descreve a página do endereço virtual v.
 *   v / pagina       : número da página virtual (a mesma conta do slide 24);
 *   (v / pagina) * 8 : posição da entrada no arquivo, pois cada uma tem 8 bytes.
 * pread lê sizeof e bytes a partir dessa posição, sem mover o cursor do arquivo.
 */
static uint64_t entrada(int fd, uintptr_t v, long pagina) {
    uint64_t e = 0;
    if (pread(fd, &e, sizeof e, (off_t)(v / pagina) * 8) != sizeof e)
        perror("pread");
    return e;
}

/* Imprime o que o pagemap diz sobre a página do ponteiro p. */
static void mostra(int fd, const char *nome, char *p, long pagina) {
    uint64_t e = entrada(fd, (uintptr_t)p, pagina);

    /* Bit presente/ausente: desloca o bit 63 para a posição 0 e isola-o. */
    int presente = (int)(e >> 63) & 1;

    /* Número da moldura: a máscara (1 << 55) - 1 tem os 55 bits de baixo em 1
       e, com o &, descarta os bits 55 a 63. */
    uint64_t pfn = e & ((1ULL << 55) - 1);

    printf("%-12s virtual %p  presente=%d", nome, (void *)p, presente);
    if (presente)
        /* Endereço físico = início da moldura + deslocamento dentro da página;
           o deslocamento é o resto da divisão do endereço virtual pela página. */
        printf("  moldura %#lx  físico %#lx", (unsigned long)pfn,
               (unsigned long)(pfn * pagina + (uintptr_t)p % pagina));
    putchar('\n');
}

int main(void) {
    long pagina = sysconf(_SC_PAGESIZE);                /* bytes por página */
    int  fd     = open("/proc/self/pagemap", O_RDONLY);
    if (fd < 0) { perror("pagemap"); return 1; }

    /* Reserva 4 páginas vizinhas no espaço virtual; nenhuma tem moldura ainda. */
    char *area = mmap(NULL, 4 * pagina, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (area == MAP_FAILED) { perror("mmap"); return 1; }

    /* Consulta antes de qualquer acesso: a página ainda está ausente. */
    mostra(fd, "antes:", area + 20, pagina);

    /* Escreve um byte em cada página: cada escrita gera uma falta de página
       e o SO entrega uma moldura. */
    for (int i = 0; i < 4; i++)
        area[i * pagina] = 1;

    /* Consulta de novo, agora o byte 20 de cada uma das 4 páginas. */
    for (int i = 0; i < 4; i++) {
        char nome[16];
        snprintf(nome, sizeof nome, "página %d:", i);
        mostra(fd, nome, area + i * pagina + 20, pagina);
    }
    return 0;
}
