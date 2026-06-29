#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"


// Inicializa a pilha de escopos vazia.
void iniciar_pilha(ScopeStack *pilha) {
    pilha->topo = NULL;
    pilha->base = NULL;
}

// Cria um novo escopo e o coloca no topo da pilha.
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
    if (!pilha->base) {
        pilha->base = novo;
    }
}

// Remove o escopo atual e libera todos os seus símbolos.
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
    if (pilha->base == escopo) {
        pilha->base = NULL;
    }
    free(escopo);
}

// Procura um identificador apenas no escopo corrente.
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

// Procura um identificador exclusivamente no escopo global.
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

// Procura um símbolo do escopo mais interno para o mais externo.
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

// Insere uma variável escalar no escopo atual.
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

    // Inicializa todas as informações associadas ao símbolo.
    novo->nome = strdup(nome);
    novo->tipo = tipo;
    novo->linha = linha;
    novo->eh_vetor = 0;
    novo->tamanho = 0;
    novo->escopo = escopo;
    novo->parametro = parametro;
    novo->pos = 0;
    // Insere a variável no início da lista encadadeada
    novo->prox = pilha->topo->simbolos;
    pilha->topo->simbolos = novo;

    return 1;
}

// Insere uma variável do tipo vetor no escopo atual.
int inserir_simbolo_vetor(ScopeStack *pilha, const char *nome,
                            TipoSimbolo tipo, int tamanho, int linha, int escopo, int parametro) {
    if (!pilha->topo) return 0;

    if (buscar_no_escopo_atual(pilha, nome) != NULL) {
        return 0;
    }

    Symbol *s = malloc(sizeof(Symbol));
    if (!s) {
        fprintf(stderr, "Erro de alocacao de memoria\n");
        exit(1);
    }

    s->nome     = strdup(nome);
    s->tipo     = tipo;
    s->eh_vetor = 1;
    s->tamanho  = tamanho;
    s->linha    = linha;
    s->escopo = escopo;
    s->parametro = parametro;
    s->pos = 0;
    s->prox     = pilha->topo->simbolos;  
    pilha->topo->simbolos = s;

    return 1;
}

// Utiliza a busca padrão de símbolos.
Symbol *buscar_simbolo_parametros(ScopeStack *pilha, const char *nome) {
    return buscar_simbolo(pilha, nome);
}

// Função para liberar a memória de todos os escopos
void liberar_pilha(ScopeStack *pilha) {
    while (pilha->topo) {
        desempilhar_escopo(pilha);
    }
}