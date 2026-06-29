#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codigo.h"

// Estruturas auxiliares para armazenar strings literais e informações das funções.
typedef struct StringLiteral {
    char label[32];
    char *valor;
    struct StringLiteral *prox;
} StringLiteral;

typedef struct FunctionInfo {
    char *nome;
    AST *tipo;
    AST *params;
    struct FunctionInfo *prox;
} FunctionInfo;

static ScopeStack pilha_codigo;
static StringLiteral *strings = NULL;
static FunctionInfo *funcoes = NULL;

static int label_counter = 0;
static int string_counter = 0;
static int escopo_atual = 0;
static int offset_local_atual = 0;
static int label_retorno_atual = -1;

static void gerar(AST *no);
static void gerar_bloco(AST *no, int reutilizar_escopo);


// Gera um identificador único para labels utilizados em desvios.
static int novo_label(void) {
    return label_counter++;
}

// Converte um nó de tipo da AST para o tipo interno da tabela de símbolos.
static TipoSimbolo tipo_de_no(AST *tipo) {
    if (tipo && tipo->lexema && strcmp(tipo->lexema, "car") == 0)
        return TIPO_CAR;
    return TIPO_INT;
}

// Obtém o tamanho declarado de um vetor na AST.
static int tamanho_decl_vetor(AST *id) {
    if (!id || !id->filho1 || !id->filho1->lexema)
        return 0;
    return atoi(id->filho1->lexema);
}

// Conta a quantidade de elementos em uma lista ligada da AST.
static int contar_lista(AST *no) {
    int total = 0;
    for (AST *p = no; p; p = p->irmao)
        total++;
    return total;
}

// Procura um símbolo durante a geração de código e aborta em caso de inconsistência.
static Symbol *buscar_codigo(const char *nome) {
    Symbol *s = buscar_simbolo(&pilha_codigo, nome);
    if (!s) {
        fprintf(stderr, "Erro interno: simbolo nao encontrado na geracao: %s\n", nome);
        exit(1);
    }
    return s;
}

// Emite instruções para carregar o valor de uma variável (global ou local).
static void emitir_load_simbolo(Symbol *s, const char *reg) {
    if (s->escopo == 0) {
        printf("lw %s, _g_%s\n", reg, s->nome);
    } else {
        printf("lw %s, %d($fp)\n", reg, s->pos);
    }
}

// Emite instruções para armazenar um valor em uma variável.
static void emitir_store_simbolo(Symbol *s, const char *reg) {
    if (s->escopo == 0) {
        printf("sw %s, _g_%s\n", reg, s->nome);
    } else {
        printf("sw %s, %d($fp)\n", reg, s->pos);
    }
}

// Calcula o endereço de memória correspondente a um identificador.
static void emitir_endereco_simbolo(Symbol *s, const char *reg) {
    if (s->escopo == 0) {
        printf("la %s, _g_%s\n", reg, s->nome);
    } else {
        printf("addiu %s, $fp, %d\n", reg, s->pos);
    }
}

// Obtém o endereço base de um vetor para acesso indexado.
static void emitir_base_vetor(Symbol *s, const char *reg) {
    if (!s->eh_vetor) {
        fprintf(stderr, "Erro interno: %s nao e vetor\n", s->nome);
        exit(1);
    }
    emitir_load_simbolo(s, reg);
}

// Empilha o conteúdo de $s0 na pilha de execução.
static void emitir_push_s0(void) {
    printf("addiu $sp, $sp, -4\n");
    printf("sw $s0, 0($sp)\n");
}

// Remove o topo da pilha e armazena no registrador informado.
static void emitir_pop_reg(const char *reg) {
    printf("lw %s, 0($sp)\n", reg);
    printf("addiu $sp, $sp, 4\n");
}

// Converte constantes do tipo caractere para seus valores inteiros.
static int valor_caractere(const char *lexema) {
    if (!lexema)
        return 0;

    if (lexema[1] != '\\')
        return (unsigned char) lexema[1];

    switch (lexema[2]) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '0': return '\0';
        case '\'': return '\'';
        case '"': return '"';
        case '\\': return '\\';
        default: return (unsigned char) lexema[2];
    }
}

// Registra uma nova string literal na seção .data.
static const char *registrar_string(const char *valor) {
    StringLiteral *novo = malloc(sizeof(StringLiteral));
    if (!novo) {
        fprintf(stderr, "Erro de alocacao de memoria\n");
        exit(1);
    }

    snprintf(novo->label, sizeof(novo->label), "_str%d", string_counter++);
    novo->valor = strdup(valor);
    novo->prox = strings;
    strings = novo;

    return novo->label;
}

// Evita duplicação de strings reutilizando labels já existentes.
static const char *buscar_string(const char *valor) {
    for (StringLiteral *s = strings; s; s = s->prox)
        if (strcmp(s->valor, valor) == 0)
            return s->label;
    return registrar_string(valor);
}

// Percorre a AST coletando todas as strings utilizadas pelo programa.
static void coletar_strings(AST *no) {
    if (!no)
        return;

    if (no->tipo == AST_STRING)
        buscar_string(no->lexema);

    coletar_strings(no->filho1);
    coletar_strings(no->filho2);
    coletar_strings(no->filho3);
    coletar_strings(no->irmao);
}

// Armazena informações das funções para consultas futuras.
static void registrar_funcoes(AST *no) {
    for (AST *f = no; f; f = f->irmao) {
        FunctionInfo *info = malloc(sizeof(FunctionInfo));
        if (!info) {
            fprintf(stderr, "Erro de alocacao de memoria\n");
            exit(1);
        }

        info->nome = strdup(f->lexema);
        info->tipo = f->filho1;
        info->params = f->filho2;
        info->prox = funcoes;
        funcoes = info;
    }
}

static FunctionInfo *buscar_funcao(const char *nome) {
    for (FunctionInfo *f = funcoes; f; f = f->prox)
        if (strcmp(f->nome, nome) == 0)
            return f;
    return NULL;
}

// Obtém o tipo de retorno de uma expressão.
static TipoSimbolo tipo_expr(AST *no) {
    if (!no)
        return TIPO_INT;

    switch (no->tipo) {
        case AST_CARCONST:
            return TIPO_CAR;

        case AST_IDENT:
        case AST_ACESS_VETOR:
            return buscar_codigo(no->lexema)->tipo;

        case AST_CHAMADA_FUNC: {
            FunctionInfo *f = buscar_funcao(no->lexema);
            return f ? tipo_de_no(f->tipo) : TIPO_INT;
        }

        case AST_ATRIB:
            return tipo_expr(no->filho1);

        case AST_OP:
            return TIPO_INT;

        default:
            return TIPO_INT;
    }
}

// Gera a declaração de uma variável global na seção .data.
static void emitir_decl_global(AST *id, TipoSimbolo tipo) {
    if (id->tipo == AST_DECL_VETOR) {
        int tamanho = tamanho_decl_vetor(id);
        inserir_simbolo_vetor(&pilha_codigo, id->lexema, tipo, tamanho,
                              id->linha, 0, 0);

        printf("_g_%s: .word _g_%s_dados\n", id->lexema, id->lexema);
        printf("_g_%s_dados: .space %d\n", id->lexema, tamanho * 4);
    } else {
        inserir_simbolo(&pilha_codigo, id->lexema, tipo, id->linha, 0, 0);
        printf("_g_%s: .word 0\n", id->lexema);
    }
}

// Percorre todas as declarações globais do programa.
static void emitir_globais(AST *varsection) {
    if (!varsection)
        return;

    for (AST *decl = varsection->filho1; decl; decl = decl->irmao) {
        TipoSimbolo tipo = tipo_de_no(decl->filho2);
        for (AST *id = decl->filho1; id; id = id->irmao)
            emitir_decl_global(id, tipo);
    }
}

// Reserva espaço para uma variável local simples na pilha.
static int alocar_local_simples(Symbol *s) {
    offset_local_atual += 4;
    s->pos = -offset_local_atual;

    printf("addiu $sp, $sp, -4\n");
    return 4;
}

// Reserva memória para vetores locais e inicializa seu endereço base.
static int alocar_local_vetor(Symbol *s, int tamanho) {
    int bytes_dados = tamanho * 4;
    int bytes_total = 4 + bytes_dados;

    offset_local_atual += 4;
    s->pos = -offset_local_atual;
    printf("addiu $sp, $sp, -4\n");

    if (bytes_dados > 0) {
        offset_local_atual += bytes_dados;
        printf("addiu $sp, $sp, -%d\n", bytes_dados);
    }

    printf("addiu $t0, $fp, %d\n", s->pos - bytes_dados);
    printf("sw $t0, %d($fp)\n", s->pos);

    return bytes_total;
}

// Registra variáveis locais na tabela de símbolos e realiza sua alocação.
static int registrar_locais(AST *varsection) {
    int bytes = 0;

    if (!varsection)
        return 0;

    for (AST *decl = varsection->filho1; decl; decl = decl->irmao) {
        TipoSimbolo tipo = tipo_de_no(decl->filho2);

        for (AST *id = decl->filho1; id; id = id->irmao) {
            Symbol *s;

            if (id->tipo == AST_DECL_VETOR) {
                int tamanho = tamanho_decl_vetor(id);
                inserir_simbolo_vetor(&pilha_codigo, id->lexema, tipo, tamanho,
                                      id->linha, escopo_atual, 0);
                s = buscar_no_escopo_atual(&pilha_codigo, id->lexema);
                bytes += alocar_local_vetor(s, tamanho);
            } else {
                inserir_simbolo(&pilha_codigo, id->lexema, tipo, id->linha,
                                escopo_atual, 0);
                s = buscar_no_escopo_atual(&pilha_codigo, id->lexema);
                bytes += alocar_local_simples(s);
            }
        }
    }

    return bytes;
}

// Calcula o endereço correspondente ao lado esquerdo de uma atribuição.
static void gerar_endereco_lvalue(AST *lval) {
    if (lval->tipo == AST_IDENT) {
        Symbol *s = buscar_codigo(lval->lexema);
        emitir_endereco_simbolo(s, "$t0");
        return;
    }

    if (lval->tipo == AST_ACESS_VETOR) {
        Symbol *s = buscar_codigo(lval->lexema);

        gerar(lval->filho1);
        printf("sll $t1, $s0, 2\n");
        emitir_base_vetor(s, "$t0");
        printf("addu $t0, $t0, $t1\n");
        return;
    }

    fprintf(stderr, "Erro interno: lvalue invalido\n");
    exit(1);
}

// Gera código para todos os elementos de uma lista da AST.
static void gerar_lista(AST *no) {
    for (AST *p = no; p; p = p->irmao)
        gerar(p);
}

// Empilha argumentos e realiza a chamada de uma função.
static void gerar_chamada_funcao(AST *no) {
    int n_args = contar_lista(no->filho1);

    // Os argumentos são empilhados da direita para a esquerda.
    if (n_args > 0) {
        AST **args = malloc(sizeof(AST *) * n_args);
        if (!args) {
            fprintf(stderr, "Erro de alocacao de memoria\n");
            exit(1);
        }

        int i = 0;
        for (AST *a = no->filho1; a; a = a->irmao)
            args[i++] = a;

        for (int j = n_args - 1; j >= 0; j--) {
            gerar(args[j]);
            emitir_push_s0();
        }

        free(args);
    }

    printf("jal _f_%s\n", no->lexema);

    if (n_args > 0)
        printf("addiu $sp, $sp, %d\n", n_args * 4);
}

// Gera código para operações de atribuição.
static void gerar_atribuicao(AST *no) {
    gerar(no->filho2);
    emitir_push_s0();

    gerar_endereco_lvalue(no->filho1);

    emitir_pop_reg("$t1");
    printf("sw $t1, 0($t0)\n");
    printf("move $s0, $t1\n");
}

// Traduz operadores aritméticos, relacionais e lógicos para MIPS.
static void gerar_operacao(AST *no) {
    if (strcmp(no->lexema, "unary-") == 0) {
        gerar(no->filho1);
        printf("subu $s0, $zero, $s0\n");
        return;
    }

    if (strcmp(no->lexema, "!") == 0) {
        gerar(no->filho1);
        printf("seq $s0, $s0, $zero\n");
        return;
    }

    // Primeiro operando é preservado na pilha enquanto o segundo é avaliado.
    gerar(no->filho1);
    emitir_push_s0();

    gerar(no->filho2);
    emitir_pop_reg("$t1");

    if (strcmp(no->lexema, "+") == 0) {
        printf("addu $s0, $t1, $s0\n");
    } else if (strcmp(no->lexema, "-") == 0) {
        printf("subu $s0, $t1, $s0\n");
    } else if (strcmp(no->lexema, "*") == 0) {
        printf("mul $s0, $t1, $s0\n");
    } else if (strcmp(no->lexema, "/") == 0) {
        printf("div $t1, $s0\n");
        printf("mflo $s0\n");
    } else if (strcmp(no->lexema, "==") == 0) {
        printf("seq $s0, $t1, $s0\n");
    } else if (strcmp(no->lexema, "!=") == 0) {
        printf("sne $s0, $t1, $s0\n");
    } else if (strcmp(no->lexema, "<") == 0) {
        printf("slt $s0, $t1, $s0\n");
    } else if (strcmp(no->lexema, ">") == 0) {
        printf("sgt $s0, $t1, $s0\n");
    } else if (strcmp(no->lexema, "<=") == 0) {
        printf("sle $s0, $t1, $s0\n");
    } else if (strcmp(no->lexema, ">=") == 0) {
        printf("sge $s0, $t1, $s0\n");
    } else if (strcmp(no->lexema, "||") == 0) {
        printf("sne $t1, $t1, $zero\n");
        printf("sne $s0, $s0, $zero\n");
        printf("or $s0, $t1, $s0\n");
    } else if (strcmp(no->lexema, "&") == 0) {
        printf("sne $t1, $t1, $zero\n");
        printf("sne $s0, $s0, $zero\n");
        printf("and $s0, $t1, $s0\n");
    }
}

// Gera código para leitura via syscall e armazenamento da variável.
static void gerar_leia(AST *no) {
    TipoSimbolo tipo = tipo_expr(no->filho1);

    printf("li $v0, %d\n", tipo == TIPO_CAR ? 12 : 5);
    printf("syscall\n");
    printf("move $s0, $v0\n");
    emitir_push_s0();

    gerar_endereco_lvalue(no->filho1);

    emitir_pop_reg("$t1");
    printf("sw $t1, 0($t0)\n");
}

// Gera código para impressão de strings, inteiros ou caracteres.
static void gerar_escreva(AST *no) {
    if (no->filho1 && no->filho1->tipo == AST_STRING) {
        const char *label = buscar_string(no->filho1->lexema);
        printf("la $a0, %s\n", label);
        printf("li $v0, 4\n");
        printf("syscall\n");
        return;
    }

    TipoSimbolo tipo = tipo_expr(no->filho1);
    gerar(no->filho1);
    printf("move $a0, $s0\n");
    printf("li $v0, %d\n", tipo == TIPO_CAR ? 11 : 1);
    printf("syscall\n");
}

// Função principal de geração de código para cada tipo de nó da AST.
static void gerar(AST *no) {
    if (!no)
        return;

    switch (no->tipo) {
        case AST_BLOCO:
            gerar_bloco(no, 0);
            break;

        case AST_COMANDO_VAZIO:
            break;

        case AST_INTCONST:
            printf("li $s0, %s\n", no->lexema);
            break;

        case AST_CARCONST:
            printf("li $s0, %d\n", valor_caractere(no->lexema));
            break;

        case AST_IDENT: {
            Symbol *s = buscar_codigo(no->lexema);
            emitir_load_simbolo(s, "$s0");
            break;
        }

        case AST_ACESS_VETOR:
            gerar_endereco_lvalue(no);
            printf("lw $s0, 0($t0)\n");
            break;

        case AST_ATRIB:
            gerar_atribuicao(no);
            break;

        case AST_OP:
            gerar_operacao(no);
            break;

        case AST_SE: {
            int label_senao = novo_label();
            int label_fim = novo_label();

            gerar(no->filho1);

            if (no->filho3) {
                printf("beq $s0, $zero, L%d\n", label_senao);
                gerar(no->filho2);
                printf("b L%d\n", label_fim);
                printf("L%d:\n", label_senao);
                gerar(no->filho3);
                printf("L%d:\n", label_fim);
            } else {
                printf("beq $s0, $zero, L%d\n", label_fim);
                gerar(no->filho2);
                printf("L%d:\n", label_fim);
            }
            break;
        }

        case AST_ENQUANTO: {
            int label_inicio = novo_label();
            int label_fim = novo_label();

            printf("L%d:\n", label_inicio);
            gerar(no->filho1);
            printf("beq $s0, $zero, L%d\n", label_fim);
            gerar(no->filho2);
            printf("b L%d\n", label_inicio);
            printf("L%d:\n", label_fim);
            break;
        }

        case AST_LEIA:
            gerar_leia(no);
            break;

        case AST_ESCREVA:
            gerar_escreva(no);
            break;

        case AST_NOVALINHA:
            printf("la $a0, _newline\n");
            printf("li $v0, 4\n");
            printf("syscall\n");
            break;

        case AST_RETORNE:
            gerar(no->filho1);
            printf("b L%d\n", label_retorno_atual);
            break;

        case AST_CHAMADA_FUNC:
            gerar_chamada_funcao(no);
            break;

        default:
            break;
    }
}

// Cria e destrói escopos, além de gerenciar variáveis locais do bloco.
static void gerar_bloco(AST *no, int reutilizar_escopo) {
    int bytes_locais;
    int offset_anterior = offset_local_atual;

    if (!reutilizar_escopo) {
        empilhar_escopo(&pilha_codigo);
        escopo_atual++;
    }

    bytes_locais = registrar_locais(no->filho1);
    gerar_lista(no->filho2);

    if (bytes_locais > 0)
        printf("addiu $sp, $sp, %d\n", bytes_locais);

    offset_local_atual = offset_anterior;

    if (!reutilizar_escopo) {
        desempilhar_escopo(&pilha_codigo);
        escopo_atual--;
    }
}

// Registra parâmetros formais utilizando offsets relativos ao frame pointer.
static void registrar_parametros(AST *params) {
    int pos = 8;

    for (AST *p = params; p; p = p->irmao) {
        Symbol *s;
        TipoSimbolo tipo = tipo_de_no(p->filho2);

        if (p->tipo == AST_PARAM_VETOR) {
            inserir_simbolo_vetor(&pilha_codigo, p->lexema, tipo, 0,
                                  p->linha, escopo_atual, 1);
        } else {
            inserir_simbolo(&pilha_codigo, p->lexema, tipo,
                            p->linha, escopo_atual, 1);
        }

        s = buscar_no_escopo_atual(&pilha_codigo, p->lexema);
        s->pos = pos;
        pos += 4;
    }
}

// Gera o prólogo, corpo e epílogo de uma função.
static void gerar_funcao(AST *func) {
    // Salva FP e RA e cria o novo frame.
    int label_retorno = novo_label();
    int label_retorno_anterior = label_retorno_atual;
    int offset_anterior = offset_local_atual;

    label_retorno_atual = label_retorno;
    offset_local_atual = 0;

    printf("_f_%s:\n", func->lexema);
    printf("addiu $sp, $sp, -8\n");
    printf("sw $fp, 0($sp)\n");
    printf("sw $ra, 4($sp)\n");
    printf("move $fp, $sp\n");

    empilhar_escopo(&pilha_codigo);
    escopo_atual = 1;
    registrar_parametros(func->filho2);
    gerar_bloco(func->filho3, 1);
    desempilhar_escopo(&pilha_codigo);

    printf("L%d:\n", label_retorno);
    printf("move $sp, $fp\n");
    printf("lw $fp, 0($sp)\n");
    printf("lw $ra, 4($sp)\n");
    printf("addiu $sp, $sp, 8\n");
    printf("jr $ra\n");

    // Restaura o frame anterior e retorna ao chamador.
    offset_local_atual = offset_anterior;
    label_retorno_atual = label_retorno_anterior;
}

// Gera código para todas as funções declaradas no programa.
static void gerar_funcoes(AST *funcoes_ast) {
    for (AST *f = funcoes_ast; f; f = f->irmao)
        gerar_funcao(f);
}

// Libera estruturas auxiliares utilizadas durante a geração.
static void liberar_strings(void) {
    while (strings) {
        StringLiteral *tmp = strings;
        strings = strings->prox;
        free(tmp->valor);
        free(tmp);
    }
}

// Libera a lista de informações sobre funções registradas.
static void liberar_funcoes(void) {
    while (funcoes) {
        FunctionInfo *tmp = funcoes;
        funcoes = funcoes->prox;
        free(tmp->nome);
        free(tmp);
    }
}

// Inicializa a geração de código MIPS a partir da AST completa.
void gerar_codigo(AST *raiz, ScopeStack *pilha) {
    (void) pilha;

    label_counter = 0;
    string_counter = 0;
    escopo_atual = 0;
    offset_local_atual = 0;
    label_retorno_atual = -1;

    coletar_strings(raiz);
    registrar_funcoes(raiz->filho2);

    iniciar_pilha(&pilha_codigo);
    empilhar_escopo(&pilha_codigo);

    printf(".data\n");
    printf("_newline: .asciiz \"\\n\"\n");
    for (StringLiteral *s = strings; s; s = s->prox)
        printf("%s: .asciiz %s\n", s->label, s->valor);

    printf(".align 2\n");
    emitir_globais(raiz->filho1);

    printf(".text\n");
    printf(".globl main\n");

    gerar_funcoes(raiz->filho2);

    printf("main:\n");
    printf("move $fp, $sp\n");

    empilhar_escopo(&pilha_codigo);
    escopo_atual = 1;
    offset_local_atual = 0;
    gerar_bloco(raiz->filho3, 1);
    desempilhar_escopo(&pilha_codigo);

    printf("li $v0, 10\n");
    printf("syscall\n");

    liberar_pilha(&pilha_codigo);
    liberar_strings();
    liberar_funcoes();
}