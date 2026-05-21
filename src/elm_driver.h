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

#define ELM_SIG_ENABLE  (1 << 0)
#define ELM_SIG_CLR_OP  (1 << 1)
#define ELM_SIG_RESET   (1 << 2)


int mapear_fpga(void);
int reiniciar_fpga(void);
int carregar_pesos(const char *caminho);
int carregar_beta(const char *caminho);
int carregar_bias(const char *caminho);
int carregar_imagem(const char *caminho);
int iniciar_inferencia(void);
int obter_resultado(void);
int ler_status_fpga(void);

#endif