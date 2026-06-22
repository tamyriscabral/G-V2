#ifndef AST_H
#define AST_H

// Estrutura genérica para atributos semânticos
// (cada símbolo pode carregar atributos herdados)
typedef struct {
    char *str;
} Atributo;

// Estrutura para representar as informações da AST com contexto
// (carrega informações do token para a árvore)
typedef struct {
    char *lexema;
    int linha;
} TokenInfo;

// Estrutura para representar os *tipos* de nós da AST
// (aqui a estrutura da gramática é convertida em tipos de nós)
// Adicionado AST_RETORNE, AST_CHAMADA_FUNC, AST_FUNCAO, AST_PARAM
typedef enum {
    AST_PROGRAMA,
    AST_BLOCO,
    AST_VARSECTION,
    AST_DECLVAR,
    AST_TIPO,
    AST_COMANDO_VAZIO,
    AST_LEIA,
    AST_ESCREVA,
    AST_NOVALINHA,
    AST_SE,
    AST_ENQUANTO,
    AST_ATRIB,
    AST_OP,
    AST_IDENT,
    AST_INTCONST,
    AST_CARCONST,
    AST_STRING,
    AST_RETORNE,
    AST_CHAMADA_FUNC,
    AST_FUNCAO,
    AST_DECL_VETOR,
    AST_PARAM,
    AST_PARAM_VETOR,
    AST_ACESS_VETOR
} AstKind;

// Estrutura para representar um nó da AST
// (ignora detalhes sintáticos como parênteses e regras intermediárias)
typedef struct ast {
    AstKind tipo; // Define o papel sintático do nó
    char *lexema; // Armazena o valor textual associado ao nó
    int linha;    // Informação de localização no código fonte.
    int escopo; //Separa variáveis locais e globais
    
    // Representa os componentes da construção sitática
    struct ast *filho1; 
    struct ast *filho2;
    struct ast *filho3;

    // Ponteiro para o próximo nó da mesma lista (auxilia na criação de listas)
    struct ast *irmao;
} AST;

// Função principal que rege a criação de nós da AST
AST *criar_no(AstKind tipo, const char *lexema, int linha, int escopo, 
              AST *filho1, AST *filho2, AST *filho3);

// Função que encadeia nós como lista 
AST *adicionar_irmao(AST *no, AST *irmao);

// Função para imprimir a AST (depuração)
void imprimir_ast(AST *no, int nivel);

// Função para liberar memória alocada para a AST
void liberar_ast(AST *no);

#endif