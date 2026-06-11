#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "codigo.h"

// Contadores para geração de registradores ($tx) e rótulos (Lx)
static int temp_counter = 0;
static int label_counter = 0;

typedef struct VarDeclarada {
    char *nome;
    struct VarDeclarada *prox;
} VarDeclarada;

static VarDeclarada *vars_declaradas = NULL;

static int var_ja_declarada(const char *nome) {
    for (VarDeclarada *v = vars_declaradas; v != NULL; v = v->prox) {
        if (strcmp(v->nome, nome) == 0) {
            return 1;
        }
    }
    return 0;
}

static void registrar_var(const char *nome) {
    VarDeclarada *nova = malloc(sizeof(VarDeclarada));
    nova->nome = strdup(nome);
    nova->prox = vars_declaradas;
    vars_declaradas = nova;
}

static int novo_label() {
    return label_counter++;
}

static int novo_temp() {
    return temp_counter++;
}

static int string_counter = 0;

typedef struct StringLiteral {
    char label[32];
    char *valor;
    struct StringLiteral *prox;
} StringLiteral;

static StringLiteral *strings = NULL;

static const char *registrar_string(const char *valor) {
    StringLiteral *novo = malloc(sizeof(StringLiteral));

    snprintf(novo->label, sizeof(novo->label), "str%d", string_counter++);
    novo->valor = strdup(valor);
    novo->prox = strings;
    strings = novo;

    return novo->label;
}

static const char *buscar_string(const char *valor) {
    for (StringLiteral *s = strings; s != NULL; s = s->prox) {
        if (strcmp(s->valor, valor) == 0) {
            return s->label;
        }
    }

    return registrar_string(valor);
}


// Função principal que percorre a AST e emite código em MIPS
static int gerar(AST *no) {
    if (!no){
        return -1; 
    } 

    switch (no->tipo) {
        // Inicia a geração a partir do bloco principal
        case AST_PROGRAMA: {
            gerar(no->filho1);
            gerar(no->filho2);
            gerar(no->filho3);
            break;
        }

        // Representa um bloco lexical (variáveis + comandos)
        case AST_BLOCO: {
            gerar(no->filho1);
            gerar(no->filho2);
            break;
        }


        case AST_VARSECTION: {
            break; 
        }

        case AST_DECLVAR: {
            break; 
        }
            
    
        // Carrega constante imediata no registrador (li)
        case AST_INTCONST: {
            int t = novo_temp();
            printf("li $t%d, %s\n", t, no->lexema);
            return t;
        }

        // Carrega variável da memória (lw)
        case AST_IDENT: {
            int t = novo_temp();
            printf("lw $t%d, %s\n", t, no->lexema);
            return t;
        }

        // Avalia a expressão do lado direito e armazena a variável no lado esquerdo (sw)
        case AST_ATRIB: {
            int t = gerar(no->filho2);
            printf("sw $t%d, %s\n", t, no->filho1->lexema);
            break;
        }

        // Avalia expressões aritméticas e relacionais
        // (garante que operandos estejam prontos antes da operação)
        case AST_OP: {
            int t1 = gerar(no->filho1);
            int t2 = gerar(no->filho2);

            int t = novo_temp();

            // Operadores aritméticos
            if (strcmp(no->lexema, "unary-") == 0) {
                int t1 = gerar(no->filho1);
                int t = novo_temp();

                printf("sub $t%d, $zero, $t%d\n", t, t1);
                return t;
            }
            else if (strcmp(no->lexema, "+") == 0)
                printf("add $t%d, $t%d, $t%d\n", t, t1, t2);
            else if (strcmp(no->lexema, "-") == 0)
                printf("sub $t%d, $t%d, $t%d\n", t, t1, t2);
            else if (strcmp(no->lexema, "*") == 0)
                printf("mul $t%d, $t%d, $t%d\n", t, t1, t2);
            else if (strcmp(no->lexema, "/") == 0)
                printf("div $t%d, $t%d, $t%d\n", t, t1, t2);
            
            // Operadores relacionais
            else if (strcmp(no->lexema, "==") == 0)
                printf("seq $t%d, $t%d, $t%d\n", t, t1, t2);
            else if (strcmp(no->lexema, "!=") == 0)
                printf("sne $t%d, $t%d, $t%d\n", t, t1, t2);
            else if (strcmp(no->lexema, "<") == 0)
                printf("slt $t%d, $t%d, $t%d\n", t, t1, t2);
            else if (strcmp(no->lexema, ">") == 0)
                printf("sgt $t%d, $t%d, $t%d\n", t, t1, t2);
            else if (strcmp(no->lexema, "<=") == 0)
                printf("sle $t%d, $t%d, $t%d\n", t, t1, t2);
            else if (strcmp(no->lexema, ">=") == 0)
                printf("sge $t%d, $t%d, $t%d\n", t, t1, t2);

            return t;
        }

        case AST_NOVALINHA: {
            printf("la $a0, newline\n");
            printf("li $v0, 4\n");
            printf("syscall\n");
            break;
        }

        case AST_CARCONST: {
            int t = novo_temp();
            printf("li $t%d, %d\n", t, no->lexema[1]);
            return t;
        }

        case AST_LEIA: {
            printf("li $v0, 5\n");
            printf("syscall\n");
            printf("sw $v0, %s\n", no->filho1->lexema);
            break;
        }
        
        // Chamada de sistema MIPS para imprimir inteiro
        case AST_ESCREVA: {
            if (no->filho1 && no->filho1->tipo == AST_STRING) {
                const char *lbl = buscar_string(no->filho1->lexema);

                printf("la $a0, %s\n", lbl);
                printf("li $v0, 4\n");
                printf("syscall\n");
            } else {
                int t = gerar(no->filho1);

                printf("move $a0, $t%d\n", t);

                if (no->filho1 && no->filho1->tipo == AST_IDENT &&
                    strcmp(no->filho1->lexema, "conceito") == 0) {
                    printf("li $v0, 11\n");
                } else {
                    printf("li $v0, 1\n");
                }

                printf("syscall\n");
            }

            break;
        }

        // Constrói a estrutura de "SE" em saltos condicionais
        case AST_SE: {
            int cond = gerar(no->filho1);

            if (no->filho3) {
                // IF com ELSE
                int lbl_else = novo_label();
                int lbl_end  = novo_label();

                // "if (!cond) goto ELSE"
                printf("beq $t%d, $zero, L%d\n", cond, lbl_else);

                // THEN
                gerar(no->filho2);
                printf("j L%d\n", lbl_end);

                // ELSE
                printf("L%d:\n", lbl_else);
                gerar(no->filho3);

                // FIM
                printf("L%d:\n", lbl_end);

            } else {
                // IF sem ELSE
                int lbl_end = novo_label();

                printf("beq $t%d, $zero, L%d\n", cond, lbl_end);

                gerar(no->filho2);

                printf("L%d:\n", lbl_end);
            }

            break;
        }

        // Constrói a estrutura de "ENQUANTO" em saltos condicionais
        case AST_ENQUANTO: {
            int lbl_inicio = novo_label();
            int lbl_fim    = novo_label();

            printf("L%d:\n", lbl_inicio);

            int cond = gerar(no->filho1);

            // Se condição == 0 → sai do loop
            printf("beq $t%d, $zero, L%d\n", cond, lbl_fim);

            // Corpo
            gerar(no->filho2);

            // Volta pro início
            printf("j L%d\n", lbl_inicio);

            printf("L%d:\n", lbl_fim);

            break;
        }
    }
    
    // Só percorre irmãos para certos tipos
    if (no->irmao) {
        gerar(no->irmao);
    }

    return -1;
}

// Gera a seção .data para variáveis globais
static void gerar_data(AST *no) {
    if (!no) return;

    if (no->tipo == AST_DECLVAR) {
        for (AST *id = no->filho1; id != NULL; id = id->irmao) {
            if (!var_ja_declarada(id->lexema)) {
                printf("%s: .word 0\n", id->lexema);
                registrar_var(id->lexema);
            }
        }
    }

    gerar_data(no->filho1);
    gerar_data(no->filho2);
    gerar_data(no->filho3);
    gerar_data(no->irmao);
}

static void coletar_strings(AST *no) {
    if (!no) return;

    if (no->tipo == AST_ESCREVA &&
        no->filho1 &&
        no->filho1->tipo == AST_STRING) {
        buscar_string(no->filho1->lexema);
    }

    coletar_strings(no->filho1);
    coletar_strings(no->filho2);
    coletar_strings(no->filho3);
    coletar_strings(no->irmao);
}

static void imprimir_strings_data() {
    for (StringLiteral *s = strings; s != NULL; s = s->prox) {
        printf("%s: .asciiz %s\n", s->label, s->valor);
    }
}

// Função principal para estruturar o programa em MIPS
void gerar_codigo(AST *raiz) {
    coletar_strings(raiz);

    printf(".data\n");
    gerar_data(raiz);
    imprimir_strings_data();
    printf("newline: .asciiz \"\\n\"\n");

    printf(".text\n.globl main\nmain:\n");

    gerar(raiz);

    printf("li $v0, 10\nsyscall\n");
}
