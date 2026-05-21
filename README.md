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
