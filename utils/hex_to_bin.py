import struct
import os

def converter_hex_para_bin(arquivo_hex, arquivo_bin):
    """
    Le um arquivo .hex (texto) e converte para .bin bruto de 16 bits.
    """
    if not os.path.exists(arquivo_hex):
        print(f"[-] Arquivo ignorado (nao encontrado): {arquivo_hex}")
        return

    with open(arquivo_hex, 'r') as f_in, open(arquivo_bin, 'wb') as f_out:
        contador = 0
        
        for linha in f_in:
            # Remove quebras de linha e divide caso haja varios valores separados por espaco
            valores = linha.strip().split()
            
            for val_hex in valores:
                # Remove o prefixo '0x' caso o seu arquivo .hex tenha sido gerado com ele
                val_hex = val_hex.replace('0x', '')
                
                if not val_hex:
                    continue
                    
                # Converte a string hexadecimal para um numero inteiro
                valor_int = int(val_hex, 16)
                
                # Empacota o numero em 16 bits (Halfword / 2 bytes)
                # O '>H' forca o padrao Big-Endian (Unsigned Short).
                # Isso casa perfeitamente com a instrucao 'rev16' do seu driver ARM.
                dado_binario = struct.pack('>H', valor_int)
                
                # Grava os 2 bytes no arquivo final
                f_out.write(dado_binario)
                contador += 1
                
    print(f"[+] Sucesso: {arquivo_hex} convertido para {arquivo_bin} ({contador} dados gravados)")

# Lista com os pares de arquivos (Origem HEX -> Destino BIN)
arquivos_parametros = [
    ("w_in_q.hex", "w_in_q.bin"),
    ("beta_q.hex", "beta_q.bin"),
    ("b_q.hex", "bias_q.bin")
]

print("==================================================")
print("   CONVERSOR DE PARAMETROS ELM (HEX -> BIN)       ")
print("==================================================")

for arq_hex, arq_bin in arquivos_parametros:
    converter_hex_para_bin(arq_hex, arq_bin)

print("Conversao finalizada. Arquivos prontos para o driver!")