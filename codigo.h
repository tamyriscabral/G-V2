#ifndef CODIGO_H
#define CODIGO_H

// Garante que a geração de código receba a AST como entrada
#include "ast.h"

// Acesso à tabela de símbolos
#include "symtab.h"

// Função de entrada da fase de geração de código em MIPS
void gerar_codigo(AST *raiz);

#endif