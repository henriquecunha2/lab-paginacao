# Roteiro de Laboratório — Gerência de Memória (Parte 2: Paginação)

## Informações gerais

Neste laboratório você vai observar, simular e implementar em C os mecanismos de paginação vistos nos slides 20 a 32: memória virtual, endereços virtuais e MMU, páginas e molduras, falta de página, tradução de endereços, tamanho e custo das tabelas de páginas, TLB, tabelas multiníveis e os campos de uma entrada da tabela.

| Item | Descrição |
| --- | --- |
| Disciplina | Sistemas Operacionais — Engenharia de Computação, IFPB Campus Campina Grande |
| Conteúdo | Gerência de Memória, slides 20 a 32 (Memória virtual, paginação e tabelas de páginas) |
| Duração estimada | Cerca de 4 h de estudo (Atividades 1 a 3 em uma sessão; 4 a 6 em outra) |
| Pré-requisitos | Parte 1 concluída; Linux (nativo, VM ou WSL), C com ponteiros, `struct` e operações de bits, `gcc` e `make` |
| Trabalho | Individual, como estudo para a avaliação; não há entrega |

**Objetivos.** Ao final, você deve ser capaz de:

- Distinguir memória virtual reservada de memória física ocupada em um processo Linux.
- Decompor um endereço virtual em número de página e deslocamento e traduzi-lo com uma tabela de páginas.
- Descrever, passo a passo, o tratamento de uma falta de página e o papel dos bits presente/ausente, R, M e de proteção.
- Calcular o tamanho de uma tabela de páginas e o custo de cada forma de guardá-la.
- Medir a taxa de acertos de uma TLB e relacioná-la à localidade dos acessos.
- Implementar uma tabela de páginas de dois níveis e dizer quando ela economiza memória.

As perguntas marcadas com **Q** servem para você conferir o que entendeu: responda cada uma por escrito, apoiando-se na saída do seu programa, antes de passar à atividade seguinte.

## Preparação do ambiente

Use o mesmo ambiente da Parte 1: uma distribuição Linux com `gcc` e `make`. O Desafio exige `sudo`. Clone este repositório; ele já traz a estrutura abaixo:

```text
lab-paginacao/
├── a1_paginas/      paginas.c
├── a2_a3_mmu/       mmu.h  mapas.h  mmu.c  traducao.c  falta.c  faltas_linux.c
├── a5_tlb/          tlb.c
├── a6_multinivel/   multinivel.h  multinivel.c  indices.c
├── desafio/         pagemap.c
├── Makefile
└── README.md        este roteiro
```

Os programas `paginas.c`, `faltas_linux.c` e `pagemap.c` estão completos: leia os comentários, compile e execute. Os demais arquivos `.c` são esqueletos comentados, com os pontos a completar marcados com `TODO`. O comando `make`, na raiz, compila tudo com `gcc -Wall -Wextra -g` e deixa cada executável na pasta da sua atividade; os esqueletos compilam desde o início, mas só produzem os resultados corretos depois de completados.

Nas Atividades 2 e 3, a máquina simulada é a dos slides 23 a 26: endereços virtuais de 16 bits, endereços físicos de 15 bits e páginas de 4 KB, ou seja, 16 páginas virtuais e 8 molduras. A Atividade 4 não tem código novo: usa o programa da Atividade 1.

## Atividade 1 — Memória virtual, páginas e molduras no Linux (slides 20–24)

A memória virtual mantém na memória física apenas os pedaços do programa em uso (slide 20). Antes de simular a tradução, você vai ver essa diferença em um processo real: quanto ele reservou de espaço virtual e quantas molduras ele de fato ocupa.

**Passo 1.** Leia `a1_paginas/paginas.c`, reproduzido abaixo, e compile-o com `make`:

```c
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
```

O primeiro argumento é quanto espaço virtual reservar, em GiB; o segundo, em quantas páginas dessa área escrever um byte. `VmSize` é o tamanho do espaço virtual do processo, `VmRSS` é a memória física que ele ocupa e `VmPTE` é a memória gasta com as suas tabelas de páginas.

**Passo 2.** Execute `./paginas 1 100` e guarde a saída. Confira o tamanho da página com `getconf PAGESIZE`.

**Passo 3.** Execute de novo reservando mais espaço virtual do que a memória física da máquina; por exemplo, `./paginas 64 100` em uma máquina com 8 GiB.

**Passo 4.** Com o programa parado no `getchar()`, decomponha à mão o endereço virtual de exemplo em número de página e deslocamento, como no slide 24.

**Q1.1** Qual o tamanho da página na sua máquina? Quantos bits do endereço formam o deslocamento? Por que o slide 24 exige que o tamanho seja uma potência de 2?

**Q1.2** Compare `VmSize` e `VmRSS` nos três momentos do Passo 2. Qual deles cresceu ao reservar e qual cresceu ao tocar? Em quantos kB, e a quantas páginas isso corresponde?

**Q1.3** O Passo 3 funcionou? Relacione com a situação do slide 20: um programa de 16 MB em uma memória de 4 MB. O que aconteceria se o programa tocasse todas as páginas reservadas?

**Q1.4** Mostre a conta do Passo 4, em hexadecimal. O deslocamento obtido é o esperado para `&area[5 * pagina + 20]`? Por quê?

**Q1.5** O endereço impresso pelo programa é virtual ou físico? Que componente do slide 22 o converte, e por que o programa não consegue ver o resultado dessa conversão?

## Atividade 2 — Tradução de endereços pela MMU (slides 23, 24 e 26)

Você vai implementar a operação interna da MMU do slide 26: separar o endereço virtual em número de página e deslocamento, consultar a tabela de páginas e montar o endereço físico. A interface está em `a2_a3_mmu/mmu.h`, reproduzida abaixo; ela será reaproveitada na Atividade 3.

```c
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
```

Os slides trazem dois mapeamentos diferentes para o mesmo espaço de 16 páginas. Os dois estão declarados em `a2_a3_mmu/mapas.h`:

```c
/*
 * Cada vetor descreve uma tabela de páginas inteira:
 *   posição = número da página virtual (0 a 15);
 *   valor   = moldura em que a página está, ou -1 se está ausente (o X da figura).
 *
 * mapa_slide23 vem da figura do slide 23, lida de baixo para cima (a página 0
 * é a faixa 0K–4K); os exemplos do slide 24 usam esse mapa.
 * mapa_slide26 vem da tabela desenhada dentro da MMU no slide 26.
 */
/*                 página:                   0  1  2  3   4  5   6  7   8   9  10  11  12  13  14  15 */
static const int mapa_slide23[N_PAGINAS] = { 1, 6, 0, 2, -1, 7, -1, 5, -1, -1,  3, -1, -1,  4, -1, -1 };
static const int mapa_slide26[N_PAGINAS] = { 2, 1, 6, 0,  4, 3, -1, -1, -1,  5, -1,  7, -1, -1, -1, -1 };
```

**Passo 1.** Implemente `traduz` e `carrega` em `mmu.c`. Em `traduz`, use apenas deslocamentos e máscaras de bits (`>>`, `<<`, `&`, `|`), sem divisão nem resto: é assim que o hardware faz.

**Passo 2.** Em `traducao.c`, complete a função `mostra_traducao`, que imprime, para cada endereço: o endereço virtual em decimal e em binário (4 bits de página, espaço, 12 bits de deslocamento), a página, o deslocamento e, se houver tradução, a moldura e o endereço físico em binário (3 + 12 bits) e em decimal; caso contrário, `FALTA DE PÁGINA`. A linha do exemplo do slide 26 deve sair assim:

```text
 8196 = 0010 000000000100  página  2 desloc    4 -> moldura 6  110 000000000100 = 24580
```

**Passo 3.** Com `mapa_slide26`, traduza `0, 8192, 8196, 20500, 4095, 4096, 32780, 45100`. Com `mapa_slide23`, traduza `8192, 8196, 20500, 16500, 4095, 4096`. Confira os resultados com os exemplos dos slides 23, 24 e 26.

**Q2.1** O endereço virtual 8196 vai para o endereço físico 4 no slide 24 e para 24580 no slide 26. Os dois estão certos? O que isso mostra sobre a quem pertence o mapeamento: ao endereço ou à tabela de páginas?

**Q2.2** Os endereços virtuais 4095 e 4096 são vizinhos. Os endereços físicos correspondentes também são, em cada um dos dois mapeamentos? Que liberdade isso dá ao SO na hora de escolher molduras, em comparação com as partições da Parte 1?

**Q2.3** O slide 26 diz que os 12 bits de deslocamento são copiados diretamente. Mostre, na sua saída, que isso acontece em todas as traduções. Como ficaria a divisão dos 16 bits com páginas de 1 KB, e quantas entradas a tabela teria?

**Q2.4** Nesta máquina há 16 páginas e 8 molduras. Quantas páginas podem estar presentes ao mesmo tempo? Quais endereços do Passo 3 geraram falta de página, e o que a MMU faz nesse caso (slide 25)?

## Atividade 3 — Falta de página e os bits da entrada da tabela (slides 25 e 32)

Você vai implementar o tratamento da falta de página descrito no slide 25, usando os campos da entrada da tabela do slide 32, e depois contar as faltas de página de um processo real.

**Parte A — simulador.** Em `a2_a3_mmu/falta.c`, complete a função abaixo sobre a `mmu.h` da Atividade 2:

```c
/*
 * Simula um acesso do programa à memória, incluindo o que o SO faz quando a
 * página não está presente.
 *   virt    : endereço virtual acessado
 *   escrita : 0 = leitura; 1 = escrita
 *   fis     : saída; recebe o endereço físico quando o acesso é concluído
 * Retorna 0 se o acesso foi concluído (com ou sem falta de página no caminho),
 * ou -2 se a página não permite o acesso pedido (violação de proteção).
 */
int acessa(uint16_t virt, int escrita, uint16_t *fis);
```

O estado do simulador é a tabela de páginas, um vetor `dono[N_MOLDURAS]` com a página que ocupa cada moldura, o índice da próxima moldura vítima e três contadores: faltas de página, gravações no disco e violações de proteção. `acessa` segue estas regras, nesta ordem:

1. **Página ausente: falta de página.** Conte a falta. A moldura vítima é escolhida em rodízio: 0 na primeira falta, 1 na segunda, e assim por diante, voltando a 0 depois da 7. Se a página que ocupa a vítima estiver com M = 1, conte uma gravação no disco. Marque essa página como ausente, com R = M = 0. Carregue a página que faltava na moldura, com presente = 1 e R = M = 0, e atualize `dono`. Depois continue no item 2: é a reexecução da instrução.
2. **Proteção.** Se o acesso é de escrita e a página não tem `PROT_E`, conte a violação e retorne -2, sem alterar R nem M.
3. **Acesso.** Faça R = 1 e, se for escrita, M = 1. Obtenha o endereço físico com `traduz`.

O rodízio é uma simplificação proposital: escolher bem a vítima é o assunto dos slides 33 em diante.

**Passo 1.** Comece com `mapa_slide26` (as 8 molduras ocupadas, R = M = 0) e torne a página 4 somente leitura (`prot = PROT_L`). Execute os acessos abaixo, em que L é leitura e E é escrita:

| # | Acesso | # | Acesso |
| --- | --- | --- | --- |
| 1 | L 8196 | 8 | E 32780 |
| 2 | E 100 | 9 | L 12300 |
| 3 | E 16400 | 10 | L 20500 |
| 4 | L 32780 | 11 | E 45100 |
| 5 | E 24600 | 12 | L 8200 |
| 6 | L 4100 | 13 | L 36900 |
| 7 | L 100 | 14 | L 49200 |

**Passo 2.** Para cada acesso, imprima a página; se houve falta, a moldura vítima, a página retirada e se ela foi gravada no disco; e o endereço físico ou `TRAP: violação de proteção`. Ao final, imprima os três contadores e a tabela de páginas (página, moldura, R e M das páginas presentes).

**Parte B — faltas de página reais.** Leia `a2_a3_mmu/faltas_linux.c`, reproduzido abaixo:

```c
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
```

**Passo 3.** Execute `./faltas_linux` e depois `/usr/bin/time -v ./faltas_linux`. Na saída do `time`, localize as linhas *Minor page faults* e *Major page faults*. Uma falta é *major* quando a página precisa ser lida do disco, como no slide 25, e *minor* quando o SO resolve sem E/S; por exemplo, entregando uma moldura zerada no primeiro acesso.

**Q3.1** Em quais acessos do Passo 1 a página retirada foi gravada no disco? Por que as outras vítimas puderam ser descartadas sem gravação? Que bit do slide 32 decide isso?

**Q3.2** A página 1 estava na memória no início e, mesmo assim, o acesso 6 gerou falta. O que aconteceu com ela? Quantas das 8 faltas atingiram páginas que já tinham estado na memória? Que bit do slide 32 ajudaria o SO a escolher melhor a vítima?

**Q3.3** O acesso 3 não é uma falta de página: a página 4 estava presente. Que campo da entrada barrou o acesso? Retome a Q4.4 da Parte 1: o que a paginação oferece que base e limite não ofereciam?

**Q3.4** O slide 25 diz que a instrução que causou a falta é executada de novo. Em que ponto do seu `acessa` isso aparece? Por que o programa do usuário não percebe que a falta ocorreu?

**Q3.5** Na Parte B, quantas faltas houve em cada passada? Explique a diferença. Elas foram *minor* ou *major*? Qual dos passos do slide 25 não foi necessário nessas faltas?

## Atividade 4 — Tamanho e custo das tabelas de páginas (slides 27–29)

O slide 27 aponta dois problemas: a tabela de páginas pode ser muito grande e o mapeamento precisa ser rápido. Nesta atividade você vai quantificar os dois, com contas e com uma medição no Linux. Não há código novo.

**Passo 1.** Preencha a tabela abaixo, supondo uma tabela de um único nível, com uma entrada para cada página virtual. Use potências de 2 e a unidade mais conveniente (KiB, MiB, GiB…).

| Máquina | Endereço virtual | Página | Entrada | Número de entradas | Tamanho da tabela por processo |
| --- | --- | --- | --- | --- | --- |
| Simulador das Atividades 2 e 3 | 16 bits | 4 KB | 1 B |  |  |
| 32 bits (slide 27) | 32 bits | 4 KB | 4 B |  |  |
| x86-64 na prática (slide 27) | 48 bits | 4 KB | 8 B |  |  |
| 64 bits completos (slide 27) | 64 bits | 4 KB | 8 B |  |  |

**Passo 2.** Execute o programa da Atividade 1 duas vezes, `./paginas 1 100` e `./paginas 1 50000`, e anote `VmPTE` depois de tocar as páginas em cada execução. A segunda execução ocupa cerca de 200 MiB de memória física; se a sua VM tiver pouca memória, use 20000.

**Q4.1** Confira os números do slide 27 com a sua tabela do Passo 1. Com 100 processos em execução, quanta memória as tabelas da máquina de 32 bits ocupariam? E a de 48 bits, caberia em algum computador?

**Q4.2** No Passo 2, quanto `VmPTE` cresceu de uma execução para a outra? Mostre que o crescimento corresponde a uma entrada de 8 bytes por página tocada. O processo reservou 1 GiB nas duas execuções: por que `VmPTE` não depende do espaço reservado? Guarde a resposta para a Atividade 6.

**Q4.3** Pelo slide 28, se uma instrução leva 4 ns, a consulta à tabela deve levar menos de 1 ns. Suponha a tabela guardada na memória (slide 29) e um acesso à memória de 10 ns. Quanto tempo leva cada referência à memória feita pelo programa? A meta do slide 28 é atendida?

**Q4.4** Agora suponha a outra opção do slide 29: a tabela inteira da máquina de 32 bits é carregada em registradores a cada troca de contexto, à mesma velocidade da Q5.3 da Parte 1 (8 bytes a cada 4 ns). Quanto tempo leva cada troca? Com 100 trocas por segundo, que fração do tempo da CPU é gasta só nisso?

**Q4.5** Monte um quadro com as duas opções do slide 29, indicando o custo de cada uma na tradução e na troca de contexto. Qual dos dois problemas do slide 27 cada um dos slides 30 e 31 ataca?

## Atividade 5 — TLB e localidade (slide 30)

Você vai simular a TLB do slide 30 e medir como a taxa de acertos depende do padrão de acessos do programa. Aqui só interessa se a tradução está ou não na TLB; por isso o simulador não precisa da tabela de páginas.

**Passo 1.** Em `a5_tlb/tlb.c`, implemente uma TLB com `n` entradas, cada uma com o número da página e um bit de validade. A função `consulta(pagina)` procura a página em todas as entradas: se achar, conta um acerto; se não, conta uma falha e grava a página na TLB, substituindo as entradas em rodízio (a mais antiga sai primeiro).

**Passo 2.** Gere os quatro padrões abaixo, cada um com 100 000 acessos a endereços virtuais `v` em páginas de 4 KB (`pagina = v >> 12`). Esvazie a TLB e chame `srand(1)` antes de cada padrão.

| Padrão | Endereço do acesso `i` | O que representa |
| --- | --- | --- |
| A | `(i * 4) % (64 * 4096)` | Percorrer em ordem um vetor de `int` que ocupa 64 páginas |
| B | `(i * 4096) % (64 * 4096)` | Ler um único elemento de cada página do mesmo vetor |
| C | `(rand() % 4) * 4096 + rand() % 4096` | Acessos aleatórios confinados a 4 páginas |
| D | `(rand() % 64) * 4096 + rand() % 4096` | Acessos aleatórios espalhados por 64 páginas |

**Passo 3.** Execute os quatro padrões com uma TLB de 8 entradas e depois com uma de 64. Para cada caso, imprima acertos, falhas e a taxa de acertos *h*.

**Passo 4.** Calcule o tempo efetivo de acesso de cada caso, supondo 1 ns para consultar a TLB, 10 ns por acesso à memória e *k* acessos extras à memória para consultar a tabela de páginas a cada falha:

```math
T = 1 + 10 + (1 - h) \cdot k \cdot 10 \quad \text{(ns)}
```

Use *k* = 1, que corresponde à tabela de um nível guardada na memória (slide 29), e *k* = 4, que corresponde aos quatro níveis do x86-64. Apresente tudo em uma tabela com um padrão por linha.

**Q5.1** Os padrões A e B percorrem o mesmo vetor. Por que a taxa de acertos é tão diferente com 8 entradas? Deduza, sem o programa, o número de falhas do padrão A.

**Q5.2** Os padrões C e D são igualmente aleatórios. O que explica a diferença entre eles? Relacione com a frase do slide 30 sobre localidade e estime, por probabilidade, a taxa de acertos do padrão D com 8 entradas.

**Q5.3** O que muda com 64 entradas? Que relação entre o número de entradas e o número de páginas em uso torna a TLB eficaz?

**Q5.4** Compare os tempos do Passo 4 com os 10 ns de um acesso sem paginação. Em que casos a TLB resolve o segundo problema do slide 27? Por que as falhas pesam mais com *k* = 4?

**Q5.5** As entradas da TLB valem para a tabela de páginas de um processo. O que o SO precisa fazer com a TLB em uma troca de contexto? Como isso muda a comparação entre as duas opções do slide 29?

## Atividade 6 — Tabela de páginas de dois níveis (slide 31)

Você vai implementar a tabela de dois níveis do slide 31, reproduzir a conta "1 + 3 tabelas" e descobrir quando essa organização deixa de economizar memória. O endereço virtual tem 32 bits: 10 de PT1, 10 de PT2 e 12 de deslocamento. Cada tabela tem 1024 entradas de 4 bytes, ou seja, ocupa exatamente uma página de 4 KiB.

```c
/*
 * Tabela de páginas de dois níveis (slide 31), para endereços de 32 bits:
 *
 *   | PT1: 10 bits | PT2: 10 bits | deslocamento: 12 bits |
 *
 * PT1 escolhe uma entrada da tabela de 1º nível; essa entrada aponta para uma
 * tabela de 2º nível; PT2 escolhe, dentro dela, a entrada da página.
 * Cada tabela de 2º nível cobre 1024 páginas de 4 KB, isto é, 4 MB.
 */
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
```

**Passo 1.** Implemente as duas funções em `a6_multinivel/multinivel.c`. Uma tabela de 2º nível só é alocada (`calloc`) no primeiro `mapeia` que cair na sua região de 4 MB.

**Passo 2.** Monte o processo do slide 31: 4 MB de código a partir de `0x00000000`, 4 MB de dados a partir de `0x00400000` e 4 MB de pilha no topo, de `0xFFC00000` a `0xFFFFFFFF`. Mapeie todas as páginas, nessa ordem, em molduras consecutivas a partir da moldura 256. Imprima o número de tabelas e a memória que elas ocupam.

**Passo 3.** Traduza `0x00403004`, `0x00000FFF`, `0xFFFFFFFC`, `0x00800000` e `0x7FFF1234`, imprimindo PT1, PT2, deslocamento e o endereço físico ou `FALTA DE PÁGINA`.

**Passo 4.** Os dados crescem para 5 MB: mapeie mais 256 páginas a partir de `0x00800000`. Quantas tabelas existem agora?

**Passo 5.** Crie outra tabela, vazia, e mapeie 1024 páginas espalhadas: a primeira página de cada região de 4 MB (`virt = r << 22`, com `r` de 0 a 1023). Imprima o número de tabelas e a memória ocupada.

**Passo 6.** No x86-64, os 48 bits do endereço virtual se dividem em quatro índices de 9 bits e 12 bits de deslocamento. Complete `a6_multinivel/indices.c`, que recebe um endereço em hexadecimal e imprime os quatro índices e o deslocamento. Teste com `0x7ffd3a5c1e48` e depois com o endereço de exemplo impresso pelo programa da Atividade 1.

**Q6.1** Quantas tabelas o Passo 2 criou e quanta memória elas ocupam? Compare com a tabela de um nível da Atividade 4 para a máquina de 32 bits. O resultado confere com o slide 31?

**Q6.2** No Passo 3, dois endereços geraram falta. Em que nível a busca parou em cada um, e o que isso poupou à MMU? Quantas entradas de 1º nível estão vazias, e quanto espaço virtual cada entrada vazia deixa de mapear?

**Q6.3** Quantas tabelas o Passo 4 acrescentou para 1 MB a mais de dados? Quanto os dados ainda podem crescer sem criar outra tabela?

**Q6.4** No Passo 5, o processo usa apenas 4 MB de memória. Quanto ocupam as suas tabelas? Compare com a tabela de um nível e diga que propriedade dos programas reais faz a tabela multinível valer a pena.

**Q6.5** Retome a Q4.2: por que `VmPTE` cresceu com as páginas tocadas e não com o espaço reservado?

**Q6.6** Com quatro níveis, quantos acessos à memória uma tradução exige quando a TLB falha? A tabela multinível resolve o problema do tamanho; que preço ela cobra no outro problema do slide 27, e que mecanismo o compensa?

## Desafio — A tradução real, vista pelo `/proc` (slides 23–26)

O desafio é opcional e mostra, em um processo real, o bit presente/ausente e o número da moldura que você simulou nas Atividades 2 e 3. O Linux expõe em `/proc/self/pagemap` uma entrada de 64 bits para cada página virtual do processo: o bit 63 indica se a página está presente e os bits 0 a 54 trazem o número da moldura. O número da moldura só é revelado ao administrador; por isso, execute com `sudo`.

**Passo 1.** Leia `desafio/pagemap.c`, reproduzido abaixo, compile e execute com `sudo ./pagemap`:

```c
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
```

**Passo 2.** Execute também sem `sudo` e compare as duas saídas.

**Passo 3.** Acrescente uma variável global e uma variável local ao programa e mostre a tradução do endereço de cada uma.

**Q.D1** Qual o valor do bit presente antes e depois de tocar a página? Que evento da Atividade 3 aconteceu entre as duas leituras?

**Q.D2** As quatro páginas são vizinhas no espaço virtual. As molduras também são vizinhas, e estão na mesma ordem? Relacione com a Q2.2. Confira ainda que os 12 bits finais dos endereços virtual e físico são iguais.

**Q.D3** O que muda na saída sem `sudo`? Por que o núcleo esconde o número da moldura de um processo comum?
