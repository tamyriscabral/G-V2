#ifndef SYMTAB_H
#define SYMTAB_H

// Representa os tipos da linguagem G-V2
typedef enum {
    TIPO_INT,
    TIPO_CAR
} TipoSimbolo;

// Estrutura para representar uma variável na tabela de símbolos
typedef struct Symbol {
    char *nome;      //identificador
    TipoSimbolo tipo;//tipo
    int eh_vetor;   // 0 = escalar, 1 = vetor
    int tamanho;    // tamanho do vetor (0 se escalar)
    int linha;     // linha onde foi declarada
    int escopo;   // 0 = global, 1 = primeiro nível de função, 2 = bloco aninhado, etc.
    int pos;
    int parametro;
    struct Symbol *prox;   // ponteiro para o próximo símbolo
} Symbol;
// Estrutura para representar um escopo da linguagem
// (e.g. bloco { ... })
typedef struct Scope {
    Symbol *simbolos;        // Lista de variáveis declaradas nesse escopo
    struct Scope *prox;      // Ponteiro para o próximo escopo
} Scope;

// Estrutura para representar 
// a pilha de escopos ativos durante a análise
typedef struct {
    Scope *topo;
    Scope *base;
} ScopeStack;

/* Pilha de escopos */
void iniciar_pilha(ScopeStack *pilha);     // vazia
void empilhar_escopo(ScopeStack *pilha);
void desempilhar_escopo(ScopeStack *pilha);

/* Símbolos */

// Busca variável **apenas** no escopo atual
Symbol *buscar_no_escopo_atual(ScopeStack *pilha, const char *nome);

// Busca variável no escopo 0 (global)
Symbol *buscar_global(ScopeStack *pilha, const char *nome);

//Função auxiliar para buscas
Symbol *buscar_simbolo(ScopeStack *pilha, const char *nome);

Symbol *buscar_simbolo_parametros(ScopeStack *pilha, const char *nome);

// Insere variável no escopo atual
int inserir_simbolo(ScopeStack *pilha, const char *nome, TipoSimbolo tipo, int linha, int escopo, int parametro);

//Se for vetor
int inserir_simbolo_vetor(ScopeStack *pilha, const char *nome,TipoSimbolo tipo, int tamanho, int linha, int escopo, int parametro);

// Libera a memória de todos os escopos e símbolos
void liberar_pilha(ScopeStack *pilha);

#endif