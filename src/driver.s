@ ============================================================
@ Driver ELM — ARM Assembly (ARMv7, Linux userspace)
@ Placa Alvo: DE1-SoC (Cortex-A9)
@ Versão sem dsb, sem .ltorg e sem .syntax unified
@ ============================================================

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

@ ── Seção .data ─────────────────────────────────────────────
.section .data
dev_mem:  .asciz "/dev/mem"

@ ── Seção .bss (Buffers) ────────────────────────────────────
.section .bss
.balign 4
base:    .space 4
buf_w:   .space 200704
buf_bt:  .space 2560
buf_bs:  .space 256
buf_img: .space 784

@ ── Seção .text (Código) ────────────────────────────────────
.section .text

@ ============================================================
@ FUNÇÃO DE USO INTERNO: ler_arquivo
@ ============================================================
.type ler_arquivo, %function
ler_arquivo:
    push {r4-r7, lr}
    mov  r4, r1                       
    mov  r5, r2                       

    mov  r7, #5
    mov  r1, #0
    mov  r2, #0
    svc  0
    cmp  r0, #0
    blt  err_io

    mov  r6, r0                       

    mov  r7, #3
    mov  r1, r4
    mov  r2, r5
    svc  0

    mov  r7, #6
    mov  r0, r6
    svc  0

    mov  r0, #0
    pop  {r4-r7, pc}

err_io:
    mov  r0, #-1
    pop  {r4-r7, pc}

@ ============================================================
@ FUNÇÃO DE USO INTERNO: send_cmd
@ ============================================================
.type send_cmd, %function
send_cmd:
    str  r2, [r9, #0x20]              
    mov  r3, #1
    str  r3, [r9, #0x10]              
    mov  r3, #0
    str  r3, [r9, #0x10]              

1:  
    ldr  r3, [r9, #0x00]              
    tst  r3, #0x20                    
    bne  1b                           

    tst  r3, #0x40                    
    bne  2f                           

    mov  r0, #0                       
    bx   lr

2:  
    mov  r3, #2
    str  r3, [r9, #0x10]              
    mov  r3, #0
    str  r3, [r9, #0x10]              
    mov  r0, #-99                     
    bx   lr

@ ============================================================
@ FUNÇÕES EXPORTADAS (C API)
@ ============================================================

mapear_fpga:
    push {r4, r5, r7, lr}             
    mov  r7, #5                       
    ldr  r0, =dev_mem                 
    ldr  r1, =0x101002                
    mov  r2, #0                       
    svc  0                            
    cmp  r0, #0
    bge  mem_ok
    mov  r0, #-1                      
    pop  {r4, r5, r7, pc}             

mem_ok:
    mov  r7, #192                     
    mov  r4, r0                       
    mov  r0, #0                       
    mov  r1, #4096                    
    mov  r2, #3                       
    mov  r3, #1                       
    ldr  r5, =0xFF200000              
    lsr  r5, r5, #12                  
    svc  0                            
    cmn  r0, #4096                    
    bcc  mmap_ok
    mov  r0, #-2                      
    pop  {r4, r5, r7, pc}             

mmap_ok:
    ldr  r1, =base                    
    str  r0, [r1]                     
    mov  r0, #0                       
    pop  {r4, r5, r7, pc}             

reiniciar_fpga:
    ldr  r1, =base
    ldr  r1, [r1]                     
    cmp  r1, #0                       
    moveq r0, #-1
    bxeq lr
    mov  r3, #0x4                     
    str  r3, [r1, #0x10]              
    mov  r3, #0                       
    str  r3, [r1, #0x10]              
    mov  r0, #0                       
    bx   lr

carregar_pesos:
    push {r9-r11, lr}
    ldr  r1, =buf_w                   
    ldr  r2, =200704
    bl   ler_arquivo
    cmp  r0, #0
    blt  end_load_w

    ldr  r9, =base
    ldr  r9, [r9]
    mov  r10, #0
    ldr  r11, =100352

loop_w:
    cmp  r10, r11
    bhs  ok_load_w

    lsl  r2, r10, #3
    orr  r2, r2, #1
    bl   send_cmd
    cmp  r0, #0
    blt  end_load_w

    lsl  r0, r10, #1
    ldr  r3, =buf_w
    ldrh r0, [r3, r0]
    rev16 r0, r0
    lsl  r2, r0, #3
    orr  r2, r2, #2
    bl   send_cmd
    cmp  r0, #0
    blt  end_load_w

    add  r10, r10, #1
    b    loop_w

ok_load_w:
    mov  r0, #0
end_load_w:
    pop  {r9-r11, pc}

carregar_beta:
    push {r9-r11, lr}
    ldr  r1, =buf_bt
    ldr  r2, =2560
    bl   ler_arquivo
    cmp  r0, #0
    blt  end_load_bt

    ldr  r9, =base
    ldr  r9, [r9]
    mov  r10, #0
    ldr  r11, =1280

loop_bt:
    cmp  r10, r11
    bhs  ok_load_bt

    lsl  r0, r10, #1
    ldr  r3, =buf_bt
    ldrh r0, [r3, r0]
    rev16 r0, r0
    lsl  r2, r0, #14
    lsl  r3, r10, #3
    orr  r2, r2, r3
    orr  r2, r2, #4
    bl   send_cmd
    cmp  r0, #0
    blt  end_load_bt

    add  r10, r10, #1
    b    loop_bt

ok_load_bt:
    mov  r0, #0
end_load_bt:
    pop  {r9-r11, pc}

carregar_bias:
    push {r9-r11, lr}
    ldr  r1, =buf_bs
    ldr  r2, =256
    bl   ler_arquivo
    cmp  r0, #0
    blt  end_load_bs

    ldr  r9, =base
    ldr  r9, [r9]
    mov  r10, #0
    mov  r11, #128

loop_bs:
    cmp  r10, r11
    bhs  ok_load_bs

    lsl  r0, r10, #1
    ldr  r3, =buf_bs
    ldrh r0, [r3, r0]
    rev16 r0, r0
    lsl  r2, r0, #10
    lsl  r3, r10, #3
    orr  r2, r2, r3
    orr  r2, r2, #3
    bl   send_cmd
    cmp  r0, #0
    blt  end_load_bs

    add  r10, r10, #1
    b    loop_bs

ok_load_bs:
    mov  r0, #0
end_load_bs:
    pop  {r9-r11, pc}

carregar_imagem:
    push {r9-r11, lr}
    ldr  r1, =buf_img
    ldr  r2, =784
    bl   ler_arquivo
    cmp  r0, #0
    blt  end_load_img

    ldr  r9, =base
    ldr  r9, [r9]
    mov  r10, #0
    mov  r11, #784

loop_img:
    cmp  r10, r11
    bhs  ok_load_img

    ldr  r3, =buf_img
    ldrb r0, [r3, r10]
    lsl  r2, r0, #13
    lsl  r3, r10, #3
    orr  r2, r2, r3
    bl   send_cmd
    cmp  r0, #0
    blt  end_load_img

    add  r10, r10, #1
    b    loop_img

ok_load_img:
    mov  r0, #0
end_load_img:
    pop  {r9-r11, pc}

iniciar_inferencia:
    ldr  r1, =base
    ldr  r1, [r1]
    mov  r2, #5
    str  r2, [r1, #0x20]
    mov  r3, #1
    str  r3, [r1, #0x10]
    mov  r3, #0
    str  r3, [r1, #0x10]
    mov  r0, #0
    bx   lr

obter_resultado:
    ldr  r1, =base
    ldr  r1, [r1]
1:  
    ldr  r0, [r1, #0x00]              
    tst  r0, #0x10                    
    beq  1b                           
    and  r0, r0, #0xF                 
    bx   lr

ler_status_fpga:
    ldr  r1, =base
    ldr  r1, [r1]                     
    ldr  r0, [r1, #0x00]              
    bx   lr