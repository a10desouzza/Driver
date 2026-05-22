#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "elm_driver.h"

#define PATH_WEIGHTS "../data/w_in_q.bin"
#define PATH_BETA    "../data/beta_q.bin"
#define PATH_BIAS    "../data/bias_q.bin"
#define PASTA_IMAGE  "../data/image"

void imprimir_status(void) {
    int s = ler_status_fpga();
    printf("[STATUS HW] ");
    if (s & ELM_BIT_ERROR) printf("[ERRO] ");
    if (s & ELM_BIT_BUSY)  printf("[BUSY] ");
    if (s & ELM_BIT_DONE)  printf("[DONE] ");
    if ((s & (ELM_BIT_ERROR | ELM_BIT_BUSY | ELM_BIT_DONE)) == 0)
        printf("[LIVRE/OCIOSO] ");
    printf("| DATA_OUT: 0x%02X\n", s);
}

void processar_pasta_recursiva(const char *pasta, int *total_processado) {
    DIR *dir = opendir(pasta);
    if (!dir) return;

    struct dirent *entrada;
    char caminho_completo[512];

    while ((entrada = readdir(dir)) != NULL) {
        if (entrada->d_name[0] == '.') continue; 

        snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", pasta, entrada->d_name);

        if (entrada->d_type == DT_DIR) {
            processar_pasta_recursiva(caminho_completo, total_processado);
        } 
        else if (entrada->d_type == DT_REG) {
            printf("\n[*] Processando: %s\n", caminho_completo);
            
            int ret = carregar_imagem(caminho_completo);
            if (ret != ELM_OK) {
                printf("    [-] Erro ao carregar (Codigo: %d).\n", ret);
                continue;
            }
            
            iniciar_inferencia();
            int resultado = obter_resultado();
            printf("    [+] >> DIGITO PREDITO PELA FPGA: %d <<\n", resultado);
            
            reiniciar_fpga();
            (*total_processado)++;
        }
    }
    closedir(dir);
}

int main(void) {
    int ret;
    int total_batch = 0;

    printf("==================================================\n");
    printf("   TESTBENCH AUTOMATIZADO - ELM CO-PROCESSOR      \n");
    printf("==================================================\n");

    if (mapear_fpga() != ELM_OK) {
        printf("[-] ERRO FATAL: Falha no mapeamento /dev/mem.\n");
        return EXIT_FAILURE;
    }

    reiniciar_fpga();
    
    printf("\n[TESTE 1] Protecao contra comando precoce e operacao invalida\n");
    iniciar_inferencia();
    imprimir_status();
    
    int status_lixo = obter_resultado();
    printf("    [+] >> DIGITO PREDITO PELA FPGA (Lixo): %d <<\n", status_lixo & ELM_MASK_DIGIT);

    ret = carregar_pesos(PATH_WEIGHTS);
    if (ret == ELM_ERR_FPGA) {
        printf("[+] Sucesso: O hardware bloqueou a escrita e o driver reportou o erro (-99).\n");
    }
    imprimir_status();
    
    reiniciar_fpga();

    printf("\n[TESTE 2] Carga de parametros da Rede Neural\n");
    printf("-> Enviando Pesos...\n");
    carregar_pesos(PATH_WEIGHTS);
    printf("-> Enviando Betas...\n");
    carregar_beta(PATH_BETA);
    printf("-> Enviando Bias...\n");
    carregar_bias(PATH_BIAS);
    
    printf("[+] Parametros base carregados com sucesso.\n");

    printf("\n==================================================\n");
    printf("   [TESTE 3] INFERENCIA RECURSIVA EM LOTE         \n");
    printf("==================================================\n");
    
    processar_pasta_recursiva(PASTA_IMAGE, &total_batch);
    
    printf("\n==================================================\n");
    printf("   FIM DO TESTBENCH: %d imagens inferidas na FPGA.\n", total_batch);
    printf("==================================================\n");

    return EXIT_SUCCESS;
}#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "elm_driver.h"

#define PATH_WEIGHTS "../data/w_in_q.bin"
#define PATH_BETA    "../data/beta_q.bin"
#define PATH_BIAS    "../data/bias_q.bin"
#define PASTA_IMAGE  "../data/image"

void imprimir_status(void) {
    int s = ler_status_fpga();
    printf("[STATUS HW] ");
    if (s & ELM_BIT_ERROR) printf("[ERRO] ");
    if (s & ELM_BIT_BUSY)  printf("[BUSY] ");
    if (s & ELM_BIT_DONE)  printf("[DONE] ");
    if ((s & (ELM_BIT_ERROR | ELM_BIT_BUSY | ELM_BIT_DONE)) == 0)
        printf("[LIVRE/OCIOSO] ");
    printf("| DATA_OUT: 0x%02X\n", s);
}

void processar_pasta_recursiva(const char *pasta, int *total_processado) {
    DIR *dir = opendir(pasta);
    if (!dir) return;

    struct dirent *entrada;
    char caminho_completo[512];

    while ((entrada = readdir(dir)) != NULL) {
        if (entrada->d_name[0] == '.') continue; 

        snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", pasta, entrada->d_name);

        if (entrada->d_type == DT_DIR) {
            processar_pasta_recursiva(caminho_completo, total_processado);
        } 
        else if (entrada->d_type == DT_REG) {
            printf("\n[*] Processando: %s\n", caminho_completo);

            int ret = carregar_imagem(caminho_completo);
            if (ret != ELM_OK) {
                printf("    [-] Erro ao carregar (Codigo: %d).\n", ret);
                continue;
            }

            iniciar_inferencia();
            int resultado = obter_resultado();
            printf("    [+] >> DIGITO PREDITO PELA FPGA: %d <<\n", resultado);
            
            reiniciar_fpga();
            (*total_processado)++;
        }
    }
    closedir(dir);
}

int main(void) {
    int ret;
    int total_batch = 0;

    printf("==================================================\n");
    printf("   TESTBENCH AUTOMATIZADO - ELM CO-PROCESSOR      \n");
    printf("==================================================\n");

    if (mapear_fpga() != ELM_OK) {
        printf("[-] ERRO FATAL: Falha no mapeamento /dev/mem.\n");
        return EXIT_FAILURE;
    }

    reiniciar_fpga();
    
    printf("\n[TESTE 1] Protecao contra comando precoce e operacao invalida\n");
    iniciar_inferencia();
    imprimir_status();
    
    int status_lixo = ler_status_fpga();
    printf("    [+] >> DIGITO PREDITO PELA FPGA (Lixo): %d <<\n", status_lixo & ELM_MASK_DIGIT);

    ret = carregar_pesos(PATH_WEIGHTS);
    if (ret == ELM_ERR_FPGA) {
        printf("[+] Sucesso: O hardware bloqueou a escrita e o driver reportou o erro (-99).\n");
    }
    imprimir_status();
    
    reiniciar_fpga();

    printf("\n[TESTE 2] Carga de parametros da Rede Neural\n");
    printf("-> Enviando Pesos...\n");
    carregar_pesos(PATH_WEIGHTS);
    printf("-> Enviando Betas...\n");
    carregar_beta(PATH_BETA);
    printf("-> Enviando Bias...\n");
    carregar_bias(PATH_BIAS);
    
    printf("[+] Parametros base carregados com sucesso.\n");

    printf("\n==================================================\n");
    printf("   [TESTE 3] INFERENCIA RECURSIVA EM LOTE         \n");
    printf("==================================================\n");
    
    processar_pasta_recursiva(PASTA_IMAGE, &total_batch);
    
    printf("\n==================================================\n");
    printf("   FIM DO TESTBENCH: %d imagens inferidas na FPGA.\n", total_batch);
    printf("==================================================\n");

    return EXIT_SUCCESS;
}
