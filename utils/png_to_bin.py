//necessario: pip install Pillow

from PIL import Image
import sys

def png_para_bin(arquivo_entrada, arquivo_saida):
    """
    Le a imagem PNG (arquivo_entrada), converte para tons de cinza,
    garante a dimensao 28x28 e salva como binario bruto (arquivo_saida).
    """
    try:
        # Abre a imagem e converte para tons de cinza (8 bits por pixel)
        img = Image.open(arquivo_entrada).convert('L')
        
        # Redimensiona para 28x28 caso a imagem original tenha outro tamanho
        if img.size != (28, 28):
            img = img.resize((28, 28))
            
        # Extrai os 784 pixels em uma lista plana
        pixels = list(img.getdata())
        
        # Grava os pixels puros no arquivo binario
        with open(arquivo_saida, 'wb') as f_out:
            f_out.write(bytearray(pixels))
            
        print(f"[+] Sucesso: '{arquivo_entrada}' convertido para '{arquivo_saida}' (784 bytes gravados).")
        
    except FileNotFoundError:
        print(f"[-] Erro: O arquivo '{arquivo_entrada}' nao foi encontrado na pasta atual.")
    except Exception as e:
        print(f"[-] Erro inesperado ao processar '{arquivo_entrada}': {e}")

# ==========================================================
# USO DO SCRIPT
# Modifique os nomes abaixo para os arquivos que voce tem
# na mesma pasta deste script.
# ==========================================================

if __name__ == "__main__":
    print("--- Conversor Simples PNG para BIN (28x28) ---")
    
    # Exemplo 1: Convertendo um arquivo especifico
    png_para_bin("digito_teste.png", "imagem_pronta.bin")
    
    # Exemplo 2: Convertendo outro arquivo (basta duplicar a linha)
    # png_para_bin("meu_digito.png", "saida_digito.bin")
    
    print("Processo finalizado.")