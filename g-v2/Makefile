### Construção do arquivo de baixo para cima ###

# Nome do executável final
TARGET = g-v2

# regra padrão ao executar "make"
# isso dispara a compilação toda
all: $(TARGET)

# Compila todos os módulos em um único executável
# - lex.yy.c        → analisador léxico (Flex)
# - g-v2.tab.c      → analisador sintático (Bison)
# - ast.c           → estrutura da árvore sintática
# - symtab.c        → tabela de símbolos
# - semantico.c     → análise semântica
# - codigo.c        → geração de código
$(TARGET): lex.yy.c g-v2.tab.c ast.c symtab.c semantico.c codigo.c
	gcc -o $(TARGET) g-v2.tab.c lex.yy.c ast.c symtab.c semantico.c codigo.c

# Gera o analisador léxico usando Flex
# g-v2.l - regras léxicas
# g-v2.tab.h - tabela dos tokens do parser
lex.yy.c: g-v2.l g-v2.tab.h
	flex g-v2.l

# Gera o analisador sintático usando Bison
# Entrada com a gramática (g-v2.y) e a 
# Saída é o código do parser (g-v2.tab.c) e os seus tokens (g-v2.tab.h)
g-v2.tab.c g-v2.tab.h: g-v2.y
	bison -d -v g-v2.y

# Remove todos os arquivos gerados automaticamente
# Permite recompilar o projeto
clean:
	rm -f $(TARGET) lex.yy.c g-v2.tab.c g-v2.tab.h g-v2.output