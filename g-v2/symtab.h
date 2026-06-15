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
} ScopeStack;

/* Pilha de escopos */
void iniciar_pilha(ScopeStack *pilha);     // vazia
void empilhar_escopo(ScopeStack *pilha);
void desempilhar_escopo(ScopeStack *pilha);

/* Símbolos */

// Busca variável **apenas** no escopo atual
Symbol *buscar_no_escopo_atual(ScopeStack *pilha, const char *nome);

// Busca variável em **todos** os escopos (do mais interno para o mais externo)
Symbol *buscar_na_pilha(ScopeStack *pilha, const char *nome);

// Insere variável no escopo atual
int inserir_simbolo(ScopeStack *pilha, const char *nome, TipoSimbolo tipo, int linha);

//Se for vetor
void inserir_simbolo_vetor(ScopeStack *pilha, const char *nome,TipoSimbolo tipo, int tamanho, int linha);

// Libera a memória de todos os escopos e símbolos
void liberar_pilha(ScopeStack *pilha);

#endif