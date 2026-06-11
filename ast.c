#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"


// Função responsável por criar os nós da AST (Árvore Sintática Abstrata)
// A AST abstrai a árvore de derivação, mantendo apenas a estrutura semântica 
// relevante para fases posteriores.

AST *criar_no(AstKind tipo, const char *lexema, int linha,
              AST *filho1, AST *filho2, AST *filho3) {
    
    // Alocação de memória dinâmica
    AST *no = (AST *) malloc(sizeof(AST));
    
    if (!no) {
        fprintf(stderr, "Erro de alocacao de memoria\n");
        exit(1);
    }

    no->tipo = tipo; // símbolos não-terminais (e.g. AST_SE, AST_ENQUANTO)
    no->linha = linha; // guarda a linha onde ocorreu erro semântico
    
    // Armazena o lexema associado ao nó (e.g. identificadores, constantes)
    no->lexema = lexema ? strdup(lexema) : NULL;

    // Lógica de "filhos" para mapear os componentes da gramática
    // e.g. AST_SE: filho1 = condição, filho2 = bloco "THEN", filho3 = bloco "ELSE"
    no->filho1 = filho1;
    no->filho2 = filho2;
    no->filho3 = filho3;

    // nó irmão nulo que garante o término da lista sem puxar lixo da memória
    no->irmao = NULL;

    return no;
}

// Encadeia nós como irmãos, representando listas na AST
AST *adicionar_irmao(AST *no, AST *irmao) {
    if (!no) return irmao; // lista vazia

    // Percorre até o último irmão
    AST *p = no;
    while (p->irmao) {
        p = p->irmao;
    }

    // Conecta o novo nó ao final da lista
    p->irmao = irmao;

    return no;
}

// Funções para imprimir a AST (depuração)
static const char *nome_tipo_ast(AstKind tipo) {
    switch (tipo) {
        case AST_PROGRAMA: return "AST_PROGRAMA";
        case AST_BLOCO: return "AST_BLOCO";
        case AST_VARSECTION: return "AST_VARSECTION";
        case AST_DECLVAR: return "AST_DECLVAR";
        case AST_TIPO: return "AST_TIPO";
        case AST_LISTACOMANDO: return "AST_LISTACOMANDO";
        case AST_COMANDO_VAZIO: return "AST_COMANDO_VAZIO";
        case AST_LEIA: return "AST_LEIA";
        case AST_ESCREVA: return "AST_ESCREVA";
        case AST_NOVALINHA: return "AST_NOVALINHA";
        case AST_SE: return "AST_SE";
        case AST_ENQUANTO: return "AST_ENQUANTO";
        case AST_ATRIB: return "AST_ATRIB";
        case AST_OP: return "AST_OP";
        case AST_IDENT: return "AST_IDENT";
        case AST_INTCONST: return "AST_INTCONST";
        case AST_CARCONST: return "AST_CARCONST";
        case AST_STRING: return "AST_STRING";
        default: return "AST_DESCONHECIDO";
    }
}

// Busca em Profundidade (DFS) para imprimir a AST
void imprimir_ast(AST *no, int nivel) {
    if (!no) return;

    for (int i = 0; i < nivel; i++) {
        printf("  ");
    }

    // Mostra o tipo do nó e seu valor associado (se houver)
    printf("%s", nome_tipo_ast(no->tipo));
    if (no->lexema) {
        printf(" (%s)", no->lexema);
    }
    printf(" [linha %d]\n", no->linha);

    // Visita os filhos
    imprimir_ast(no->filho1, nivel + 1);
    imprimir_ast(no->filho2, nivel + 1);
    imprimir_ast(no->filho3, nivel + 1);

    // Visita os irmãos
    imprimir_ast(no->irmao, nivel);
}

// Libera toda a memória da AST,
// evitando vazamentos e uso inválido de memória.
void liberar_ast(AST *no) {
    if (!no) return;

    liberar_ast(no->filho1);
    liberar_ast(no->filho2);
    liberar_ast(no->filho3);
    liberar_ast(no->irmao);

    if (no->lexema) {
        free(no->lexema); // libera a string duplicada do lexema
    }

    free(no);
}