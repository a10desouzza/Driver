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
Entradas: O CoProcessador deve receber as entradas do seu sistema, essas sendo o arquivo da imagem, o arquivo de pesos e o arquivo dos viéses, além dos bits da instrução, já que o CoProcessador só vai carregar, calcular ou enviar, após receber a instrução de 32bits relacionada.

Saídas: O CoProcessador retorna, além de seu estado, o resultado da predição, encontrado após a inferência.

Sabendo disso, nosso objetivo era criar um Driver que poderia receber os dados dos arquivos de imagens, pesos e viéses, e direcioná-los ao CoProcessador, mesmo sem ter acesso ao que acontece dentro do mesmo. Para isso, nosso primeiro problema foi entender como que o CoProcessador poderia receber esses dados, sendo que não tinhamos acesso ao seu conteúdo.

1. A Comunicação
   - Leitura dos arquivos
     Já que já tinhamos os arquivos, e os bits de cada instrução já estavam definidos, nós precisávamos definir como o Driver iria ler esses dados. O caminho que escolhemos, foi usar o comando svc (systemcall), para direcionar o driver ao diretório onde os arquivos estavam, para que os registradores que definimos pudessem receber os dados.
     Para testar, enviamos os arquivos e pedimos uma saída que possuia relação com eles, por exemplo, os 16 primeiros bits do arquivo.
     
    ```bash 
    mov  r4, r0         ← salva o fd aberto
	ldr  r1, =buf_w     ← endereço do buffer na RAM do HPS (destino)
	mov  r2, #200704    ← quantos bytes ler
	mov  r7, #3         ← syscall read()
	svc  0              ← kernel lê o arquivo e despeja direto no buffer
    ```
    
    - Envio dos dados
      Como foi dito, não existe um meio direto do Driver falar com o CoProcessador, já que um não tem acesso ao outro, então foi necessário o uso dos endereços da HPS, que ambos possuem acesso, para que essa comunicação ocorra. O Driver lê os arquivos em seus diretórios e os registradores escrevem os dados em um buffer, através de um loop, já que não era possível escrever todos os dados em somente um ciclo, ou em uma instrução. Esse buffer possui tamanho definido no código a partir do arquivo que ele vai receber, e já é uma parte (endereço) da memória dessa HPS, então quando os arquivos terminam de ser escritos, a parte do Driver já vai ter sido feita.
      
    ```bash
    lsl  r0, r10, #1       ← índice × 2 (cada valor ocupa 2 bytes)
	ldr  r3, =buf_w        ← endereço base do buffer
	ldrh r0, [r3, r0]      ← lê 2 bytes do buffer (um valor de 16 bits)
	rev16 r0, r0           ← corrige endianness antes de enviar
    ```
    
    - Como o CoProcessador lê
      A HPS redireciona os dados desse endereço, definido por nós como saída do Driver e entrada do CoProcessador, para a porta LW do mesmo, onde os registradores vão direcionar os dados para cada módulo, dependendo do valor dos 32 bits da instrução, para que cada um faça seu papel na resolução. A Porta LW é feita para receber esses dados, então apartir desse ponto arquitetura do CoProcessador começa a trabalhar, devolvendo a predição da imagem (saída), após receber todas as instruções e dados.
      
    - Leitura do Resultado
      Os Registradores do CoProcessador direcionam o resultado da inferência (saída do sistema), para outro endereço da HPS, esse que foi definido para essa função. Criamos a função de leitura da predição, onde os registradores vão que vai ler aquilo que for escrito nesse endereço, e retornar esse valor para o usuário no terminal.
      
   ```bash
   poll_done:
    ldr  r0, [r9, #0x00]   ← lê o registrador de status da FPGA
    tst  r0, #0x10         ← testa o bit 4 (flag "done")
    beq  poll_done         ← se não terminou, continua lendo
    and  r0, r0, #0xF      ← isola os 4 bits com o dígito predito (0–9)
	```
   
2. Como executar
```bash
sudo su
gcc main.c driver.s -o elm -I. -no-pie
./elm
```
O sudo su é necessário pois, como descrito anteriormente, o Driver tem acesso ao diretório do arquivo, então ele precisa de acesso root para abrir o /dev/mem — sem isso ele falha logo na primeira chamada. O -no-pie desativa o PIE (Position Independent Executable) para que o linker misture o main.c com o driver.s sem conflito.

3. Erros encontrados no desenvolvimento

O primeiro problema foi na função mapear_fpga: estávamos passando um endereço físico errado para o mmap2. O resultado foi que todas as escritas nos registradores da FPGA iam para uma região inválida do barramento e simplesmente não chegavam ao hardware.

O segundo problema foi das Endianness trocada os arquivos .bin dos parâmetros foram gerados em big-endian, mas o ARM opera em little-endian. Cada valor de 16 bits chegava com os bytes trocados — um bias FBEB virava EBFB, as imagens nao tiveram esse problema, pois cada pixel da imagem corespondia a 8 bits.

A correção foi adicionar rev16 em todos os loops de leitura de parâmetros:

```asm
ldrh  r0, [r3, r0]   @ lê 2 bytes do buffer
rev16 r0, r0         @ corrige a ordem dos bytes
```
4. Banco de Registradores
   Montamos um banco de registradores para todas as funções que o driver precisa:
   
   | Offset (Hex) | Nome do Registrador | Acesso | Bits | Descrição Técnica e Uso no Driver |
| :--- | :--- | :---: | :---: | :--- |
| **0x00** | `REG_INSTRUCTION` | Escrita (W) | `[2:0]` | Seletor da operação/memória de destino (Ex: 000 = IMG, 011 = BIAS). |
| **0x00** | `REG_INSTRUCTION` | Escrita (W) | `[31:3]` | Contém o Endereço e o Dado a ser gravado (formato varia conforme o OPCODE). |
| **0x04** | `REG_CONTROL` | Escrita (W) | `[0]` | Escreva 1 para disparar o cálculo (inferência) da Rede Neural. |
| **0x04** | `REG_CONTROL` | Escrita (W) | `[1]` | Escreva 1 para limpar a flag de erro de limite de memória. |
| **0x04** | `REG_CONTROL` | Escrita (W) | `[2]` | Escreva 1 para reiniciar/zerar a máquina de estados do coprocessador. |
| **0x04** | `REG_CONTROL` | Escrita (W) | `[31:3]` | Bits não utilizados pelo controle. Recomenda-se escrever 0. |
| **0x08** | `REG_STATUS` | Leitura (R) | `[3:0]` | Retorna o dígito (0 a 9) classificado pela IA. Válido apenas quando DONE = 1. |
| **0x08** | `REG_STATUS` | Leitura (R) | `[4]` | Flag de conclusão: 1 significa que a IA terminou de processar a imagem. |
| **0x08** | `REG_STATUS` | Leitura (R) | `[5]` | Flag de ocupado: 1 significa que o hardware está gravando dados. Não envie nova instrução. |
| **0x08** | `REG_STATUS` | Leitura (R) | `[6]` | Flag de erro: 1 significa tentativa de gravação fora do limite físico de memória. |
| **0x08** | `REG_STATUS` | Leitura (R) | `[31:7]` | Bits não utilizados. O hardware retornará 0. |
# Testes e Resultados
1. Testbench
Antes de carregar qualquer parâmetro, o main chama iniciar_inferencia() de propósito, para ver se a FPGA bloqueia o comando. Depois lê o status com imprimir_status(), que decodifica o registrador de hardware e imprime flags legíveis como [BUSY] ou [ERRO]
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

2. Taxa de acertos

Rodamos o teste duas vezes: uma com os parâmetros reais e outra substituindo o bias_q.bin por um arquivo de zeros do mesmo tamanho.

| Configuração | Acertos | Acurácia |
|---|---|---|
| Bias correto | 83/100 | 83% |
| Bias zerado | 82/100 | 82% |

A diferença foi de apenas 1 imagem. Os erros que mudaram entre os dois testes foram opostos — um erro que virou acerto e um acerto que virou erro — o que mostra que o bias zerado não prejudicou nenhuma classe específica. A maior parte do trabalho da rede está nos pesos e no beta.

O dígito 5 foi o que gerou mais erros (6/10), o coprocessador identificou ele como 6, 9 e até 1.

#Conclusão
O Driver possui uma ligação funcional e estável com o CoProcessador, fornecendo tudo que é necessário para que esse realize sua função. A Taxa de 83% de acerto e o jeito em que o Driver funciona são resultado de uma estrutura estável de projeto, porém existe um erro de planejamento, que gera a necessidade de uma atualização para uso futuro: Nosso driver tem acesso ao disco rígido do computador, através do svc (systemcall), algo que um driver não deveria fazer, embora tenha sido a maneira que encontramos de resolver o problema, entendemos hoje que isso deve ser mudado.

Entretanto, de forma geral, acreditamos que conseguimos alcançar o objetivo final, de maneira aceitável.
