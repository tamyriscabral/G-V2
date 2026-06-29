#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantico.h"
static int escopo = -1;
static int dentro_funcao = 0;
static TipoSimbolo tipo_retorno_atual = TIPO_INT;

// Armazena informações sobre os parâmetros de cada função.
typedef struct ParamInfo {
    TipoSimbolo tipo;
    int eh_vetor;
    struct ParamInfo *prox;
} ParamInfo;

// Representa a assinatura de uma função para validação de chamadas.
typedef struct FuncInfo {
    char *nome;
    TipoSimbolo retorno;
    int num_params;
    ParamInfo *params;
    struct FuncInfo *prox;
} FuncInfo;

static FuncInfo *funcoes = NULL;

// Função auxiliar para apontar erros semânticos
static void erro(const char *msg, int linha) {
    printf("ERRO: %s %d\n", msg, linha);
    exit(1);
}

static TipoSimbolo analisar(AST *no, ScopeStack *pilha);
static void analisar_bloco(AST *no, ScopeStack *pilha, int reutilizar_escopo);

// Converte um nó AST de tipo para o enum utilizado internamente.
static TipoSimbolo tipo_ast(AST *tipo) {
    if (tipo && tipo->lexema && strcmp(tipo->lexema, "car") == 0)
        return TIPO_CAR;
    return TIPO_INT;
}

// Conta o número de elementos em uma lista da AST.
static int contar_lista(AST *no) {
    int total = 0;
    for (AST *p = no; p; p = p->irmao)
        total++;
    return total;
}

// Procura uma função previamente registrada.
static FuncInfo *buscar_funcao(const char *nome) {
    for (FuncInfo *f = funcoes; f; f = f->prox)
        if (strcmp(f->nome, nome) == 0)
            return f;
    return NULL;
}

// Registra a assinatura de uma função antes da análise de seu corpo.
static void registrar_assinatura(AST *func) {
    if (buscar_funcao(func->lexema))
        erro("Funcao ja declarada", func->linha);

    FuncInfo *info = malloc(sizeof(FuncInfo));
    if (!info) {
        fprintf(stderr, "Erro de alocacao de memoria\n");
        exit(1);
    }

    info->nome = strdup(func->lexema);
    info->retorno = tipo_ast(func->filho1);
    info->num_params = 0;
    info->params = NULL;
    info->prox = funcoes;
    funcoes = info;

    ParamInfo **fim = &info->params;
    for (AST *p = func->filho2; p; p = p->irmao) {
        ParamInfo *param = malloc(sizeof(ParamInfo));
        if (!param) {
            fprintf(stderr, "Erro de alocacao de memoria\n");
            exit(1);
        }

        param->tipo = tipo_ast(p->filho2);
        param->eh_vetor = (p->tipo == AST_PARAM_VETOR);
        param->prox = NULL;
        *fim = param;
        fim = &param->prox;
        info->num_params++;
    }
}

// Registra todas as funções declaradas no programa.
static void registrar_assinaturas(AST *funcs) {
    for (AST *f = funcs; f; f = f->irmao)
        registrar_assinatura(f);
}

// Libera a memória utilizada pelas assinaturas registradas.
static void liberar_assinaturas(void) {
    while (funcoes) {
        FuncInfo *f = funcoes;
        funcoes = funcoes->prox;

        while (f->params) {
            ParamInfo *p = f->params;
            f->params = p->prox;
            free(p);
        }

        free(f->nome);
        free(f);
    }
}

// Verifica se uma expressão representa um vetor.
static int expr_eh_vetor(AST *no, ScopeStack *pilha) {
    if (!no)
        return 0;

    if (no->tipo == AST_IDENT) {
        Symbol *s = buscar_simbolo(pilha, no->lexema);
        return s && s->eh_vetor;
    }

    if (no->tipo == AST_ATRIB)
        return expr_eh_vetor(no->filho1, pilha);

    return 0;
}

// Analisa todos os nós pertencentes a uma lista da AST.
static void analisar_lista(AST *no, ScopeStack *pilha) {
    AST *atual = no;

    while (atual != NULL) {
        analisar(atual, pilha);
        atual = atual->irmao;
    }
}

// Gerencia escopos e realiza a análise semântica de um bloco.
static void analisar_bloco(AST *no, ScopeStack *pilha, int reutilizar_escopo) {
    if (!no)
        return;

    if (!reutilizar_escopo)
        empilhar_escopo(pilha);

    analisar(no->filho1, pilha);
    analisar_lista(no->filho2, pilha);

    if (!reutilizar_escopo)
        desempilhar_escopo(pilha);
}

// Função principal que percorre a AST
// Percorre recursivamente a AST realizando todas as verificações semânticas.
// É recursiva e retorna o tipo do nó analisado
static TipoSimbolo analisar(AST *no, ScopeStack *pilha) {
    if (!no) return TIPO_INT; // tipo padrão para nós nulos

    switch (no->tipo) {
        // Inicio da análise semântica (raiz)

        // Analisa variáveis globais, funções e o bloco principal.
        case AST_PROGRAMA: {
            analisar(no->filho1, pilha); // DeclVarGlobais
            analisar_lista(no->filho2, pilha); // DeclFunc
            analisar(no->filho3, pilha); // principal
            break;
        }
        
        // Criação da pilha de escopos no contexto de blocos
        // (garante que variáveis tenham visibilidade 
        // limitada ao bloco onde são declaradas)
        case AST_BLOCO: {
            analisar_bloco(no, pilha, 0);
            break;
        } 

        // Analisa todas as declarações de variáveis do bloco.
        case AST_VARSECTION: {
            escopo++;
            AST *decl = no->filho1;
            while (decl != NULL) {
                analisar(decl, pilha);  // Chama AST_DECLVAR
                decl = decl->irmao;
            }
            return TIPO_INT;
        }


        // Verifica redeclarações e registra as variáveis na tabela de símbolos.
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
            if (no->filho1 && no->filho1->tipo == AST_IDENT) {
                Symbol *s = buscar_simbolo(pilha, no->filho1->lexema);
                if (!s)
                    erro("Variavel nao declarada", no->filho1->linha);
                if (s->eh_vetor)
                    erro("Leitura requer variavel escalar", no->filho1->linha);
            } else {
                analisar(no->filho1, pilha);
            }
            break;
        }

        // Impede a escrita direta de vetores.
        case AST_ESCREVA: {
            if (expr_eh_vetor(no->filho1, pilha))
                erro("Escrita requer expressao escalar", no->linha);
            analisar(no->filho1, pilha);
            break;
        }
            
        case AST_NOVALINHA: {
            break;
        }
            
        // A condição de um "se" deve produzir um valor inteiro.
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

        // Valida a expressão de controle do laço.
        case AST_ENQUANTO: {
            TipoSimbolo t = analisar(no->filho1, pilha);
            if (t != TIPO_INT) {
                erro("Condicao deve ser INT", no->linha);
            }
            analisar(no->filho2, pilha);
            break;
        }

        // Verifica compatibilidade entre a variável e a expressão atribuída.
        case AST_ATRIB: {
            AST *lval = no->filho1;
            Symbol *s = buscar_simbolo(pilha, lval->lexema);
            if (!s) {
                erro("Variavel nao declarada", lval->linha);
            }

            // Se for acesso a vetor, valida o índice
            if (lval->tipo == AST_ACESS_VETOR) {
                if (!s->eh_vetor)
                    erro("Variavel nao e vetor", lval->linha);
                TipoSimbolo tidx = analisar(lval->filho1, pilha);
                if (tidx != TIPO_INT)
                    erro("Indice de vetor deve ser INT", lval->linha);
            }

            TipoSimbolo t = analisar(no->filho2, pilha);
            int lval_eh_vetor = (lval->tipo == AST_IDENT && s->eh_vetor);
            int expr_vetor = expr_eh_vetor(no->filho2, pilha);

            if (lval_eh_vetor != expr_vetor) {
                erro("Tipos incompativeis", no->linha);
            }

            if (s->tipo != t) {
                erro("Tipos incompativeis", no->linha);
            }
            return s->tipo;
        }
        
        // Garante que o identificador foi previamente declarado.
        case AST_IDENT: {
            Symbol *s = buscar_simbolo(pilha, no->lexema);
            if (!s) {
                erro("Variavel nao declarada", no->linha);
            } else {
                return s->tipo;
            }
            break;
        }

        // Constantes inteiras possuem tipo INT.
        case AST_INTCONST: {
            return TIPO_INT;
        }
        
        // Constantes de caractere possuem tipo CAR.
        case AST_CARCONST: {
            return TIPO_CAR;
        }
        
        // Cadeias de caracteres são tratadas como tipo CAR.
        case AST_STRING: {
            return TIPO_CAR;
        }

        // Valida operandos de operações aritméticas, relacionais e lógicas.
        // (e.g "int + car" não pode acontecer)
        case AST_OP: {
            TipoSimbolo t1 = analisar(no->filho1, pilha);
            TipoSimbolo t2 = no->filho2 ? analisar(no->filho2, pilha) : TIPO_INT;

            if (expr_eh_vetor(no->filho1, pilha) ||
                (no->filho2 && expr_eh_vetor(no->filho2, pilha))) {
                erro("Operacao com vetor invalida", no->linha);
            }

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

        // Verifica se o acesso ao vetor é válido.
        case AST_ACESS_VETOR: {
            Symbol *s = buscar_simbolo(pilha, no->lexema);
            if (!s) {
                erro("Variavel nao declarada", no->linha);
            }
            if (!s->eh_vetor) {
                erro("Variavel nao e vetor", no->linha);
            }
            TipoSimbolo tidx = analisar(no->filho1, pilha);
            if (tidx != TIPO_INT) {
                erro("Indice de vetor deve ser INT", no->linha);
            }
            return s->tipo; // tipo do elemento do vetor
        }
        
        // Valida o índice utilizado em declarações/acessos a vetores.
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

        // Registra um parâmetro escalar da função.
        case AST_PARAM: {
            TipoSimbolo tipo = analisar(no->filho2, pilha);
            if (buscar_no_escopo_atual(pilha, no->lexema)) {
                erro("Parametro ja declarado", no->linha);
            }
        
            inserir_simbolo(pilha, no->lexema, tipo, no->linha,  escopo, 1);
            break;
        }

        // Registra um parâmetro passado por referência (vetor).
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

        // Analisa o corpo da função dentro de um novo escopo.
        case AST_FUNCAO: {
            TipoSimbolo retorno_anterior = tipo_retorno_atual;
            int dentro_anterior = dentro_funcao;

            escopo++;
            empilhar_escopo(pilha);
            tipo_retorno_atual = tipo_ast(no->filho1);
            dentro_funcao = 1;

            analisar_lista(no->filho2, pilha);
            analisar_bloco(no->filho3, pilha, 1);

            desempilhar_escopo(pilha);
            tipo_retorno_atual = retorno_anterior;
            dentro_funcao = dentro_anterior;
            break;
        }

        // Verifica se o retorno é válido e compatível com a função.
        case AST_RETORNE: {
            TipoSimbolo t;

            if (!dentro_funcao)
                erro("Retorne fora de funcao", no->linha);

            t = analisar(no->filho1, pilha);
            if (t != tipo_retorno_atual)
                erro("Tipo de retorno incompativel", no->linha);

            if (expr_eh_vetor(no->filho1, pilha))
                erro("Retorno nao pode ser vetor", no->linha);

            return t;
        }

        // Confere quantidade e tipos dos argumentos da chamada.
        case AST_CHAMADA_FUNC: {
            FuncInfo *f = buscar_funcao(no->lexema);
            ParamInfo *param;
            AST *arg;

            if (!f)
                erro("Funcao nao declarada", no->linha);

            if (contar_lista(no->filho1) != f->num_params)
                erro("Numero de argumentos incompativel", no->linha);

            param = f->params;
            arg = no->filho1;

            while (param && arg) {
                TipoSimbolo tipo_arg = analisar(arg, pilha);
                int arg_vetor = expr_eh_vetor(arg, pilha);

                if (tipo_arg != param->tipo || arg_vetor != param->eh_vetor)
                    erro("Argumento incompativel", arg->linha);

                param = param->prox;
                arg = arg->irmao;
            }

            return f->retorno;
        }


    }
    return TIPO_INT;

}

// Inicializa as estruturas e inicia a análise semântica da AST.
void analisar_semantico(AST *raiz, ScopeStack *pilha) {

    escopo = -1;
    dentro_funcao = 0;
    tipo_retorno_atual = TIPO_INT;
    liberar_assinaturas();
    registrar_assinaturas(raiz->filho2);

    empilhar_escopo(pilha);
    analisar(raiz, pilha);
    liberar_assinaturas();
}