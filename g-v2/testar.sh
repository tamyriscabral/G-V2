#!/bin/bash

# Arquivo de saída
SAIDA="resultado_testes.txt"

# Limpa o arquivo anterior
> "$SAIDA"

# Lista de testes
TESTES=(
    testeacessovetor.g
    testecompleto.g
    testeescritavetor.g
    testefuncao+param.g
    testefuncao1param.g
    testefuncaocomarg.g
    testefuncaosemarg.g
    testefuncaosemparam.g
    testefuncaovetor.g
    testeindice.g
    testeleituravetor.g
    testevetor.g
)

for teste in "${TESTES[@]}"
do
    echo "========================================" >> "$SAIDA"
    echo "TESTE: $teste" >> "$SAIDA"
    echo "========================================" >> "$SAIDA"

    if [ -f "$teste" ]; then
        ./g-v2 "$teste" >> "$SAIDA" 2>&1
    else
        echo "Arquivo não encontrado." >> "$SAIDA"
    fi

    echo -e "\n" >> "$SAIDA"
done

echo "Resultados gravados em $SAIDA"