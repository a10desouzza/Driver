/*
 * elm_driver.h - Driver para controle do FPGA ELM
 *
 * Autores: Pedro Henrique, Lucas Vilas Boas Dourado, Arthur Souza
 * Data: 21 de maio de 2026
 * Versao: 1.0
 *
 * Descricao:
 * Este arquivo define as funcoes e constantes necessarias para interagir com o FPGA ELM,
 * incluindo mapeamento de memoria, controle de sinais e leitura/escrita de dados
 */

#ifndef ELM_DRIVER_H
#define ELM_DRIVER_H

#include <stdint.h>

/* --- Enderecos e Tamanhos de Memoria --- */
#define ELM_BRIDGE_PHYS      0xFF200000UL /* Endereco fisico base da ponte HPS-FPGA */
#define ELM_BRIDGE_PAGE_SIZE 4096         /* Tamanho da pagina para mapeamento (4KB) */

/* --- Offsets dos Registradores --- */
#define ELM_REG_DATA_OUT  0x00            /* Registrador para ler dados e status da FPGA */
#define ELM_REG_SIGNALS   0x10            /* Registrador para enviar sinais de controle */
#define ELM_REG_DATA_IN   0x20            /* Registrador para enviar instrucoes/dados */

/* --- Mascaras do Registrador de Saida (DATA_OUT) --- */
#define ELM_BIT_DONE   (1 << 4)           /* Flag indicando inferencia concluida */
#define ELM_BIT_BUSY   (1 << 5)           /* Flag indicando FPGA ocupada processando */
#define ELM_BIT_ERROR  (1 << 6)           /* Flag indicando erro de hardware/operacao */
#define ELM_MASK_DIGIT 0xF                /* Mascara (bits 0-3) para extrair o digito predito */

/* --- Sinais de Controle (SIGNALS) --- */
#define ELM_SIG_ENABLE  (1 << 0)          /* Pulso para validar dado no DATA_IN */
#define ELM_SIG_CLR_OP  (1 << 1)          /* Pulso para limpar erro de operacao (Clear) */
#define ELM_SIG_RESET   (1 << 2)          /* Pulso de Reset geral da maquina de estados */

/* --- Codigos de Retorno padronizados --- */
#define ELM_OK              0
#define ELM_ERR_DEVMEM     -1
#define ELM_ERR_MMAP       -2
#define ELM_ERR_FILE_W     -3
#define ELM_ERR_FILE_BT    -4
#define ELM_ERR_FILE_BS    -5
#define ELM_ERR_FILE_IMG   -6
#define ELM_ERR_READ_IMG   -7
#define ELM_ERR_FPGA       -99


/* --- Prototipos das Funcoes --- */

/* Mapeia a memoria fisica da FPGA para o Linux
 * Espera: Nada (void)
 * Retorna: 0 (Sucesso), -1 (Erro /dev/mem) ou -2 (Erro mmap)
 */
int mapear_fpga(void);

/* Aplica o pulso de reset na maquina de estados da FPGA
 * Espera: Nada (void)
 * Retorna: 0 (Sucesso) ou -1 (Erro se memoria nao estiver mapeada)
 */
int reiniciar_fpga(void);

/* Le o arquivo binario e envia os pesos para a memoria da FPGA
 * Espera: caminho (String com o local do arquivo)
 * Retorna: 0 (Sucesso), < 0 (Erro arquivo) ou -99 (Erro interno FPGA)
 */
int carregar_pesos(const char *caminho);

/* Le o arquivo binario e envia os coeficientes beta para a FPGA
 * Espera: caminho (String com o local do arquivo)
 * Retorna: 0 (Sucesso), < 0 (Erro arquivo) ou -99 (Erro interno FPGA)
 */
int carregar_beta(const char *caminho);

/* Le o arquivo binario e envia os valores de bias para a FPGA
 * Espera: caminho (String com o local do arquivo)
 * Retorna: 0 (Sucesso), < 0 (Erro arquivo) ou -99 (Erro interno FPGA)
 */
int carregar_bias(const char *caminho);

/* Le o arquivo binario e envia os pixels da imagem para a FPGA
 * Espera: caminho (String com o local do arquivo)
 * Retorna: 0 (Sucesso), < 0 (Erro arquivo) ou -99 (Erro interno FPGA)
 */
int carregar_imagem(const char *caminho);

/* Envia o comando START para a FPGA iniciar a classificacao
 * Espera: Nada (void)
 * Retorna: 0 (Sempre sucesso na emissao)
 */
int iniciar_inferencia(void);

/* Funcao bloqueante que aguarda a flag DONE e extrai o resultado
 * Espera: Nada (void)
 * Retorna: Digito classificado (0 a 9)
 */
int obter_resultado(void);

/* Funcao nao-bloqueante que le o estado atual do registrador de saida
 * Espera: Nada (void)
 * Retorna: Valor bruto contendo os bits de status e o digito
 */
int ler_status_fpga(void);

#endif
