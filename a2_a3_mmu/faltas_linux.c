/*
 * faltas_linux.c — conta as faltas de página que o próprio processo sofre.
 *
 * Uso: ./faltas_linux [número de páginas]                (padrão: 10000)
 *
 * O programa reserva n páginas e escreve um byte em cada uma, duas vezes.
 * Antes e depois de cada passada, pergunta ao núcleo quantas faltas de página
 * o processo já teve; a diferença é o número de faltas daquela passada.
 */
#define _GNU_SOURCE             /* libera MAP_ANONYMOUS e madvise */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>             /* sysconf */
#include <sys/mman.h>           /* mmap, madvise */
#include <sys/resource.h>       /* getrusage */

/*
 * Devolve o total de faltas de página "menores" (minor) sofridas pelo
 * processo desde que começou. getrusage preenche uma estrutura com o consumo
 * de recursos do processo; ru_minflt é o contador das faltas resolvidas sem
 * leitura do disco. (As que exigem leitura do disco ficam em ru_majflt.)
 */
static long faltas_menores(void) {
    struct rusage u;
    getrusage(RUSAGE_SELF, &u);         /* RUSAGE_SELF = este processo */
    return u.ru_minflt;
}

int main(int argc, char *argv[]) {
    size_t n      = argc > 1 ? strtoul(argv[1], NULL, 10) : 10000;  /* páginas */
    long   pagina = sysconf(_SC_PAGESIZE);                          /* bytes por página */

    /* Reserva n páginas de espaço virtual. Como na Atividade 1, nenhuma
       moldura é entregue neste momento. */
    char *area = mmap(NULL, n * pagina, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (area == MAP_FAILED) { perror("mmap"); return 1; }

    /* Páginas do tamanho normal: com páginas grandes, uma única falta
       traria 512 páginas de uma vez e a contagem ficaria distorcida. */
    madvise(area, n * pagina, MADV_NOHUGEPAGE);

    for (int passada = 1; passada <= 2; passada++) {
        long antes = faltas_menores();              /* contador antes da passada */

        /* Um acesso de escrita por página: area[i * pagina] é o primeiro
           byte da página i. */
        for (size_t i = 0; i < n; i++)
            area[i * pagina] = (char)passada;

        /* A diferença dos contadores é o número de faltas desta passada. */
        printf("passada %d: %ld faltas de página em %zu páginas\n",
               passada, faltas_menores() - antes, n);
    }
    return 0;
}
