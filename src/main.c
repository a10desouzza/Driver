#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "elm_driver.h"

#define PASTA_DATA "../data"
#define PASTA_IMAGE "../data/image"

void limpar_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

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

int selecionar_arquivo(const char *tipo_parametro, const char *pasta_inicial, char *caminho_saida, size_t tam_max) {
    char pasta_atual[512];
    strncpy(pasta_atual, pasta_inicial, sizeof(pasta_atual));

    while (1) {
        DIR *dir = opendir(pasta_atual);
        if (!dir) {
            printf("[-] Erro: Nao foi possivel abrir a pasta %s\n", pasta_atual);
            return -1;
        }

        struct dirent *entrada;
        char itens[100][256];
        int is_dir[100];
        int contador = 0;

        printf("\n--- Navegando em: %s --- [%s] ---\n", pasta_atual, tipo_parametro);
        printf(" 0. [CANCELAR / SAIR DO MENU]\n");

        while ((entrada = readdir(dir)) != NULL) {
            if (entrada->d_name[0] == '.') continue;

            if (entrada->d_type == DT_DIR || entrada->d_type == DT_REG) {
                strncpy(itens[contador], entrada->d_name, 256);
                is_dir[contador] = (entrada->d_type == DT_DIR);
                
                if (is_dir[contador]) {
                    printf(" %d. [PASTA] %s/\n", contador + 1, itens[contador]);
                } else {
                    printf(" %d. %s\n", contador + 1, itens[contador]);
                }
                
                contador++;
                if (contador >= 100) break; 
            }
        }
        closedir(dir);

        if (contador == 0) {
            printf("    (Pasta vazia)\n");
        }

        int escolha = -1;
        while (escolha < 0 || escolha > contador) {
            printf("Escolha o numero (0 a %d): ", contador);
            if (scanf("%d", &escolha) != 1) {
                limpar_buffer();
                escolha = -1;
            }
        }
        limpar_buffer();

        if (escolha == 0) {
            return -1; 
        }

        int indice = escolha - 1;
        
        if (is_dir[indice]) {
            strncat(pasta_atual, "/", sizeof(pasta_atual) - strlen(pasta_atual) - 1);
            strncat(pasta_atual, itens[indice], sizeof(pasta_atual) - strlen(pasta_atual) - 1);
        } else {
            snprintf(caminho_saida, tam_max, "%s/%s", pasta_atual, itens[indice]);
            return 0; 
        }
    }
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
                printf("    [-] Erro ao carregar (Codigo: %d). Pulando...\n", ret);
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
    int opcao = -1;
    int ret;
    char caminho_escolhido[512];
    int total_batch = 0;

    printf("==================================================\n");
    printf("     PAINEL DE DEPURACAO — ELM CO-PROCESSOR       \n");
    printf("==================================================\n");

    printf("[*] Mapeando Lightweight Bridge (/dev/mem)...\n");
    if (mapear_fpga() != ELM_OK) {
        printf("[-] ERRO FATAL: Falha no mapeamento. Rode com sudo.\n");
        return EXIT_FAILURE;
    }
    printf("[+] FPGA mapeada com sucesso!\n");

    printf("[*] Aplicando RESET inicial obrigatorio...\n");
    reiniciar_fpga();
    imprimir_status();

    while (1) {
        printf("\n====================== MENU ======================\n");
        printf(" 1. Escolher e Carregar Pesos \n");
        printf(" 2. Escolher e Carregar Coeficientes \n");
        printf(" 3. Escolher e Carregar Bias\n");
        printf(" 4. Escolher e Carregar UMA Imagem \n");
        printf(" 5. Disparar Inferencia n");
        printf(" 6. Ler Resultado \n");
        printf(" 7. Rodar TODAS imagens das subpastas\n");
        printf(" 8. Verificar Status Atual dos Registradores\n");
        printf(" 0. Sair\n");
        printf("==================================================\n");
        printf("Escolha uma acao: ");

        if (scanf("%d", &opcao) != 1) {
            printf("\n[-] Entrada invalida.\n");
            limpar_buffer();
            continue;
        }
        limpar_buffer();

        switch (opcao) {
            case 1:
                if (selecionar_arquivo("PESOS W_IN", PASTA_DATA, caminho_escolhido, sizeof(caminho_escolhido)) == 0) {
                    printf("-> Enviando %s...\n", caminho_escolhido);
                    ret = carregar_pesos(caminho_escolhido);
                    if (ret == ELM_ERR_FPGA) printf("[-] ERRO HW: FPGA barrou a operacao.\n");
                    else if (ret < 0) printf("[-] ERRO IO: Falha de arquivo.\n");
                    else printf("[+] Pesos transferidos!\n");
                }
                break;

            case 2:
                if (selecionar_arquivo("BETA", PASTA_DATA, caminho_escolhido, sizeof(caminho_escolhido)) == 0) {
                    printf("-> Enviando %s...\n", caminho_escolhido);
                    ret = carregar_beta(caminho_escolhido);
                    if (ret == ELM_ERR_FPGA) printf("[-] ERRO HW: FPGA barrou a operacao.\n");
                    else if (ret < 0) printf("[-] ERRO IO: Falha de arquivo.\n");
                    else printf("[+] Betas transferidos!\n");
                }
                break;

            case 3:
                if (selecionar_arquivo("BIAS", PASTA_DATA, caminho_escolhido, sizeof(caminho_escolhido)) == 0) {
                    printf("-> Enviando %s...\n", caminho_escolhido);
                    ret = carregar_bias(caminho_escolhido);
                    if (ret == ELM_ERR_FPGA) printf("[-] ERRO HW: FPGA barrou a operacao.\n");
                    else if (ret < 0) printf("[-] ERRO IO: Falha de arquivo.\n");
                    else printf("[+] Bias transferidos!\n");
                }
                break;

            case 4:
                if (selecionar_arquivo("IMAGEM UNICA", PASTA_IMAGE, caminho_escolhido, sizeof(caminho_escolhido)) == 0) {
                    printf("-> Enviando %s...\n", caminho_escolhido);
                    ret = carregar_imagem(caminho_escolhido);
                    if (ret == ELM_ERR_FPGA) printf("[-] ERRO HW: FPGA barrou a operacao.\n");
                    else if (ret < 0) printf("[-] ERRO IO: Falha de arquivo.\n");
                    else printf("[+] Imagem transferida para a memoria do hardware!\n");
                }
                break;

            case 5:
                printf("\n-> Disparando pulso de START na FPGA...\n");
                iniciar_inferencia();
                imprimir_status(); 
                break;

            case 6:
                printf("\n-> Aguardando calculo (Bloqueante ate DONE=1)...\n");
                int res = obter_resultado();
                printf("\n========================================\n");
                printf("  RESULTADO OBTIDO: Digito %d\n", res);
                printf("========================================\n");
                printf("\n[*] Aplicando RESET automatico da FSM para liberar a proxima execucao...\n");
                reiniciar_fpga();
                imprimir_status();
                break;

            case 7:
                printf("\n==================================================\n");
                printf("   INFERENCIA RECURSIVA (VARRENDO PASTAS)         \n");
                printf("==================================================\n");
                total_batch = 0;
                processar_pasta_recursiva(PASTA_IMAGE, &total_batch);
                printf("\n==================================================\n");
                printf("   FIM DO LOTE: %d imagens inferidas na FPGA.\n", total_batch);
                printf("==================================================\n");
                break;

            case 8:
                imprimir_status();
                break;

            case 0:
                printf("\nEncerrando. Ate mais!\n");
                return EXIT_SUCCESS;

            default:
                printf("\n[-] Opcao invalida.\n");
                break;
        }
    }

    return EXIT_SUCCESS;
}