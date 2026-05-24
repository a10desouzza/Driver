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
     Já que já tinhamos os arquivos, e os bits de cada instrução já estavam definidos, nós precisávamos definir como o Driver iria ler esses dados. O caminho que escolhemos, foi usar o comando svc (systemcall), para direcionar o driver ao diretório onde os arquivos estavam, para que esse pudesse interpretar os valores.
     Para testar, enviamos os arquivos e pedimos uma saída que possuia relação com eles, por exemplo, os 16 primeiros bits do arquivo.
     
    - Envio dos dados
      Como foi dito, não existe um meio direto do Driver falar com o CoProcessador, já que um não tem acesso ao outro, então foi necessário o uso dos endereços da HPS, que ambos possuem acesso, para que essa comunicação ocorra. O Driver lê os arquivos em seus diretórios e escreve os dados em um buffer, de tamanho definido para cada arquivo. Esse buffer já é uma parte (endereço) da memória dessa HPS, então quando o driver escreve os dados nele, sua parte já foi feita.
      
    - Como o CoProcessador lê
      A HPS redireciona os dados desse endereço, definido por nós como saída do Driver e entrada do CoProcessador, para a porta LW do mesmo, onde os registradores vão direcionar os dados para cada módulo, dependendo do valor dos 32 bits da instrução, para que cada um faça seu papel na resolução. A Porta LW é feita para receber esses dados, então apartir desse ponto arquitetura do CoProcessador começa a trabalhar, devolvendo a predição da imagem (saída), após receber todas as instruções e dados.
      
    - Leitura do Resultado
      Os Registradores do CoProcessador direcionam o resultado da inferência (saída do sistema), para outro endereço da HPS, esse que foi definido para essa função. Criamos a função de leitura da predição, onde os registradores vão que vai ler aquilo que for escrito nesse endereço, e retornar esse valor para o usuário no terminal.

2. Como executar
```bash
sudo su
gcc main.c driver.s -o elm -I. -no-pie
./elm
```
O sudo su é necessário porque o driver precisa de acesso root para abrir /dev/mem — sem isso ele falha logo na primeira chamada. O -no-pie desativa o PIE (Position Independent Executable) para que o linker aceite misturar o main.c com o driver.s sem conflito de relocações.

3. Erros encontrados no desenvolvimento

O primeiro problema foi na função mapear_fpga: estávamos passando um endereço físico errado para o mmap2. O resultado foi que todas as escritas nos registradores da FPGA iam para uma região inválida do barramento e simplesmente não chegavam ao hardware.

O segundo problema foi das Endianness trocada os arquivos .bin dos parâmetros foram gerados em big-endian, mas o ARM opera em little-endian. Cada valor de 16 bits chegava com os bytes trocados — um bias FBEB virava EBFB, as imagens nao tiveram esse problema, pois cada pixel da imagem corespondia a 8 bits.

A correção foi adicionar rev16 em todos os loops de leitura de parâmetros:

```asm
ldrh  r0, [r3, r0]   @ lê 2 bytes do buffer
rev16 r0, r0         @ corrige a ordem dos bytes
```

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
