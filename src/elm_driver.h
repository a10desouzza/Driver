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

/**
 * @brief Mapeia os registradores do FPGA para o espaço de usuário,
 *        permitindo acesso direto à ponte LW via ponteiro de memória.
 * @return ELM_OK em caso de sucesso,
 *         ELM_ERR_DEVMEM em caso de erro ao abrir "/dev/mem",
 *         ELM_ERR_MMAP em caso de falha de mmap().
 */
int mapear_fpga(void);

/**
 * @brief Reinicia o FPGA enviando um pulso no registrador de controle (ELM_SIG_RESET).
 * @return ELM_OK em caso de sucesso,
 *         ELM_ERR_DEVMEM se mapear_fpga() não foi chamado antes.
 */
int reiniciar_fpga(void);

/**
 * @brief Carrega os pesos do modelo ELM a partir de um arquivo binário
 *        (100352 valores de 16 bits), enviando cada peso via 2 comandos ao FPGA.
 * @param caminho Caminho para o arquivo de pesos.
 * @return ELM_OK em caso de sucesso,
 *         ELM_ERR_FILE_W em caso de falha ao abrir o arquivo,
 *         ELM_ERR_FPGA em caso de falha no envio da instrução ao FPGA.
 */
int carregar_pesos(const char *caminho);

/**
 * @brief Carrega os coeficientes beta do modelo ELM a partir de um arquivo binário
 *        (1280 valores de 16 bits), enviando cada coeficiente via 1 comando ao FPGA.
 * @param caminho Caminho para o arquivo de coeficientes beta.
 * @return ELM_OK em caso de sucesso,
 *         ELM_ERR_FILE_BT em caso de falha ao abrir o arquivo,
 *         ELM_ERR_FPGA em caso de falha no envio da instrução ao FPGA.
 */
int carregar_beta(const char *caminho);

/**
 * @brief Carrega os valores de bias do modelo ELM a partir de um arquivo binário
 *        (128 valores de 16 bits), enviando cada valor via 1 comando ao FPGA.
 * @param caminho Caminho para o arquivo de bias.
 * @return ELM_OK em caso de sucesso,
 *         ELM_ERR_FILE_BS em caso de falha ao abrir o arquivo,
 *         ELM_ERR_FPGA em caso de falha no envio da instrução ao FPGA.
 */
int carregar_bias(const char *caminho);

/**
 * @brief Carrega os dados de entrada (imagem 28x28 = 784 bytes) para o FPGA
 *        a partir de um arquivo binário, enviando cada pixel via 1 comando ao FPGA.
 * @param caminho Caminho para o arquivo de imagem.
 * @return ELM_OK em caso de sucesso,
 *         ELM_ERR_FILE_IMG em caso de arquivo inválido,
 *         ELM_ERR_READ_IMG em caso de falha ao ler o arquivo,
 *         ELM_ERR_FPGA em caso de falha no envio da instrução ao FPGA.
 */
int carregar_imagem(const char *caminho);

/**
 * @brief Dispara o processo de inferência no FPGA via pulso no registrador de controle.
 * @note  Não verifica se mapear_fpga() foi chamado antes.
 *        Não bloqueante — use obter_resultado() para aguardar o resultado.
 * @return ELM_OK sempre.
 */
int iniciar_inferencia(void);

/**
 * @brief Aguarda a conclusão da inferência e retorna o dígito classificado.
 *        Bloqueia em loop até que o bit ELM_BIT_DONE seja setado pelo FPGA.
 * @note  Deve ser chamada após iniciar_inferencia().
 *        Se a FPGA travar, esta função nunca retorna.
 * @return Valor entre 0 e 9 representando o dígito classificado.
 *         Valores entre 10 e 15 indicam possível erro de inferência.
 */
int obter_resultado(void);

/**
 * @brief Lê o valor bruto do registrador de status do FPGA (ELM_REG_DATA_OUT),
 *        sem aplicar nenhuma máscara ou processamento.
 * @note  Não bloqueante. Útil para diagnóstico e polling manual.
 *        Bits relevantes: ELM_BIT_DONE, ELM_BIT_BUSY, ELM_BIT_ERROR, ELM_MASK_DIGIT.
 * @return Valor de 32 bits do registrador de status.
 */
int ler_status_fpga(void);

#endif /* ELM_DRIVER_H */