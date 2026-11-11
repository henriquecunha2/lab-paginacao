/*
 * falta.c — simulador de falta de página (Atividade 3, Parte A).
 *
 * O estado do simulador são as variáveis globais abaixo. O main já prepara
 * o estado inicial e executa a sequência de acessos do roteiro; falta
 * completar a função acessa e a impressão final (Passos 1 e 2).
 */
#include <stdio.h>
#include "mmu.h"
#include "mapas.h"

static pte_t tp[N_PAGINAS];         /* tabela de páginas                              */
static int   dono[N_MOLDURAS];      /* dono[m] = página que ocupa a moldura m         */
static int   proxima_vitima = 0;    /* moldura que será esvaziada na próxima falta    */
static int   faltas     = 0;        /* contador de faltas de página                   */
static int   gravacoes  = 0;        /* contador de páginas modificadas gravadas no disco */
static int   violacoes  = 0;        /* contador de violações de proteção              */

/*
 * Simula um acesso do programa à memória, incluindo o que o SO faz quando a
 * página não está presente.
 *   virt    : endereço virtual acessado
 *   escrita : 0 = leitura; 1 = escrita
 *   fis     : saída; recebe o endereço físico quando o acesso é concluído
 * Retorna 0 se o acesso foi concluído (com ou sem falta de página no caminho),
 * ou -2 se a página não permite o acesso pedido (violação de proteção).
 */
int acessa(uint16_t virt, int escrita, uint16_t *fis) {
    /*
     * TODO (Atividade 3, Passo 1): siga as três regras do roteiro, nesta ordem.
     *   1. Página ausente: conte a falta; a vítima é a moldura proxima_vitima
     *      (avance o rodízio); se a página dona da vítima tem M = 1, conte uma
     *      gravação; marque-a como ausente, com R = M = 0; carregue a página
     *      que faltava nessa moldura e atualize dono[].
     *   2. Proteção: escrita em página sem PROT_E conta uma violação e
     *      retorna -2, sem mexer em R e M.
     *   3. Acesso: R = 1; se for escrita, M = 1; use traduz() para obter *fis.
     * Imprima aqui também as informações da falta pedidas no Passo 2.
     */
    (void)virt; (void)escrita; (void)fis;       /* remova ao implementar */
    (void)proxima_vitima; (void)dono;
    return 0;
}

int main(void) {
    /* Estado inicial: mapa do slide 26, com as 8 molduras ocupadas. */
    carrega(tp, mapa_slide26);
    for (int p = 0; p < N_PAGINAS; p++)         /* monta o vetor dono[] a partir da tabela */
        if (tp[p].presente)
            dono[tp[p].moldura] = p;
    tp[4].prot = PROT_L;                        /* a página 4 é somente leitura */

    /* A sequência de acessos do Passo 1: 'L' = leitura, 'E' = escrita. */
    static const struct { char op; uint16_t virt; } seq[] = {
        {'L',  8196}, {'E',   100}, {'E', 16400}, {'L', 32780}, {'E', 24600},
        {'L',  4100}, {'L',   100}, {'E', 32780}, {'L', 12300}, {'L', 20500},
        {'E', 45100}, {'L',  8200}, {'L', 36900}, {'L', 49200},
    };

    for (unsigned i = 0; i < sizeof seq / sizeof *seq; i++) {
        uint16_t fis = 0;
        printf("%2u %c %5u (página %u)\n", i + 1, seq[i].op,
               (unsigned)seq[i].virt, (unsigned)(seq[i].virt >> BITS_DESLOC));
        if (acessa(seq[i].virt, seq[i].op == 'E', &fis) == -2)
            puts("   TRAP: violação de proteção");
        else
            printf("   endereço físico %u\n", (unsigned)fis);
    }

    printf("faltas de página: %d   gravações no disco: %d   violações de proteção: %d\n",
           faltas, gravacoes, violacoes);

    /* TODO (Atividade 3, Passo 2): imprima a tabela final, uma linha por
       página presente, com página, moldura, R e M. */
    return 0;
}
