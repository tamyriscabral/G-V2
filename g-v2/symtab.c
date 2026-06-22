#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"


// Função para iniciar a pilha de escopos
void iniciar_pilha(ScopeStack *pilha) {
    pilha->topo = NULL;
}

// Função para criar um novo escopo
void empilhar_escopo(ScopeStack *pilha) {
    Scope *novo = (Scope *) malloc(sizeof(Scope));
    if (!novo) {
        fprintf(stderr, "Erro de alocacao de memoria\n");
        exit(1);
    }
    
    // Inicializa a lista de símbolos vazia para o novo escopo
    novo->simbolos = NULL;  
    
    // Empilha o novo escopo no topo da pilha
    novo->prox = pilha->topo;
    pilha->topo = novo;
}

// Função que remove o escopo atual (ao sair de um bloco)
void desempilhar_escopo(ScopeStack *pilha) {
    if (!pilha->topo) return;

    Scope *escopo = pilha->topo;
    Symbol *s = escopo->simbolos;

    // Loop para liberar a memória dos símbolos do escopo
    while (s) {
        Symbol *temp = s;
        s = s->prox;
        free(temp->nome);
        free(temp);
    }

    // Remoção e liberação de memória
    pilha->topo = escopo->prox;
    free(escopo);
}

// Busca apenas no escopo atual
Symbol *buscar_no_escopo_atual(ScopeStack *pilha, const char *nome) {
    if (!pilha || !pilha->topo) return NULL;
    
    Symbol *s = pilha->topo->simbolos;
    while (s) {
        if (strcmp(s->nome, nome) == 0) {
            return s;
        }
        s = s->prox;
    }
    return NULL;
}

// Busca apenas no escopo global (escopo 0)
Symbol *buscar_global(ScopeStack *pilha, const char *nome) {
    if (!pilha || !pilha->topo) return NULL;
    
    Scope *escopo = pilha->topo;
    while (escopo->prox) {  // Navega até o primeiro
        escopo = escopo->prox;
    }
    
    Symbol *s = escopo->simbolos;
    while (s) {
        if (strcmp(s->nome, nome) == 0) {
            return s;
        }
        s = s->prox;
    }
    return NULL;
}

Symbol *buscar_simbolo(ScopeStack *pilha, const char *nome) {
    if (!pilha || !pilha->topo) return NULL;

    Scope *escopo = pilha->topo;
    while (escopo != NULL) {
        Symbol *s = escopo->simbolos;
        while (s) {
            if (strcmp(s->nome, nome) == 0)
                return s;  // retorna o primeiro encontrado (mais interno)
            s = s->prox;
        }
        escopo = escopo->prox;  // sobe para o escopo externo
    }
    return NULL;
}

// Função para inserir uma variável no escopo atual
int inserir_simbolo(ScopeStack *pilha, const char *nome, TipoSimbolo tipo, int linha, int escopo, int parametro){
    if (!pilha->topo) return 0; // Evita inserir sem escopo ativo

    // Evita redeclaração no mesmo escopo
    if (buscar_no_escopo_atual(pilha, nome) != NULL) {
        return 0;
    }

    Symbol *novo = (Symbol *) malloc(sizeof(Symbol));
    if (!novo) {
        fprintf(stderr, "Erro de alocacao de memoria\n");
        exit(1);
    }

    // Copia no nome da variável para 
    //evitar problemas com memória temporária
    novo->nome = strdup(nome);
    novo->tipo = tipo;
    novo->linha = linha;
    novo->eh_vetor = 0;
    novo->tamanho = 0;
    novo->escopo = escopo;
    novo->parametro = parametro;
    // Insere a variável no início da lista encadadeada
    novo->prox = pilha->topo->simbolos;
    pilha->topo->simbolos = novo;

    printf("Inserido símbolo: %s (tipo: %d, escopo: %d, parâm: %d)\n", nome, tipo, escopo, parametro); 
    return 1;
}

int inserir_simbolo_vetor(ScopeStack *pilha, const char *nome,
                            TipoSimbolo tipo, int tamanho, int linha, int escopo, int parametro) {
    Symbol *s = malloc(sizeof(Symbol));
    s->nome     = strdup(nome);
    s->tipo     = tipo;
    s->eh_vetor = 1;
    s->tamanho  = tamanho;
    s->linha    = linha;
    s->escopo = 0;
    s->parametro = parametro;
    s->prox     = pilha->topo->simbolos;  
    pilha->topo->simbolos = s;

    printf("Inserido símbolo: %s (tipo: %d, escopo: %d, parâm: %d)\n", nome, tipo, escopo, parametro);

    return 1;
}

// Função para liberar a memória de todos os escopos
void liberar_pilha(ScopeStack *pilha) {
    while (pilha->topo) {
        desempilhar_escopo(pilha);
    }
}