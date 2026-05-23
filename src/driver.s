@================================================================================================
@ elm_driver.h - Driver para controle do FPGA ELM
@ Autores: Pedro Henrique, Lucas Vilas Boas Dourado, Arthur Souza
@ Data: 21 de maio de 2026
@ Versao: 1.0
@ Descricao:
@ Este arquivo assembly contem as funcoes e constantes necessarias para interagir com o FPGA ELM,
@ incluindo mapeamento de memoria, controle de sinais e leitura/escrita de dados                  
@ ================================================================================================
.global mapear_fpga
.type mapear_fpga, %function
.global reiniciar_fpga
.type reiniciar_fpga, %function
.global carregar_pesos
.type carregar_pesos, %function
.global carregar_beta
.type carregar_beta, %function
.global carregar_bias
.type carregar_bias, %function
.global carregar_imagem
.type carregar_imagem, %function
.global iniciar_inferencia
.type iniciar_inferencia, %function
.global obter_resultado
.type obter_resultado, %function
.global ler_status_fpga
.type ler_status_fpga, %function

.section .data
dev_mem:  .asciz "/dev/mem"          @ caminho do arquivo de memória física

.section .bss
.balign 4
base:    .space 4                    @ endereço virtual mapeado
buf_w:   .space 200704               @ buffer de pesos   (100352 x 2 bytes)
buf_bt:  .space 2560                 @ buffer de betas   (1280   x 2 bytes)
buf_bs:  .space 256                  @ buffer de biases  (128    x 2 bytes)
buf_img: .space 784                  @ buffer de imagem  (784    x 1 byte)

.section .text

@ Abre /dev/mem e mapeia a região de periféricos da FPGA via mmap2.
mapear_fpga:
    push {r4, r5, r7, lr}

    mov  r7, #5                      @ syscall open
    ldr  r0, =dev_mem                @ caminho "/dev/mem"
    ldr  r1, =0x101002               @ flags O_RDWR | O_SYNC
    mov  r2, #0
    svc  0
    cmp  r0, #0                      @ fd válido?
    bge  mem_ok
    mov  r0, #-1                     @ erro no open
    pop  {r4, r5, r7, pc}

mem_ok:
    mov  r4, r0                      @ salva fd

    mov  r7, #192                    @ syscall mmap2
    mov  r0, #0                      @ kernel escolhe o endereço
    mov  r1, #4096                   @ tamanho: 1 página
    mov  r2, #3                      @ PROT_READ | PROT_WRITE
    mov  r3, #1                      @ MAP_SHARED
    ldr  r5, =0xFF200000             @ endereço físico base da FPGA
    lsr  r5, r5, #12                 @ offset em páginas para mmap2
    svc  0

    cmn  r0, #4096                   @ MAP_FAILED?
    bcc  mmap_ok
    mov  r0, #-2                     @ erro no mmap
    pop  {r4, r5, r7, pc}

mmap_ok:
    ldr  r1, =base
    str  r0, [r1]                    @ salva endereço virtual mapeado

    mov  r0, #0                      @ sucesso
    pop  {r4, r5, r7, pc}

@ Envia pulso de reset ao coprocessador.
reiniciar_fpga:
    push {r9, lr}
    ldr  r9, =base
    ldr  r9, [r9]                    @ carrega endereço base

    cmp  r9, #0                      @ memória mapeada?
    beq  rst_err

    mov  r3, #0x4                    @ sobe reset
    str  r3, [r9, #0x10]
    mov  r3, #0                      @ desce reset
    str  r3, [r9, #0x10]

    mov  r0, #0                      @ sucesso
    pop  {r9, pc}

rst_err:
    mov  r0, #-1                     @ erro: base nula
    pop  {r9, pc}

@ Lê 100352 pesos (fp16) e os envia em dois comandos: endereço (op1) e valor (op2).
carregar_pesos:
    push {r4, r7, r9-r11, lr}
    ldr  r9, =base
    ldr  r9, [r9]

    mov  r7, #5                      @ syscall open
    mov  r1, #0                      @ O_RDONLY
    mov  r2, #0
    svc  0
    cmp  r0, #0
    bge  w_ok
    mov  r0, #-3                     @ erro no open
    pop  {r4, r7, r9-r11, pc}

w_ok:
    mov  r4, r0                      @ salva fd

    ldr  r1, =buf_w                  @ destino da leitura
    mov  r2, #200704                 @ 200704 bytes
    mov  r7, #3                      @ syscall read
    svc  0

    mov  r7, #6                      @ syscall close
    mov  r0, r4
    svc  0

    mov  r10, #0                     @ iterador
    ldr  r11, =100352                @ limite

loop_w:
    cmp  r10, r11
    bhs  w_done

    lsl  r0, r10, #1                 @ offset = índice * 2
    ldr  r3, =buf_w
    ldrh r0, [r3, r0]                @ lê 16 bits
    rev16 r0, r0                     @ corrige endianness

    mov  r1, r10
    bl   fmt_w_addr                  @ monta instrução de endereço
    bl   send_cmd                    @ envia endereço

    bl   fmt_w_val                   @ monta instrução de valor
    bl   send_cmd                    @ envia valor
    bl   chk_err
    cmp  r0, #0
    blt  end_w                       @ aborta se erro

    add  r10, r10, #1
    b    loop_w

w_done:
    mov  r0, #0
end_w:
    pop  {r4, r7, r9-r11, pc}

@ Lê 1280 betas (fp16) e os envia com opcode 4.
@ Estrutura idêntica a carregar_pesos; diferenças marcadas abaixo.
carregar_beta:
    push {r4, r7, r9-r11, lr}
    ldr  r9, =base
    ldr  r9, [r9]

    mov  r7, #5
    mov  r1, #0
    mov  r2, #0
    svc  0
    cmp  r0, #0
    bge  bt_ok
    mov  r0, #-4                     @ código de erro diferente
    pop  {r4, r7, r9-r11, pc}

bt_ok:
    mov  r4, r0

    ldr  r1, =buf_bt                 @ buffer de betas
    mov  r2, #2560                   @ 1280 x 2 bytes
    mov  r7, #3
    svc  0

    mov  r7, #6
    mov  r0, r4
    svc  0

    mov  r10, #0
    mov  r11, #1280                  @ limite diferente

loop_bt:
    cmp  r10, r11
    bhs  bt_done

    lsl  r0, r10, #1
    ldr  r3, =buf_bt
    ldrh r0, [r3, r0]
    rev16 r0, r0

    mov  r1, r10
    bl   fmt_bt                      @ formato de beta (opcode 4)
    bl   send_cmd
    bl   chk_err                     @ pesos usam dois send_cmd; beta usa apenas um
    cmp  r0, #0
    blt  end_bt

    add  r10, r10, #1
    b    loop_bt

bt_done:
    mov  r0, #0
end_bt:
    pop  {r4, r7, r9-r11, pc}

@ Lê 128 biases e os envia com opcode 3.
@ Estrutura idêntica a carregar_beta; diferenças marcadas abaixo.
carregar_bias:
    push {r4, r7, r9-r11, lr}
    ldr  r9, =base
    ldr  r9, [r9]

    mov  r7, #5
    mov  r1, #0
    mov  r2, #0
    svc  0
    cmp  r0, #0
    bge  bs_ok
    mov  r0, #-5                     @ código de erro diferente
    pop  {r4, r7, r9-r11, pc}

bs_ok:
    mov  r4, r0

    ldr  r1, =buf_bs                 @ buffer de biases
    mov  r2, #256                    @ 128 x 2 bytes
    mov  r7, #3
    svc  0

    mov  r7, #6
    mov  r0, r4
    svc  0

    mov  r10, #0
    mov  r11, #128                   @ limite diferente

loop_bs:
    cmp  r10, r11
    bhs  bs_done

    lsl  r0, r10, #1
    ldr  r3, =buf_bs
    ldrh r0, [r3, r0]
    rev16 r0, r0

    mov  r1, r10
    bl   fmt_bs                      @ formato de bias (opcode 3)
    bl   send_cmd
    bl   chk_err
    cmp  r0, #0
    blt  end_bs

    add  r10, r10, #1
    b    loop_bs

bs_done:
    mov  r0, #0
end_bs:
    pop  {r4, r7, r9-r11, pc}

@ Lê 784 pixels (8 bits) e os envia com opcode 0.
@ Diferenças em relação às funções anteriores marcadas abaixo.
carregar_imagem:
    push {r4, r7, r9-r11, lr}
    ldr  r9, =base
    ldr  r9, [r9]

    mov  r7, #5
    mov  r1, #0
    mov  r2, #0
    svc  0
    cmp  r0, #0
    bge  img_ok
    mov  r0, #-6                     @ código de erro diferente
    pop  {r4, r7, r9-r11, pc}
    
img_ok:
    mov  r4, r0

    ldr  r1, =buf_img                @ buffer de imagem
    mov  r2, #784                    @ 784 x 1 byte
    mov  r7, #3
    svc  0
    cmp  r0, #1                      @ verifica se leu ao menos 1 byte
    bge  img_lida

    mov  r7, #6                      @ fecha antes de retornar erro
    mov  r0, r4
    svc  0
    mov  r0, #-7                     @ erro: nenhum byte lido
    pop  {r4, r7, r9-r11, pc}

img_lida:
    mov  r7, #6
    mov  r0, r4
    svc  0

    mov  r10, #0
    mov  r11, #784                   @ limite diferente

loop_img:
    cmp  r10, r11
    bhs  img_done

    ldr  r3, =buf_img
    ldrb r0, [r3, r10]              @ lê 1 byte (pixel); sem rev16 pois não é fp16
    mov  r1, r10
    bl   fmt_img                     @ formato de imagem (opcode 0)
    bl   send_cmd
    bl   chk_err
    cmp  r0, #0
    blt  end_img

    add  r10, r10, #1
    b    loop_img

img_done:
    mov  r0, #0
end_img:
    pop  {r4, r7, r9-r11, pc}

@ Escreve opcode 5 e envia pulso de start para disparar a inferência.
iniciar_inferencia:
    push {r9, lr}
    ldr  r9, =base
    ldr  r9, [r9]

    mov  r2, #5                      @ opcode de inferência
    str  r2, [r9, #0x20]            @ escreve no registrador de dados
    mov  r3, #1                      @ sobe start
    str  r3, [r9, #0x10]
    mov  r3, #0                      @ desce start
    str  r3, [r9, #0x10]

    mov  r0, #0
    pop  {r9, pc}

@ Aguarda bit 4 do status (done) e retorna bits [3:0] com a classe inferida.
obter_resultado:
    push {r9, lr}
    ldr  r9, =base
    ldr  r9, [r9]

poll_done:
    ldr  r0, [r9, #0x00]            @ lê registrador de status
    tst  r0, #0x10                  @ testa bit 4 (done)
    beq  poll_done                  @ aguarda enquanto não concluído

    and  r0, r0, #0xF               @ isola classe nos bits [3:0]
    pop  {r9, pc}

@ Retorna o valor bruto do registrador de status.
ler_status_fpga:
    push {r3, lr}
    ldr  r3, =base
    ldr  r3, [r3]
    ldr  r0, [r3, #0x00]            @ lê registrador de status
    pop  {r3, pc}

@ Empacota pixel (8b) e índice em r2; opcode 0 implícito (bits [2:0] = 000).
fmt_img:
    lsl  r2, r0, #13                 @ pixel nos bits [20:13]
    lsl  r3, r1, #3                  @ índice nos bits [12:3]
    orr  r2, r2, r3
    bx   lr

@ Empacota índice do peso em r2 com opcode 1.
fmt_w_addr:
    lsl  r2, r1, #3                  @ índice nos bits superiores
    orr  r2, r2, #1                  @ opcode 1
    bx   lr

@ Empacota valor do peso em r2 com opcode 2.
fmt_w_val:
    lsl  r2, r0, #3                  @ valor nos bits superiores
    orr  r2, r2, #2                  @ opcode 2
    bx   lr

@ Empacota valor e índice do bias em r2 com opcode 3.
fmt_bs:
    lsl  r2, r0, #10                 @ valor nos bits [25:10]
    lsl  r3, r1, #3                  @ índice nos bits [9:3]
    orr  r2, r2, r3
    orr  r2, r2, #3                  @ opcode 3
    bx   lr

@ Empacota valor e índice do beta em r2 com opcode 4.
@ Igual a fmt_bs, mas o campo de valor começa no bit 14.
fmt_bt:
    lsl  r2, r0, #14                 @ valor nos bits [29:14]
    lsl  r3, r1, #3                  @ índice nos bits [13:3]
    orr  r2, r2, r3
    orr  r2, r2, #4                  @ opcode 4
    bx   lr

@ Escreve r2 no registrador de dados e envia pulso de clock ao coprocessador.
send_cmd:
    str  r2, [r9, #0x20]            @ escreve comando
    mov  r3, #1                      @ sobe clock
    str  r3, [r9, #0x10]
    mov  r3, #0                      @ desce clock
    str  r3, [r9, #0x10]
    ldr  r3, [r9, #0x00]            @ leitura 1 (pipeline drain)
    ldr  r3, [r9, #0x00]            @ leitura 2 (pipeline drain)
    bx   lr

@ Verifica bit 6 do status; retorna 0 se ok, -99 e envia clear se erro.
chk_err:
    ldr  r3, [r9, #0x00]            @ lê status
    tst  r3, #0x40                  @ testa bit 6 (erro)
    bne  chk_fail
    mov  r0, #0                      @ sem erro
    bx   lr

chk_fail:
    mov  r3, #2                      @ sobe clear de erro
    str  r3, [r9, #0x10]
    mov  r3, #0                      @ desce clear
    str  r3, [r9, #0x10]
    mov  r0, #-99                    @ retorna erro
    bx   lr
