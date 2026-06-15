#ifndef SEMANTICO_H
#define SEMANTICO_H

// Fornece a estrutura da AST
#include "ast.h"

// Fornece a tabela de símbolos e a pilha de escopos
#include "symtab.h"

// Recebe a raiz da AST e executa toda a análise semântica
void analisar_semantico(AST *raiz);

#endif