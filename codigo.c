#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codigo.h"
#include "ast.h"
#include "symtab.h"

// Contador de rótulos (L0, L1, L2...)
static int label_counter = 0;

// Contador de strings literais (str0, str1...)
static int string_counter = 0;

// Referência global à tabela de símbolos
// (passada por gerar_codigo e usada em toda geração)
static ScopeStack *tabela = NULL;

// Posição do próximo slot global na pilha de $s1
// Começa em 1: a primeira variável global fica em 0($s1)
static int pos_global_atual = 1;

// Posição do próximo slot local dentro da função atual
// Relativa a $fp: variável 1 fica em -4($fp), variável 2 em -8($fp)
static int pos_local_atual = 1;

static int pos_parametro_atual = 1; // Posição do próximo parâmetro na função atual (relativa a $fp)

static int label_saida_atual = -1;

static void debug_pilha(ScopeStack *pilha, const char *buscando) {
    Scope *escopo = pilha->topo;
    int nivel = 0;
    while (escopo) {
        Symbol *s = escopo->simbolos;
        while (s) {
            if (strcmp(s->nome, buscando) == 0)
                fprintf(stderr, "Achei '%s' no nivel %d, escopo=%d, parametro=%d, pos=%d\n",
                        buscando, nivel, s->escopo, s->parametro, s->pos); 
            s = s->prox;
        }
        escopo = escopo->prox;
        nivel++;
    }
}

typedef struct StringLiteral {
    char label[32];   // nome do rótulo no .data (str0, str1...)
    char *valor;      // conteúdo da string (com aspas)
    struct StringLiteral *prox;
} StringLiteral;

static StringLiteral *strings = NULL;

static int novo_label() {
    return label_counter++;
}

//Registra a string e dá um nome str0, str1,...
static const char *registrar_string(const char *valor) {
    StringLiteral *novo = malloc(sizeof(StringLiteral));
    snprintf(novo->label, sizeof(novo->label), "str%d", string_counter++);
    novo->valor = strdup(valor);
    novo->prox = strings;
    strings = novo;
    return novo->label;
}

//Procura a string na pilha. Se não tiver, registra
static const char *buscar_string(const char *valor) {
    for (StringLiteral *s = strings; s != NULL; s = s->prox)
        if (strcmp(s->valor, valor) == 0) return s->label;
    return registrar_string(valor);
}


//Percorre toda a árvore antes para buscar todas as strings (necessário pq o .data vem antes do .text na saída)
static void coletar_strings(AST *no) {
    if (!no) return;
    if (no->tipo == AST_ESCREVA &&
        no->filho1 &&
        no->filho1->tipo == AST_STRING)
        buscar_string(no->filho1->lexema);
    coletar_strings(no->filho1);
    coletar_strings(no->filho2);
    coletar_strings(no->filho3);
    coletar_strings(no->irmao);
}

//Verifica se uma variável é global
static int eh_variavel_global(const char *nome) {
    Symbol *s = buscar_simbolo(tabela, nome);
    return s && s->escopo == 0;
}

//Empilha $s0
static void emit_push() {
    printf("sw $s0, 0($sp)\n"); //Salva o valor atual do acumulador na posição apontada por $sp(topo da pilha)
    printf("addiu $sp, $sp, -4\n"); //Move $sp 4 bytes pra baixo
}

//Desempilha para $t1
static void emit_pop_t(int temp) {
    printf("lw $t%d, 4($sp)\n", temp);
    printf("addiu $sp, $sp, 4\n");
}

//carrega variável no acumulador $s0
static void emit_load_var(const char *nome) {
    Symbol *s = buscar_simbolo(tabela, nome);
    if (!s) { fprintf(stderr, "Variavel nao encontrada: %s\n", nome); exit(1); }

    if (s->escopo == 0) {
        int desloc = -(s->pos - 1) * 4;
        printf("lw $s0, %d($s1)\n", desloc);
    } else if (s->parametro) {          // PARÂMETRO: deslocamento positivo
        int desloc = s->pos * 4;       // 4($fp), 8($fp), ...
        printf("lw $s0, %d($fp)\n", desloc);
    } else {                           // LOCAL: deslocamento negativo
        int desloc = -(s->pos) * 4;    // -4($fp), -8($fp), ...
        printf("lw $s0, %d($fp)\n", desloc);
    }
}

//armazena $s0 numa variável 
static void emit_store_var(const char *nome) {
    Symbol *s = buscar_simbolo(tabela, nome);
    if (s->escopo == 0) {
        int desloc = -(s->pos - 1) * 4;
        printf("sw $s0, %d($s1)\n", desloc); // sw $s0, desloc($s1) — armazena o acumulador no endereço $s1 + desloc
    } else if (s->parametro) {          // PARÂMETRO: deslocamento positivo
        int desloc = s->pos * 4;       // 4($fp)
        printf("sw $s0, %d($fp)\n", desloc); // sw $s0, desloc($fp) — armazena o acumulador no endereço $fp + desloc    
    }else{
        int desloc = -(s->pos) * 4;
        printf("sw $s0, %d($fp)\n", desloc); // sw $s0, desloc($fp) — armazena o acumulador no endereço $fp + desloc
    }
}


static void emit_load_var_indexed(const char *nome) {
    if (!nome) {
        fprintf(stderr, "ERRO: nome é NULL em emit_load_var_indexed\n");
        return;
    }
    
    if (!tabela) {
        fprintf(stderr, "ERRO: tabela é NULL em emit_load_var_indexed\n");
        return;
    }
    
    Symbol *s = buscar_simbolo(tabela, nome);
    if (!s) {
        fprintf(stderr, "ERRO: Variavel '%s' nao encontrada na tabela\n", nome);
        return;
    }
    
    // Neste ponto $s0 contém o índice (resultado da expressão de índice)
    // e $t1 contém a posição base do vetor (já calculada por emit_pop_t se necessário)
    // Calculamos: endereço = base + índice * 4

    int base_desloc;
    const char *base_reg;
    if (s->escopo == 0) {
        base_desloc = -(s->pos - 1) * 4;
        base_reg = "$s1";
    } else {
        base_desloc = -(s->pos) * 4;
        base_reg = "$fp";
    }

    printf("sll $s0, $s0, 2\n"); //desloca $s0 2 bits para a esquerda (equivalente a multiplicar por 4)

    printf("addiu $t1, %s, %d\n", base_reg, base_desloc); //addiu $t1, $s1/$fp, desloc — calcula o endereço base do vetor em $t1

    printf("add $t1, $t1, $s0\n"); // add $t1, $t1, $s0 — soma o offset do índice ao endereço base

    printf("lw $s0, 0($t1)\n"); // lw $s0, 0($t1) — carrega o elemento do vetor no acumulador
}

static void gerar(AST *no) {
    if (!no) return;

    switch (no->tipo) {
        case AST_PROGRAMA: {
            gerar(no->filho1); // DeclVarGlobais — registra posições, não emite código
            // filho2 (DeclFunc) é emitido ANTES do main em gerar_codigo()
            gerar(no->filho3); // DeclPrograma = bloco principal
            break;
        }

        case AST_VARSECTION: {
            gerar(no->filho1); // desce para AST_DECLVAR
            break;
        }

        case AST_DECLVAR: {
            // Descobre o tipo (filho2 = AST_TIPO)
            // Percorre a lista de identificadores (filho1)
            for (AST *id = no->filho1; id != NULL; id = id->irmao) {
                Symbol *s = buscar_simbolo(tabela, id->lexema);
                if (!s) break;

                if (s->escopo == 0) {
                    // Global: registra posição e desloca $sp (via addiu no prólogo)
                    // A posição já deve ter sido atribuída pelo semântico ou aqui
                    s->pos = pos_global_atual;
                    if (id->tipo == AST_DECL_VETOR) {
                        int tam = atoi(id->filho1->lexema);
                        pos_global_atual += tam; // vetor de N ocupa N posições
                    } else {
                        pos_global_atual += 1;
                    }
                } else {
                    // Local: registra posição relativa a $fp
                    s->pos = pos_local_atual;
                    if (id->tipo == AST_DECL_VETOR) {
                        int tam = atoi(id->filho1->lexema);
                        pos_local_atual += tam;
                        // Reserva espaço na pilha para o vetor
                        printf("addiu $sp, $sp, -%d\n", tam * 4);
                    } else {
                        pos_local_atual += 1;
                        printf("addiu $sp, $sp, -4\n");
                        // addiu $sp, $sp, -4 — reserva 4 bytes na pilha para esta variável local
                    }
                }
            }
            // Percorre irmãos (próxima DECLVAR)
            gerar(no->irmao);
            break;
        }

        case AST_BLOCO: {
            // filho1 = VARSECTION (opcional), filho2 = lista de comandos
            gerar(no->filho1); // declara variáveis locais e reserva espaço
            gerar(no->filho2); // gera os comandos
            break;
        }

        case AST_INTCONST: {
            printf("li $s0, %s\n", no->lexema); // li $s0, N — carrega a constante inteira N diretamente no acumulador
            break;
        }

        case AST_CARCONST: {
            // lexema é algo como 'a' — o valor ASCII está em lexema[1] (lexema[0] é as aspas de abertura ')
            printf("li $s0, %d\n", (int)no->lexema[1]); //li $s0, 97 — carrega o valor ASCII do caractere no acumulador
            break;
        }

        case AST_IDENT: {
            //debug_pilha(tabela, no->lexema);
            emit_load_var(no->lexema);
            // Carrega a variável no acumulador $s0
            // Globais: lw $s0, -(pos-1)*4($s1)
            // Locais:  lw $s0, -pos*4($fp)
            break;
        }

        case AST_ACESS_VETOR: {
            // Avalia o índice — resultado fica em $s0
            gerar(no->filho1);
            // Agora $s0 = índice; carregamos o elemento
            emit_load_var_indexed(no->lexema);
            break;
        }

        case AST_ATRIB: {
            AST *lval = no->filho1;

            if (lval->tipo == AST_ACESS_VETOR) {
                // valor a armazenar E índice
                // Avalia o lado direito primeiro -> resultado em $s0
                gerar(no->filho2);
                // Salva o valor na pilha
                emit_push();

                // Avalia o índice -> resultado em $s0
                gerar(lval->filho1);
                // $s0 = índice

                // Calcula endereço do vetor
                Symbol *s = buscar_simbolo(tabela, lval->lexema);
                int base_desloc;
                const char *base_reg;

                if (s->escopo == 0){ //Se for variável global
                    base_desloc = -(s->pos - 1) * 4;
                    base_reg = "$s1";
                }else{ //Se for variável local
                    base_desloc = -(s->pos) * 4;
                    base_reg = "$fp";
                }
                

                printf("sll $s0, $s0, 2\n");
                // sll $s0, $s0, 2 — índice * 4

                printf("addiu $t1, %s, %d\n", base_reg, base_desloc);
                // addiu $t1, base, desloc — endereço base do vetor

                printf("add $t1, $t1, $s0\n");
                // add $t1, $t1, $s0 — endereço do elemento

                // Recupera o valor a armazenar
                emit_pop_t(2);

                printf("sw $t2, 0($t1)\n");
                // sw $s0, 0($t2) — armazena o valor no elemento do vetor
            } else {
                // Atribuição escalar: avalia lado direito → $s0
                gerar(no->filho2);
                emit_store_var(lval->lexema);
                // sw $s0, desloc(base) — armazena o acumulador na variável
            }
            gerar(no->irmao);
            break;
        }

        case AST_OP: {
            if (strcmp(no->lexema, "unary-") == 0 || strcmp(no->lexema, "!") == 0) {
                // Operadores unários — só filho1
                gerar(no->filho1);
                // $s0 = valor do operando
                if (strcmp(no->lexema, "unary-") == 0)
                    printf("sub $s0, $zero, $s0\n");
                    // sub $s0, $zero, $s0 -> $s0 = 0 - $s0 = negativo
                else
                    printf("seq $s0, $s0, $zero\n");
                    // seq $s0, $s0, $zero -> $s0 = ($s0 == 0) ? 1 : 0 — NOT lógico
                break;
            }

            // Operadores binários
            // 1. Avalia e1 -> $s0
            gerar(no->filho1);
            // 2. Empilha e1
            emit_push();
            // sw $s0, 0($sp)   — salva e1 na pilha
            // addiu $sp, $sp, -4

            // 3. Avalia e2 -> $s0
            gerar(no->filho2);
            // 4. Desempilha e1 para $t1
            emit_pop_t(1);
            // lw $t1, 4($sp)   — recupera e1 do topo
            // addiu $sp, $sp, 4

            // 5. Aplica operação: $s0 = $s0 op $t1
            // (ordem: $t1 é e1, $s0 é e2)
            if      (!strcmp(no->lexema, "+"))  printf("add $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, "-"))  printf("sub $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, "*"))  printf("mul $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, "/"))  printf("div $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, "==")) printf("seq $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, "!=")) printf("sne $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, "<"))  printf("slt $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, ">"))  printf("sgt $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, "<=")) printf("sle $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, ">=")) printf("sge $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, "||")) printf("or  $s0, $t1, $s0\n");
            else if (!strcmp(no->lexema, "&&")) printf("and $s0, $t1, $s0\n");
            // Resultado final fica em $s0 (acumulador)
            break;
        }

        case AST_SE: {
            // avalia e1, empilha, avalia e2, desempilha,
            // compara e1 com e2
            gerar(no->filho1);
            // $s0 = resultado da condição (0 = falso, != 0 = verdadeiro)

            if (no->filho3) {
                int lbl_else = novo_label();
                int lbl_fim  = novo_label();

                printf("beq $s0, $zero, L%d\n", lbl_else);
                // beq $s0, $zero, Lelse — se condição == 0 (falso), pula para else

                gerar(no->filho2); // bloco ENTÃO
                printf("b L%d\n", lbl_fim);
                // b Lfim — pula o bloco else

                printf("L%d:\n", lbl_else);
                gerar(no->filho3); // bloco SENÃO
                printf("L%d:\n", lbl_fim);
            } else {
                int lbl_fim = novo_label();
                printf("beq $s0, $zero, L%d\n", lbl_fim);
                // beq $s0, $zero, Lfim — se falso, pula o bloco
                gerar(no->filho2);
                printf("L%d:\n", lbl_fim);
            }
            break;
        }

        case AST_ENQUANTO: {
            int lbl_inicio = novo_label();
            int lbl_fim    = novo_label();

            printf("L%d:\n", lbl_inicio);
            // Rótulo de início do while

            gerar(no->filho1);
            // Avalia condição → $s0

            printf("beq $s0, $zero, L%d\n", lbl_fim);
            // beq $s0, $zero, Lfim — se falso, sai do loop

            gerar(no->filho2); // corpo

            printf("b L%d\n", lbl_inicio);
            // b Linicio — volta para reavaliar a condição

            printf("L%d:\n", lbl_fim);
            break;
        }

        case AST_LEIA: {
            AST *lval = no->filho1;
            printf("li $v0, 5\n");
            // li $v0, 5 — código de syscall para leitura de inteiro
            printf("syscall\n");
            // syscall — executa a leitura; resultado fica em $v0

            printf("move $s0, $v0\n");
            // move $s0, $v0 — copia o valor lido para o acumulador

            if (lval->tipo == AST_ACESS_VETOR) {
                // Salva o valor lido
                emit_push();
                // Avalia o índice
                gerar(lval->filho1);
                // Calcula endereço e armazena (lógica similar ao AST_ATRIB com vetor)
            } else {
                emit_store_var(lval->lexema);
                // sw $s0, desloc(base) — armazena o valor lido na variável
            }
            break;
        }

        case AST_ESCREVA: {
            if (no->filho1->tipo == AST_STRING) {
                const char *lbl = buscar_string(no->filho1->lexema);
                printf("la $a0, %s\n", lbl);
                // la $a0, str0 — carrega endereço da string em $a0
                printf("li $v0, 4\n");
                // li $v0, 4 — código de syscall para impressão de string
                printf("syscall\n");
            } else {
                gerar(no->filho1);
                // Avalia expressão → $s0
                printf("move $a0, $s0\n");
                // move $a0, $s0 — copia acumulador para $a0 (argumento do syscall)
                printf("li $v0, 1\n");
                // li $v0, 1 — código de syscall para impressão de inteiro
                printf("syscall\n");
            }
            gerar(no->irmao);
            break;
        }

        case AST_NOVALINHA: {
            printf("la $a0, newline\n");
            // la $a0, newline — carrega endereço da string "\n"
            printf("li $v0, 4\n");
            printf("syscall\n");
            gerar(no->irmao);
            break;
        }

        case AST_RETORNE: {
            gerar(no->filho1);
            // Avalia expressão de retorno → $s0
            // $s0 JÁ É o acumulador = valor de retorno, conforme o professor
            // ("o valor retornado por uma função sempre fica no acumulador $s0")

            // Salta para o epílogo da função atual
            printf("b L%d\n", label_saida_atual);
            // b Lsaida — pula para o código de restauração de contexto
            break;
        }

        case AST_FUNCAO: {
            int lbl_saida = novo_label();
            int lbl_anterior = label_saida_atual;
            label_saida_atual = lbl_saida;
            pos_local_atual = 1;  // reseta locais

            printf("%s:\n", no->lexema);

            printf("sw $fp, 0($sp)\n");
            printf("addiu $sp, $sp, -4\n");

            printf("move $fp, $sp\n");

            printf("sw $ra, 0($sp)\n");
            printf("addiu $sp, $sp, -4\n");

            // Conta e atribui posições aos parâmetros ANTES de gerar o corpo
            int n_params = 0;
            for (AST *p = no->filho2; p != NULL; p = p->irmao) n_params++;

            int pos_param = n_params;          // último param → 4($fp), primeiro → n*4($fp)
            for (AST *p = no->filho2; p != NULL; p = p->irmao) {
                Symbol *s = buscar_simbolo(tabela, p->lexema);
                if (s) {
                    s->pos = pos_param;
                    s->parametro = 1;
                    pos_param--;
                }
            }

            gerar(no->filho3); // corpo

            printf("L%d:\n", lbl_saida);

            int z = (n_params + 1) * 4;
            printf("lw $ra, 0($fp)\n");
            printf("move $sp, $fp\n");
            printf("addiu $sp, $sp, %d\n", z);
            printf("lw $fp, 0($sp)\n");
            printf("jr $ra\n");

            label_saida_atual = lbl_anterior;
            desempilhar_escopo(tabela);

            break;
        }
        case AST_CHAMADA_FUNC: {
            // Sequência de chamada do código chamador (professor):
            // 1. Empilha $fp atual
            printf("sw $fp, 0($sp)\n");
            // sw $fp, 0($sp) — salva o frame pointer do chamador
            printf("addiu $sp, $sp, -4\n");
            // addiu $sp, $sp, -4

            // 2. Avalia argumentos em ordem INVERSA e empilha
            // Conta argumentos primeiro
            int n_args = 0;
            for (AST *a = no->filho1; a != NULL; a = a->irmao) n_args++;

            // Coleta argumentos em array para poder inverter
            AST *args[64];
            int i = 0;
            for (AST *a = no->filho1; a != NULL; a = a->irmao) args[i++] = a;

            // Empilha da direita para a esquerda (ordem inversa)
            for (int j = n_args - 1; j >= 0; j--) {
                gerar(args[j]);
                // avalia argumento j → $s0
                emit_push();
                // sw $s0, 0($sp)   — empilha o valor do argumento
                // addiu $sp, $sp, -4
            }

            // 3. Chama a função
            printf("jal %s\n", no->lexema);
            // jal nome — salta para a função e salva endereço de retorno em $ra
            // Após o retorno, $s0 contém o valor retornado

            break;
        }

    }
}

static void gerar_funcoes(AST *no) {
    for (AST *f = no; f != NULL; f = f->irmao) {
        gerar(f); // emite o código de cada função antes do main
    }
}

static void registrar_globais(AST *no) {
    // Percorre a AST de declarações globais atribuindo posições
    // sem emitir código (globais ficam em $s1, não precisam de addiu $sp)
    if (!no) return;
    if (no->tipo == AST_DECLVAR) {
        for (AST *id = no->filho1; id != NULL; id = id->irmao) {
            Symbol *s = buscar_simbolo(tabela, id->lexema);
            if (s) {
                s->pos = pos_global_atual;
                if (id->tipo == AST_DECL_VETOR)
                    pos_global_atual += atoi(id->filho1->lexema);
                else
                    pos_global_atual += 1;
            }
        }
    }
    registrar_globais(no->filho1);
    registrar_globais(no->irmao);
}

void gerar_codigo(AST *raiz, ScopeStack *pilha) {
    tabela = pilha;
    // Guarda referência global à tabela para ser usada em emit_load_var etc.
    // Coleta strings para o .data 
    coletar_strings(raiz);

    // Atribui posições às variáveis globais
    registrar_globais(raiz->filho1);
    // Percorre DeclVarGlobais e preenche s->pos para cada símbolo global

    // Seção .data
    printf(".data\n");
    printf("newline: .asciiz \"\\n\"\n");
    for (StringLiteral *s = strings; s != NULL; s = s->prox)
        printf("%s: .asciiz %s\n", s->label, s->valor);
    // Strings literais do programa

    // Seção .text 
    printf(".text\n.globl main\n");

    // Emite funções ANTES do main
    if (raiz->filho2)
        gerar_funcoes(raiz->filho2);

    // Emite main 
    printf("main:\n");

    // Inicializa $s1 com o valor atual de $sp
    // $s1 será o ponteiro fixo para o início das variáveis globais
    printf("move $s1, $sp\n");
    // move $s1, $sp — copia $sp para $s1; $s1 não muda mais

    // Reserva espaço para todas as variáveis globais de uma vez
    // N = (pos_global_atual - 1) variáveis * 4 bytes
    int n_globais = pos_global_atual - 1;
    if (n_globais > 0) {
        printf("addiu $sp, $sp, -%d\n", n_globais * 4);
        // addiu $sp, $sp, -N — empurra $sp para baixo, criando espaço
        // para todas as variáveis globais
        // A primeira global fica em 0($s1) = $s1, a segunda em -4($s1), etc.
    }

    // Gera o bloco principal
    gerar(raiz->filho3);
    // raiz->filho3 = DeclPrograma = bloco do principal {}

    // Encerramento do programa
    printf("li $v0, 10\n");
    // li $v0, 10 — código de syscall para encerrar o programa
    printf("syscall\n");
}