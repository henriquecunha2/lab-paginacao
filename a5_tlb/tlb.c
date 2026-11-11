/*
 * tlb.c — simulador de TLB (Atividade 5).
 *
 * A TLB guarda as traduções usadas recentemente. Aqui só interessa saber se
 * a página procurada está ou não na TLB; por isso cada entrada guarda apenas
 * o número da página e um bit de validade.
 *
 * Este arquivo é um esqueleto: complete os pontos marcados com TODO.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define N_ACESSOS 100000        /* acessos gerados em cada padrão */

/* Uma entrada da TLB. */
typedef struct {
    uint32_t pagina;            /* número da página virtual guardada na entrada */
    int      valida;            /* 0 = entrada vazia; 1 = entrada em uso        */
} entrada_t;

static entrada_t *tlb = NULL;   /* vetor com as entradas da TLB                 */
static int  n_entradas;         /* quantas entradas a TLB tem                   */
static int  proxima;            /* entrada que será substituída na próxima falha (rodízio) */
static long acertos, falhas;    /* contadores                                   */

/* Cria uma TLB vazia com n entradas e zera os contadores. */
static void esvazia(int n) {
    free(tlb);
    n_entradas = n;
    tlb        = calloc(n, sizeof *tlb);    /* calloc zera: todas as entradas inválidas */
    proxima    = 0;
    acertos    = falhas = 0;
}

/*
 * Procura a página na TLB.
 *   Acerto: a página está em alguma entrada válida; conta um acerto.
 *   Falha : não está; conta uma falha e grava a página na entrada "proxima",
 *           avançando o rodízio (depois da última entrada, volta à primeira).
 */
static void consulta(uint32_t pagina) {
    /* TODO (Atividade 5, Passo 1). */
    (void)pagina; (void)proxima;            /* remova esta linha ao implementar */
}

/*
 * Devolve o endereço virtual do acesso i no padrão dado ('A' a 'D').
 * Os padrões estão na tabela do Passo 2 do roteiro.
 */
static uint32_t endereco(char padrao, long i) {
    switch (padrao) {
    case 'A': return (uint32_t)((i * 4) % (64 * 4096));     /* vetor de int em ordem */
    /* TODO (Atividade 5, Passo 2): padrões B, C e D. */
    default:  return 0;
    }
}

int main(void) {
    const int  tamanhos[] = { 8, 64 };          /* Passo 3: TLB de 8 e de 64 entradas */
    const char padroes[]  = { 'A', 'B', 'C', 'D' };

    for (int t = 0; t < 2; t++) {
        printf("TLB com %d entradas\n", tamanhos[t]);
        for (int p = 0; p < 4; p++) {
            esvazia(tamanhos[t]);
            srand(1);                           /* mesma sequência aleatória em toda execução */
            for (long i = 0; i < N_ACESSOS; i++)
                consulta(endereco(padroes[p], i) >> 12);    /* >> 12: endereço -> página */

            double h = (double)acertos / N_ACESSOS;         /* taxa de acertos */
            printf("  padrão %c: acertos %6ld  falhas %6ld  h = %.4f\n",
                   padroes[p], acertos, falhas, h);
            /* TODO (Atividade 5, Passo 4): calcule e imprima T para k = 1 e k = 4. */
        }
    }
    free(tlb);
    return 0;
}
