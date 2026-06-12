# G-V2

## Alterações 1

### g-v2.y 
  *   Declarações %type <ast>:
    DeclVetor
    DeclVarGlobais
    DeclFunc
    ListaParametros
    RestoListaParam
    ListaArgumentos

  *   Programa: Passa a reconhecer declaração de variáveis, funções e início do "main"

  *   DeclVarGlobais: reconhece "global \[ 'VarSection' \]"

  *   VarSection: passa a reconhecer variáveis entre colchetes e não chaves

  *   ListaDeclVar: passa a reconhecer também vetores

  *   DeclPrograma: muda a criação da AST e evita a criação de um PROGRAMA dentro de outro PROGRAMA

  *   **NOVO** DeclFunc: recebe as declarações de funções no formato "funcao tipo id(parametros) { }"

  *   **NOVO** ListaParametros: recebe a lista de pametros no formato "tipo id[]", ou "tipo id"

  *   **NOVO** RestoListaParam: o mesmo que ListaParametros, porém com uma vírgula antes (no caso de ter mais de um parâmetro"

  *   Comando: passa a receber também "retorne expr;" e "id\[expr\] = expr;"

  *   PrimExpr: passa a receber também "id\[expr\]" e "id(argumentos)"

  *   **NOVO** ListaArgumentos: recebe "Expr"

### g-v2.l
  *  Cria os tokens FUNCAO, GLOBAL, RETORNE e colchetes para reconhecimento de vetores

### ast.h
  *  Adiciona os rótulos para os nós da árvore: AST_RETORNE (nó de retorno de função), AST_CHAMADA_FUNC (nó de chamada de função), AST_FUNCAO (nó de declaração de função), AST_PARAM (nó de declaração de parâmetro), AST_DECL_VETOR (nó de declaração de vetor), AST_PARAM_VETOR (nó de declaração dos parâmetro no caso de serem vetores), AST_ATRIB_VETOR (nó para vetores)


### codigo.c
  *  No caso de AST_PROGRAMA, adiciona também gerar(no->filho2) e gerar(no->filho3)

## Makefile
  * Alterado somente os nomes dos arquivos para os novos nomes g-v2.
