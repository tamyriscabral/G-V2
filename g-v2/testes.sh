#!/bin/bash

SAIDA="todos_os_testes.txt"

> "$SAIDA"

for arquivo in *.g
do
    echo "========================================" >> "$SAIDA"
    echo "ARQUIVO: $arquivo" >> "$SAIDA"
    echo "========================================" >> "$SAIDA"

    cat "$arquivo" >> "$SAIDA"

    echo -e "\n\n" >> "$SAIDA"
done

echo "Arquivo gerado: $SAIDA"