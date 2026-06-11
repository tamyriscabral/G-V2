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

// Função para buscar variável **apenas** no escopo atual
Symbol *buscar_no_escopo_atual(ScopeStack *pilha, const char *nome) {
    if (!pilha->topo) return NULL;

    Symbol *s = pilha->topo->simbolos;
    while (s) {
        // Compara o nome do símbolo com o nome buscado
        if (strcmp(s->nome, nome) == 0) {
            return s;
        }
        s = s->prox;
    }
    return NULL;
}

// Função para buscar variável em **todos** os escopos
Symbol *buscar_na_pilha(ScopeStack *pilha, const char *nome) {
    Scope *escopo = pilha->topo; // Começa no escopo mais interno

    // Busca em loop
    while (escopo) {
        Symbol *s = escopo->simbolos;
        while (s) {
            if (strcmp(s->nome, nome) == 0) {
                return s;
            }
            s = s->prox;
        }
        escopo = escopo->prox;
    }

    return NULL;
}

// Função para inserir uma variável no escopo atual
int inserir_simbolo(ScopeStack *pilha, const char *nome, TipoSimbolo tipo, int linha) {
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
    
    // Insere a variável no início da lista encadadeada
    novo->prox = pilha->topo->simbolos;
    pilha->topo->simbolos = novo;

    return 1;
}

// Função para liberar a memória de todos os escopos
void liberar_pilha(ScopeStack *pilha) {
    while (pilha->topo) {
        desempilhar_escopo(pilha);
    }
}