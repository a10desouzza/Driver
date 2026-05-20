#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elm_driver.h"

void limpar_buffer_entrada(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void obter_caminho_usuario(const char *nome_dado, const char *padrao, char *buffer, size_t tamanho) {
    printf("Digite o caminho do arquivo de %s [\nPadrao: %s]: ", nome_dado, padrao);
    if (fgets(buffer, tamanho, stdin) != NULL) {
        // Remove a quebra de linha obtida pelo fgets
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        // Se o usuário apenas apertou Enter, adota o valor padrão
        if (strlen(buffer) == 0) {
            strncpy(buffer, padrao, tamanho);
        }
    }
}

void imprimir_status(void) {
    int s = ler_status_fpga();
    printf(" -> Status Atual da FPGA: ");
    if (s & ELM_BIT_ERROR) printf("[ERRO] ");
    if (s & ELM_BIT_BUSY)  printf("[BUSY] ");
    if (s & ELM_BIT_DONE)  printf("[DONE] ");
    if ((s & (ELM_BIT_ERROR | ELM_BIT_BUSY | ELM_BIT_DONE)) == 0)
        printf("[OCIOSO/PRONTO] ");
    printf("| Bits DATA_OUT brutos: 0x%X\n", s);
}

int main(void) {
    int ret;
    char path_w[256], path_bt[256], path_bs[256], path_img[256];

    printf("==================================================\n");
    printf("   CONFIGURACAO DE CAMINHOS DE INFERENCIA (C Layer) \n");
    printf("==================================================\n");
    
    obter_caminho_usuario("Pesos (W_in)", "../data/w_in_q.bin", path_w, sizeof(path_w));
    obter_caminho_usuario("Betas (Beta)",  "../data/beta_q.bin", path_bt, sizeof(path_bt));
    obter_caminho_usuario("Bias (Bias)",   "../data/bias_q.bin", path_bs, sizeof(path_bs));
    obter_caminho_usuario("Imagem (.bin)", "../data/image.bin",   path_img, sizeof(path_img));

    printf("\n[1/6] Mapeando Hardware Bridge na DE1-SoC...\n");
    if ((ret = mapear_fpga()) != ELM_OK) {
        fprintf(stderr, "Falha crítica no mmap: %d\n", ret);
        return EXIT_FAILURE;
    }

    printf("[2/6] Aplicando pulso de RESET no co-processador...\n");
    reiniciar_fpga();
    imprimir_status();

    printf("\n[3/6] Enviando Pesos coletados... \nArquivo: %s\n", path_w);
    ret = carregar_pesos(path_w);
    if (ret != ELM_OK) {
        fprintf(stderr, "Erro ao processar arquivo de pesos: %d\n", ret);
        return EXIT_FAILURE;
    }
    printf("Pesos transferidos com sucesso.\n");

    printf("\n[4/6] Enviando Coeficientes Beta... \nArquivo: %s\n", path_bt);
    if ((ret = carregar_beta(path_bt)) != ELM_OK) {
        fprintf(stderr, "Erro ao carregar betas: %d\n", ret);
        return EXIT_FAILURE;
    }

    printf("\n[5/6] Enviando Vetor de Bias... \nArquivo: %s\n", path_bs);
    if ((ret = carregar_bias(path_bs)) != ELM_OK) {
        fprintf(stderr, "Erro ao carregar bias: %d\n", ret);
        return EXIT_FAILURE;
    }

    printf("\n[6/6] Enviando Imagem de entrada... \nArquivo: %s\n", path_img);
    if ((ret = carregar_imagem(path_img)) != ELM_OK) {
        fprintf(stderr, "Erro ao carregar imagem: %d\n", ret);
        return EXIT_FAILURE;
    }

    printf("\n--------------------------------------------------\n");
    printf("Executando inferência da Rede ELM na FPGA...\n");
    iniciar_inferencia();
    imprimir_status();

    printf("Aguardando flag de conclusao do hardware...\n");
    int predicao = obter_resultado();

    printf("\n==================================================\n");
    printf("           RESULTADO DA INFERENCIA               \n");
    printf(" Digito identificado pela FPGA: %d\n", predicao);
    printf("==================================================\n");
    imprimir_status();

    return EXIT_SUCCESS;
}