/*
 * elm_driver.h - Driver para controle do FPGA ELM
 *
 * Autores: Pedro Henrique, Lucas Vilas Boas Dourado, Arthur Souza
 * Data: 21 de maio de 2026
 * Versão: 1.0
 *
 * Descrição:
 *   Este arquivo define as funções e constantes necessárias para interagir com o FPGA ELM,
 *   incluindo mapeamento de memória, controle de sinais e leitura/escrita de dados
 */

#ifndef ELM_DRIVER_H
#define ELM_DRIVER_H

#include <stdint.h>

#define ELM_BRIDGE_PHYS      0xFF200000UL
#define ELM_BRIDGE_PAGE_SIZE 4096

#define ELM_REG_DATA_OUT  0x00
#define ELM_REG_SIGNALS   0x10
#define ELM_REG_DATA_IN   0x20

#define ELM_BIT_DONE   (1 << 4)
#define ELM_BIT_BUSY   (1 << 5)
#define ELM_BIT_ERROR  (1 << 6)
#define ELM_MASK_DIGIT 0xF

#define ELM_SIG_ENABLE  (1 << 0)
#define ELM_SIG_CLR_OP  (1 << 1)
#define ELM_SIG_RESET   (1 << 2)

#define ELM_OK              0
#define ELM_ERR_DEVMEM     -1
#define ELM_ERR_MMAP       -2
#define ELM_ERR_FILE_W     -3
#define ELM_ERR_FILE_BT    -4
#define ELM_ERR_FILE_BS    -5
#define ELM_ERR_FILE_IMG   -6
#define ELM_ERR_READ_IMG   -7
#define ELM_ERR_FPGA       -99

int mapear_fpga(void);
/**
 * @brief Mapeia os registradores do FPGA para o espaço de usuário, permitindo acesso direto à ponte LW via ponteiro de memória.
 * @return 0 em caso de sucesso, -1 em caso de erro na leitura de "/dev/mem" e -2 em caso de falha de mmap()
 */
int reiniciar_fpga(void);
/**
 * @brief Escreve um valor de 32 bits no registrador de controle do FPGA, envia cada peso via send_cmd em 2 comandos por peso.
 * @param valor O valor a ser escrito.
 * @return 0 em caso de sucesso, -1 em caso de mapeamento não realizado.
 */
int carregar_pesos(const char *caminho);
/** 
 * @brief Carrega os pesos do modelo ELM a partir de um arquivo binário.
 * @param caminho O caminho para o arquivo de pesos.
 * @return 0 em caso de sucesso, -1 em caso de falha ao abrir o arquivo ou -99 em falha de envio da instrução para o FPGA.
*/
int carregar_beta(const char *caminho);
/**
 * @brief Carrega os coeficientes beta do modelo ELM a partir de um arquivo binário.
 * @param caminho O caminho para o arquivo de coeficientes beta.    
 * @return 0 em caso de sucesso, -1 em caso de falha ao abrir o arquivo ou -99 em falha de envio da instrução para o FPGA.
 */
int carregar_bias(const char *caminho);
/**
 * @brief Carrega os valores de bias do modelo ELM a partir de um arquivo binário.
 * @param caminho O caminho para o arquivo de bias.
 * @return 0 em caso de sucesso, -1 em caso de falha ao abrir o arquivo ou -99 em falha de envio da instrução para o FPGA.
 */
int carregar_imagem(const char *caminho);
/**
 * @brief Carrega os dados de entrada (imagem) para o FPGA a partir de um arquivo binário.
 * @param caminho O caminho para o arquivo de imagem.
 * @return 0 em caso de sucesso, -1 em caso de falha ao abrir o arquivo ou -99 em falha de envio da instrução para o FPGA.
 */
int iniciar_inferencia(void);
/**
 * @brief Inicia o processo de inferência no FPGA, configurando os sinais de controle adequados.
 * @return 0 em todos os casos.
 */
int obter_resultado(void);
/**
 * @brief Lê o resultado da inferência do FPGA, verificando os sinais de status para garantir que a operação foi concluída com sucesso.
 * @return O resultado da inferência em caso de sucesso, 0 a 9 e quaisquer outros valores são possíveis erros de inferência.
 */
int ler_status_fpga(void);
/**
 * @brief Lê o status atual do FPGA, verificando os sinais de controle para determinar se a operação foi concluída, se o FPGA está ocupado ou se ocorreu um erro.
 * @return O valor do registrador de sinais do FPGA, onde os bits indicam o status da operação.
 */

#endif