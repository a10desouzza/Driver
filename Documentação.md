# 📖 Documentação da API: Driver Kernel/User-Space ELM (ARMv8 Assembly)

Esta documentação descreve a API de software em C para o driver desenvolvido em **ARMv8 Assembly**. O driver realiza o mapeamento de memória direta via **MMIO** utilizando a ponte Lightweight HPS-to-FPGA da placa Altera Cyclone V (De1-SoC), permitindo o carregamento de dados e a execução de inferências em lote no CoProcessador de Redes Neurais (ELM).

---

## 🛠️ Visão Geral dos Registradores de Hardware (Mapeamento MMIO)

A tabela abaixo descreve os registradores mapeados fisicamente a partir do endereço base da ponte `0xFF200000`.

| Offset | Registrador | Acesso | Bits | Descrição Técnica e Uso no Driver |
|---|---|---|---|---|
| 0x00 | ELM_REG_DATA_OUT | R | [3:0] | **Classe/Dígito:** Retorna o número predito (0-9) se DONE = 1. |
| 0x00 | ELM_REG_DATA_OUT | R | [4] | **DONE:** Flag de conclusão da inferência da IA. |
| 0x00 | ELM_REG_DATA_OUT | R | [5] | **BUSY:** Flag de hardware ocupado processando/gravando. |
| 0x00 | ELM_REG_DATA_OUT | R | [6] | **ERROR:** Erro de estouro de limite físico de memória. |
| 0x10 | ELM_REG_SIGNALS | W | [0] | **ENABLE/CLOCK:** Pulso (1 -> 0) para validar comandos no DATA_IN. |
| 0x10 | ELM_REG_SIGNALS | W | [1] | **CLEAR:** Pulso (1 -> 0) para limpar a flag de erro de estouro. |
| 0x10 | ELM_REG_SIGNALS | W | [2] | **RESET:** Pulso (1 -> 0) para zerar a máquina de estados (FSM). |
| 0x20 | ELM_REG_DATA_IN | W | [31:0] | **Barramento de Instrução:** Recebe o pacote empacotado (Opcode + Dados). |

---

## 💻 Definições de Constantes e Códigos de Erro (`elm_driver.h`)

```c
#define ELM_OK               0
#define ELM_ERR_DEVMEM      -1   // Falha ao abrir /dev/mem (Requer root/sudo)
#define ELM_ERR_MMAP        -2   // Falha ao mapear página de memória virtual
#define ELM_ERR_FILE_W      -3   // Falha ao abrir/ler arquivo de Pesos
#define ELM_ERR_FILE_BT     -4   // Falha ao abrir/ler arquivo de Betas
#define ELM_ERR_FILE_BS     -5   // Falha ao abrir/ler arquivo de Biases
#define ELM_ERR_FILE_IMG    -6   // Falha ao abrir/ler arquivo de Imagem
#define ELM_ERR_READ_IMG    -7   // Arquivo de imagem vazio ou corrompido
#define ELM_ERR_FPGA        -99  // Hardware reportou estouro de memória (Bit 6)
