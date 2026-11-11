/*
 * paginas.c — reserva espaço de endereçamento virtual e mostra quanto dele
 * vira memória física de verdade.
 *
 * Uso: ./paginas [GiB a reservar] [páginas a tocar]      (padrão: 1 e 100)
 *
 * O programa imprime três medidas do próprio processo em três momentos:
 * antes de reservar, depois de reservar e depois de escrever nas páginas.
 * Comparando os três momentos você vê a ideia do slide 20: só os pedaços
 * em uso ocupam a memória física.
 */
#define _GNU_SOURCE             /* libera MAP_ANONYMOUS, MAP_NORESERVE e madvise */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>             /* sysconf */
#include <sys/mman.h>           /* mmap, madvise */

/*
 * Imprime três linhas de /proc/self/status, o arquivo em que o núcleo
 * descreve o processo que o lê:
 *   VmSize — tamanho do espaço de endereçamento virtual (tudo o que foi reservado);
 *   VmRSS  — memória física ocupada (páginas que têm moldura neste momento);
 *   VmPTE  — memória gasta com as tabelas de páginas do processo.
 */
static void mostra(const char *momento) {
    FILE *f = fopen("/proc/self/status", "r");
    char linha[128];
    printf("%s\n", momento);
    while (fgets(linha, sizeof linha, f))                   /* uma linha por vez   */
        if (!strncmp(linha, "VmSize", 6) || !strncmp(linha, "VmRSS", 5) ||
            !strncmp(linha, "VmPTE", 5))                    /* só as três medidas  */
            printf("    %s", linha);
    fclose(f);
}

int main(int argc, char *argv[]) {
    /* Argumentos da linha de comando, com valores padrão. */
    size_t gib     = argc > 1 ? strtoul(argv[1], NULL, 10) : 1;    /* GiB a reservar  */
    size_t tocadas = argc > 2 ? strtoul(argv[2], NULL, 10) : 100;  /* páginas a tocar */

    /* O SO informa o tamanho da página e quantas molduras a máquina tem. */
    long pagina  = sysconf(_SC_PAGESIZE);
    long fisicas = sysconf(_SC_PHYS_PAGES);

    printf("tamanho da página : %ld bytes\n", pagina);
    printf("memória física    : %ld molduras (%ld MiB)\n",
           fisicas, fisicas / 1024 * pagina / 1024);
    mostra("antes de reservar:");

    /*
     * Reserva "gib" GiB de espaço virtual. O deslocamento de 30 bits
     * multiplica por 2^30, isto é, converte GiB em bytes.
     *   MAP_PRIVATE | MAP_ANONYMOUS : memória só deste processo, sem arquivo por trás;
     *   MAP_NORESERVE               : o SO não precisa garantir memória física nem
     *                                 área de troca agora, o que permite reservar
     *                                 mais do que a máquina tem.
     * Nenhuma moldura é ocupada aqui: o SO apenas anota que a faixa de
     * endereços pertence ao processo.
     */
    size_t tam = gib << 30;
    char *area = mmap(NULL, tam, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (area == MAP_FAILED) { perror("mmap"); return 1; }

    /* Pede páginas do tamanho normal nesta área. Sem isso, o Linux poderia
       usar páginas grandes, de 2 MiB, e as contas do roteiro não fechariam. */
    madvise(area, tam, MADV_NOHUGEPAGE);

    printf("reservados %zu GiB a partir de %p\n", gib, (void *)area);
    mostra("depois de reservar:");

    /*
     * "Toca" as primeiras páginas: escreve um byte no início de cada uma.
     * area[i * pagina] é o primeiro byte da página i da área. O primeiro
     * acesso a cada página gera uma falta de página, e só então o SO
     * entrega uma moldura para ela.
     */
    for (size_t i = 0; i < tocadas; i++)
        area[i * pagina] = 1;
    printf("tocadas %zu páginas\n", tocadas);
    mostra("depois de tocar:");

    /* Um endereço virtual para decompor à mão no Passo 4:
       o byte 20 da página 5 da área. */
    uintptr_t v = (uintptr_t)&area[5 * pagina + 20];
    printf("endereço virtual de exemplo: %#lx\n", (unsigned long)v);

    getchar();      /* mantém o processo vivo até você teclar Enter */
    return 0;
}
