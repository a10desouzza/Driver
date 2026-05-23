files = {
    "w_q_In.bin": 200704,  # 100352 pesos × 2 bytes
    "beta_q.bin": 2560,    # 1280 valores × 2 bytes
    "bias_q.bin": 256,     # 128 valores × 2 bytes
}

for nome, tamanho in files.items():
    with open(nome, "wb") as f:
        f.write(bytes(tamanho))
    print(f"{nome}: {tamanho} bytes gerados")