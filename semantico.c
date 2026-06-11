#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantico.h"

// Função auxiliar para apontar erros semânticos
static void erro(const char *msg, int linha) {
    printf("ERRO: %s %d\n", msg, linha);
    exit(1);
}

static TipoSimbolo analisar(AST *no, ScopeStack *pilha);

static void analisar_lista(AST *no, ScopeStack *pilha) {
    AST *atual = no;

    while (atual != NULL) {
        analisar(atual, pilha);
        atual = atual->irmao;
    }
}

// Função principal que percorre a AST
// É recursiva e retorna o tipo do nó analisado
static TipoSimbolo analisar(AST *no, ScopeStack *pilha) {
    if (!no) return TIPO_INT; // tipo padrão para nós nulos

    switch (no->tipo) {
        // Inicio da análise semântica (raiz)
        case AST_PROGRAMA: {
            return analisar(no->filho1, pilha); 
        }
        
        // Criação da pilha de escopos no contexto de blocos
        // (garante que variáveis tenham visibilidade 
        // limitada ao bloco onde são declaradas)
        case AST_BLOCO: {
            empilhar_escopo(pilha);
            analisar(no->filho1, pilha); // VARSECTION
            analisar_lista(no->filho2, pilha); // comandos// comandos
            desempilhar_escopo(pilha);
            break;
        } 

        case AST_VARSECTION: {
            analisar(no->filho1, pilha);
            break;
        }

        // Análise de declaração de variáveis
        // (comunica com a tabela de símbolos)
        case AST_DECLVAR: {
            // guarda o tipo da variável
            TipoSimbolo tipo = analisar(no->filho2, pilha);

            // Percorre a lista de identificadores
            for (AST *id = no->filho1; id != NULL; id = id->irmao) {
                if (buscar_no_escopo_atual(pilha, id->lexema)) {
                    erro("Variavel ja declarada", id->linha);
                }
                inserir_simbolo(pilha, id->lexema, tipo, id->linha);
            }
            break;
        }

        // Converte string para enum interno
        case AST_TIPO: {
            if (strcmp(no->lexema, "int") == 0)
                return TIPO_INT;
            else
                return TIPO_CAR; 
        }
            
        case AST_LISTACOMANDO: {
            analisar(no->filho1, pilha);
            break;
        }
            
        case AST_COMANDO_VAZIO: {
            break;
        }

        // Verifica se a variável foi declarada
        case AST_LEIA: {
            AST *id = no->filho1;
            if (!buscar_na_pilha(pilha, id->lexema)) {
                erro("Variavel nao declarada", id->linha);
            }
            break;
        }

        // Apenas valida a expressão
        case AST_ESCREVA: {
            analisar(no->filho1, pilha);
            break;
        }
            
        case AST_NOVALINHA: {
            break;
        }
            
        // Analisa condição
        case AST_SE: {
            TipoSimbolo t = analisar(no->filho1, pilha);
            
            // Condição deve ser do tipo INT, pois é tratamento 
            // booleano dentro do contexto da linguagem
            if (t != TIPO_INT) {
                erro("Condicao deve ser INT", no->linha);
            }
            analisar(no->filho2, pilha);
            analisar(no->filho3, pilha);
            break;
        }

        // Analisa loop (mesma lógica booleana de SE)
        case AST_ENQUANTO: {
            TipoSimbolo t = analisar(no->filho1, pilha);
            if (t != TIPO_INT) {
                erro("Condicao deve ser INT", no->linha);
            }
            analisar(no->filho2, pilha);
            break;
        }

        // Analisa atribuições
        case AST_ATRIB: {
            AST *id = no->filho1;

            // Verifica se a variável existe, buscando na pilha
            Symbol *s = buscar_na_pilha(pilha, id->lexema);

            if (!s) {
                erro("Variavel nao declarada", id->linha);
            }

            // Verifica o tipo da variável
            // para evitar "int = car"
            TipoSimbolo t = analisar(no->filho2, pilha);

            if (s->tipo != t) {
                erro("Tipos incompativeis", no->linha);
            }

            return s->tipo;
        }

        // Analisa declaração de variáveis
        case AST_IDENT: {
            Symbol *s = buscar_na_pilha(pilha, no->lexema);
            if (!s) {
                erro("Variavel nao declarada", no->linha);
            }
            return s->tipo;
        }

        // Define o tipo das constantes
        case AST_INTCONST: {
            return TIPO_INT;
        }
            
        case AST_CARCONST: {
            return TIPO_CAR;
        }
            
        case AST_STRING: {
            return TIPO_CAR;
        }

        // Analisa operações entre variáveis
        // (e.g "int + car" não pode acontecer)
        case AST_OP: {
            TipoSimbolo t1 = analisar(no->filho1, pilha);
            TipoSimbolo t2 = no->filho2 ? analisar(no->filho2, pilha) : TIPO_INT;

            // Aritméticos
            if (!strcmp(no->lexema, "+") ||
                !strcmp(no->lexema, "-") ||
                !strcmp(no->lexema, "*") ||
                !strcmp(no->lexema, "/")) {

                if (t1 != TIPO_INT || t2 != TIPO_INT)
                    erro("Operacao aritmetica requer INT", no->linha);

                return TIPO_INT;
            }

            // Relacionais
            if (!strcmp(no->lexema, "==") ||
                !strcmp(no->lexema, "!=") ||
                !strcmp(no->lexema, "<") ||
                !strcmp(no->lexema, ">") ||
                !strcmp(no->lexema, "<=") ||
                !strcmp(no->lexema, ">=")) {

                if (t1 != t2)
                    erro("Comparacao com tipos diferentes", no->linha);

                return TIPO_INT;
            }

            // Lógicos
            if (!strcmp(no->lexema, "||") ||
                !strcmp(no->lexema, "&")) {

                if (t1 != TIPO_INT || t2 != TIPO_INT)
                    erro("Operacao logica requer INT", no->linha);

                return TIPO_INT;
            }

            // Unários
            if (!strcmp(no->lexema, "!") ||
                !strcmp(no->lexema, "unary-")) {

                if (t1 != TIPO_INT)
                    erro("Operador unario requer INT", no->linha);

                return TIPO_INT;
            }
            
            break;
        }
    }

    // Permite percorrer listas de comandos e declarações
    if (no->irmao) {
        analisar(no->irmao, pilha);
    }
        
    return TIPO_INT;
}

// Função que inicializa a pilha de escopos
void analisar_semantico(AST *raiz) {
    ScopeStack pilha;
    iniciar_pilha(&pilha);  // escopo global

    analisar(raiz, &pilha);

    liberar_pilha(&pilha);  // libera memória da tabela de símbolos
}
