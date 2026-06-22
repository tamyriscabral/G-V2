#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantico.h"
static int escopo = -1;

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
            analisar(no->filho1, pilha); // DeclVarGlobais
            analisar_lista(no->filho2, pilha); // DeclFunc
            analisar(no->filho3, pilha); // principal
               }
        
        // Criação da pilha de escopos no contexto de blocos
        // (garante que variáveis tenham visibilidade 
        // limitada ao bloco onde são declaradas)
        case AST_BLOCO: {
            empilhar_escopo(pilha);
            analisar(no->filho1, pilha); // VARSECTION
            analisar_lista(no->filho2, pilha); // comandos// comandos
            //desempilhar_escopo(pilha);
            break;
        } 

        case AST_VARSECTION: {
            escopo++;
            AST *decl = no->filho1;
            while (decl != NULL) {
                analisar(decl, pilha);  // Chama AST_DECLVAR
                decl = decl->irmao;
            }
            return TIPO_INT;
        }


        // Análise de declaração de variáveis
        // (comunica com a tabela de símbolos)
        case AST_DECLVAR: {
            if (!no->filho2) return TIPO_INT;
            TipoSimbolo tipo = analisar(no->filho2, pilha);
            
            AST *id = no->filho1;
            // Percorre a lista de identificadores
            while (id != NULL) {
                if (buscar_no_escopo_atual(pilha, id->lexema)) {
                    erro("Variavel ja declarada", id->linha);
                }
                if (id->tipo == AST_DECL_VETOR) {
                    int tamanho = atoi(id->filho1->lexema); 
                    inserir_simbolo_vetor(pilha, id->lexema, tipo, tamanho, id->linha, escopo, 0);
                } else {
                    inserir_simbolo(pilha, id->lexema, tipo, id->linha, escopo, 0);
                }

                id =  id->irmao;
            }
            return TIPO_INT;
        }

        // Converte string para enum interno
        case AST_TIPO: {
            if (strcmp(no->lexema, "int") == 0)
                return TIPO_INT;
            else
                return TIPO_CAR; 
        }
            
        case AST_COMANDO_VAZIO: {
            break;
        }

        // Verifica se a variável foi declarada
        case AST_LEIA: {
            AST *id = no->filho1;
            if (!buscar_simbolo(pilha, id->lexema)) {
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
            AST *lval = no->filho1;
            Symbol *s = buscar_simbolo(pilha, lval->lexema);
            if (!s) {
                erro("Variavel nao declarada", lval->linha);
            }

            // Se for acesso a vetor, valida o índice
            if (lval->tipo == AST_ACESS_VETOR) {
                TipoSimbolo tidx = analisar(lval->filho1, pilha);
                if (tidx != TIPO_INT)
                    erro("Indice de vetor deve ser INT", lval->linha);
            }

            TipoSimbolo t = analisar(no->filho2, pilha);
            if (s->tipo != t) {
                erro("Tipos incompativeis", no->linha);
            }
            return s->tipo;
        }
        // Analisa declaração de variáveis
        case AST_IDENT: {
            Symbol *s = buscar_simbolo(pilha, no->lexema);
            if (!s) {
                erro("Variavel nao declarada", no->linha);
            } else {
            }
            break;
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

        case AST_ACESS_VETOR: {
            Symbol *s = buscar_simbolo(pilha, no->lexema);
            if (!s) {
                erro("Variavel nao declarada", no->linha);
            }
            TipoSimbolo tidx = analisar(no->filho1, pilha);
            if (tidx != TIPO_INT) {
                erro("Indice de vetor deve ser INT", no->linha);
            }
            return s->tipo; // tipo do elemento do vetor
        }
        
        case AST_DECL_VETOR: {
            Symbol *s = buscar_simbolo(pilha, no->lexema);
            if (!s) {
                erro("Variavel nao declarada", no->linha);
            }
            TipoSimbolo tidx = analisar(no->filho1, pilha);
            if (tidx != TIPO_INT) {
                erro("Indice de vetor deve ser INT", no->linha);
            }
            return s->tipo;
        }

        case AST_PARAM: {
            TipoSimbolo tipo = analisar(no->filho2, pilha);
            if (buscar_no_escopo_atual(pilha, no->lexema)) {
                erro("Parametro ja declarado", no->linha);
            }
        
            inserir_simbolo(pilha, no->lexema, tipo, no->linha,  escopo, 1);
            break;
        }

        case AST_PARAM_VETOR: {
            // filho1 = AST_IDENT (nome do parâmetro)
            // filho2 = AST_TIPO (tipo do parâmetro)
            TipoSimbolo tipo = analisar(no->filho2, pilha);
            
            AST *id = no->filho1;
            if (buscar_no_escopo_atual(pilha, id->lexema)) {
                erro("Parametro vetor ja declarado", id->linha);
            }
            inserir_simbolo_vetor(pilha, id->lexema, tipo, 0, id->linha, escopo, 1); // tamanho 0 para parâmetro vetor
            break;
        }

        case AST_FUNCAO: {
            escopo++;
            analisar_lista(no->filho2, pilha);
            analisar(no->filho3, pilha);
            break;
        }


    }
    return TIPO_INT;

}

// Função que inicializa a pilha de escopos
void analisar_semantico(AST *raiz, ScopeStack *pilha) {

    empilhar_escopo(pilha);
    analisar(raiz->filho1, pilha);
    analisar_lista(raiz->filho2, pilha);
    analisar(raiz->filho3, pilha);
}