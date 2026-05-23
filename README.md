# Especificações do Driver
Este é um Driver Kernel Linux, escrito em Assembly para ARMv8. O Programa integra, através da HPS da Altera, o processador da placa De1-SoC e o CoProcessador de Mike.  
	
O Driver foi feito para máquinas Linux, baseadas em ARMv8, sem contar a necessidade da placa equipada com o coprocessador em questão.

O CoProcessador tem a função definida de receber uma imagem de um número, dentre as separadas e retornar uma inferência de qual número está escrito nessa imagem. Ao mesmo tempo, o coprocessador também retorna o seu estado (Done, Busy ou Error)
	
O programa consiste em um arquivo .s (Código em Assembly), um arquivo .h (Arquivo de definição dos registradores) e um .c (Importação do programa e visualização das saídas).
# Fundamentação Teórica
1. MMIO
   - Para a construção do driver, foram utilizados registradores acessados através da leitura da memória do coprocessador, o que configura o uso do MMIO.
2. HPS Altera
   - Na Placa De1-SoC é possivel utilizar-se da HPS Altera, uma ponte de conexão entre o ARMv7 e os circuitos funcionando na FPGA.
   - Seu uso se dá através de canais de memória, com endereços definidos, que podem ser acessados para leitura ou escrita de dados, por ambos os lados integrados, mesmo que nenhum tenha acesso definitivo ao outro. O Driver escreve em certos endereços, as entradas do coprocessador, e em outros, realiza a leitura de sua saída.
# Metodologia
Entradas: O Driver tem a função de direcionar as entradas do programa ao coprocessador, sendo esses a imagem, os arquivos de pesos e de viéses. Além disso, o Driver envia instruções de 32 Bits para o coprocessador, cada uma dessas, com sua função e resposta do coprocessador.
Saídas: O CoProcessador retorna, além de seu estado, o resultado da predição, encontrado após a inferência.
# Testes e Resultados
1. *Como executar*
```bash
sudo su
gcc main.c driver.s -o elm -I. -no-pie
./elm
```
O `sudo su` é necessário porque o driver precisa de acesso root para abrir `/dev/mem` — sem isso ele falha logo na primeira chamada. O `-no-pie` desativa o PIE (Position Independent Executable) para que o linker aceite misturar o `main.c` com o `driver.s` sem conflito de relocações.

2. *Testbench*
Antes de carregar qualquer parâmetro, o `main` chama `iniciar_inferencia()` de propósito, para ver se a FPGA bloqueia o comando. Depois lê o status com `imprimir_status()`, que decodifica o registrador de hardware e imprime flags legíveis como `[BUSY]` ou `[ERRO]`
exemplo de como e mostrado no terminal: 
```bash
==================================================
   TESTBENCH AUTOMATIZADO - ELM CO-PROCESSOR      
==================================================

[TESTE 1] Protecao contra comando precoce e operacao invalida
[STATUS HW] [BUSY] | DATA_OUT: 0x20
    [+] >> DIGITO PREDITO PELA FPGA (Lixo): 0 <<
[+] Sucesso: O hardware bloqueou a escrita e o driver reportou o erro (-99).
[STATUS HW] [ERRO] | DATA_OUT: 0x40

[TESTE 2] Carga de parametros da Rede Neural
-> Enviando Pesos...
-> Enviando Betas...
-> Enviando Bias...
[+] Parametros base carregados com sucesso.

==================================================
   [TESTE 3] INFERENCIA RECURSIVA EM LOTE         
==================================================

[*] Processando: ../data/image/img_0_digit_5.bin
    [+] >> DIGITO PREDITO PELA FPGA: 5 <<

[*] Processando: ../data/image/img_1_digit_3.bin
    [+] >> DIGITO PREDITO PELA FPGA: 3 <<

[*] Processando: ../data/image/pasta_extra/img_2_digit_9.bin
    [+] >> DIGITO PREDITO PELA FPGA: 9 <<

==================================================
   FIM DO TESTBENCH: 3 imagens inferidas na FPGA.
==================================================
```

3. *Acurácia — bias correto vs bias zerado*

Rodamos o teste 3 duas vezes: uma com os parâmetros reais e outra substituindo o `bias_q.bin` por um arquivo de zeros do mesmo tamanho.

| Configuração | Acertos | Acurácia |
|---|---|---|
| Bias correto | 83/100 | 83% |
| Bias zerado | 82/100 | 82% |

A diferença foi de apenas 1 imagem. Os erros que mudaram entre os dois testes foram opostos — um erro que virou acerto e um acerto que virou erro — o que mostra que o bias zerado não prejudicou nenhuma classe específica. A maior parte do trabalho da rede está nos pesos e no beta.

O dígito `5` foi o mais difícil nos dois casos (6/10), sendo confundido com `6`, `9` e `1`. Isso independe do bias.

4. *Erros encontrados no desenvolvimento*

O primeiro problema foi na função `mapear_fpga`: estávamos passando um endereço físico errado para o `mmap2`. O resultado foi que todas as escritas nos registradores da FPGA iam para uma região inválida do barramento e simplesmente não chegavam ao hardware.

O segundo problema foi das Endianness trocada os arquivos `.bin` dos parâmetros foram gerados em big-endian, mas o ARM opera em little-endian. Cada valor de 16 bits chegava com os bytes trocados — um bias `FBEB` virava `EBFB`, as imagens nao tiveram esse problema, pois cada pixel da imagem corespondia a 8 bits.

A correção foi adicionar `rev16` em todos os loops de leitura de parâmetros:

```asm
ldrh  r0, [r3, r0]   @ lê 2 bytes do buffer
rev16 r0, r0         @ corrige a ordem dos bytes
```
