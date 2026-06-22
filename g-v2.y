%code requires {
    #include "ast.h"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "semantico.h"
#include "codigo.h"

extern int yylex();
extern int yylineno;
extern char *yytext;
extern FILE *yyin;

void yyerror(const char *s);

AST *raiz;
ScopeStack pilha;
%}

%union {
    AST *ast;
    TokenInfo tk;
}

%token PRINCIPAL INT CAR LEIA ESCREVA NOVALINHA SE ENTAO SENAO FIMSE ENQUANTO
%token OU E IGUAL DIFERENTE MAIORIGUAL MENORIGUAL
%token <tk> IDENTIFICADOR INTCONST CARCONST CADEIACARACTERES
%token FUNCAO RETORNE GLOBAL

%type <ast> Programa DeclPrograma Bloco VarSection ListaDeclVar Tipo 
%type <ast> ListaComando Comando Expr OrExpr AndExpr EqExpr DesigExpr AddExpr
%type <ast> MulExpr UnExpr PrimExpr
%type <ast> DeclVarGlobais
%type <ast> DeclFunc
%type <ast> ListaParametrosTail ListaParametros
%type <ast> ListaFuncoes
%type <ast> ListVar
%type <ast> LValueExpr
%type <ast> ListExpr

%%

Programa
    : DeclVarGlobais DeclFunc DeclPrograma 
      { $$ = criar_no(AST_PROGRAMA,NULL,yylineno, 0, $1,$2,$3); raiz = $$; }
    ;


DeclVarGlobais
    : GLOBAL VarSection
      {
        $$ = $2;
      }
    | %empty
      {
          $$ = NULL;
      }
    ;


VarSection
    : '[' ListaDeclVar ']'
      { $$ = criar_no(AST_VARSECTION, NULL, yylineno,0, $2, NULL, NULL); }
    ;


ListaDeclVar
    : ListVar ':' Tipo ';' ListaDeclVar
      {
          AST *decl = criar_no(AST_DECLVAR, NULL, yylineno,0, $1, $3, NULL);
          $$ = adicionar_irmao(decl, $5);
      }
    | ListVar ':' Tipo ';'
      {
          $$ = criar_no(AST_DECLVAR, NULL, yylineno, 0,$1, $3, NULL);
      }
    ;

ListVar
    : IDENTIFICADOR ',' ListVar
      {
        AST *id = criar_no(AST_IDENT,$1.lexema,$1.linha, 0,NULL,NULL,NULL);
        $$ = adicionar_irmao(id, $3);
      }
    | IDENTIFICADOR '[' INTCONST ']' ',' ListVar
      {
        AST *tam = criar_no(AST_INTCONST,$3.lexema,$3.linha, 0,NULL,NULL,NULL);
        AST *vet = criar_no(AST_DECL_VETOR,$1.lexema,$1.linha, 0, tam,NULL,NULL);
        $$ = adicionar_irmao(vet, $6);
      }
    | IDENTIFICADOR
      {
        $$ = criar_no(AST_IDENT,$1.lexema,$1.linha,0,NULL,NULL,NULL);
      }
    | IDENTIFICADOR '[' INTCONST ']' 
      {
        AST *tam = criar_no(AST_INTCONST,$3.lexema,$3.linha,0,NULL,NULL,NULL);
        $$ = criar_no(AST_DECL_VETOR,$1.lexema,$1.linha,0,tam,NULL,NULL);
      }
    ;


DeclFunc
    : FUNCAO '[' IDENTIFICADOR '(' ListaParametros ')' ':' Tipo Bloco ListaFuncoes']'
      {
          AST *func =criar_no(AST_FUNCAO,$3.lexema,$3.linha,0,$8,$5,$9);
          $$ = adicionar_irmao(func,$10);
      }
    | %empty
      {
          $$ = NULL;
      }
    ;

ListaFuncoes
    : IDENTIFICADOR '(' ListaParametros ')' ':' Tipo Bloco ListaFuncoes
      {
        AST *func = criar_no(AST_FUNCAO,$1.lexema,$1.linha,0,$6,$3,$7);
        $$ = adicionar_irmao(func, $8);
      }
    | %empty
        {
            $$ = NULL;
        }
    ;

ListaParametros
    : ListaParametrosTail
      {
        $$ = $1;
      }
    | %empty
        {
            $$ = NULL;
        }
    ;


ListaParametrosTail
    : IDENTIFICADOR ':' Tipo
      {
        AST *id = criar_no(AST_IDENT,$1.lexema,$1.linha,0,NULL,NULL,NULL);
        $$ = criar_no(AST_PARAM,$1.lexema,$1.linha,0,id,$3,NULL);
      }
    | IDENTIFICADOR '[' ']' ':' Tipo
      {
        AST *id = criar_no(AST_IDENT,$1.lexema,$1.linha,0,NULL,NULL,NULL);
        $$ = criar_no(AST_PARAM_VETOR,$1.lexema,$1.linha,0,id,$5,NULL);
      }
    | IDENTIFICADOR ':' Tipo ',' ListaParametrosTail
      {
        AST *id = criar_no(AST_IDENT, $1.lexema, $1.linha, 0,NULL, NULL, NULL);
        AST *param = criar_no(AST_PARAM, $1.lexema, $1.linha, 0,id, $3, NULL);
        $$ = adicionar_irmao(param, $5);
      }
    | IDENTIFICADOR '[' ']' ':' Tipo ',' ListaParametrosTail  /* ALTERNATIVA NOVA */
      {
        AST *id = criar_no(AST_IDENT, $1.lexema, $1.linha, 0,NULL, NULL, NULL);
        AST *param = criar_no(AST_PARAM_VETOR, $1.lexema, $1.linha, 0,id, $5, NULL);
        $$ = adicionar_irmao(param, $7);
      }
    ;


DeclPrograma : PRINCIPAL Bloco { $$ = $2; } ;

Bloco
    : '{' ListaComando '}'
      { $$ = criar_no(AST_BLOCO, NULL, yylineno,0, NULL, $2, NULL); }
    | VarSection '{' ListaComando '}'
      { $$ = criar_no(AST_BLOCO, NULL, yylineno, 0,$1, $3, NULL); }
    ;


ListaComando
    : Comando
      { $$ = $1; }
    | Comando ListaComando
      { $$ = adicionar_irmao($1, $2); }
    ;



Tipo
    : INT
      { $$ = criar_no(AST_TIPO, "int", yylineno, 0,NULL, NULL, NULL); }
    | CAR
      { $$ = criar_no(AST_TIPO, "car", yylineno, 0,NULL, NULL, NULL); }
    ;


Comando
    : ';'
      { $$ = criar_no(AST_COMANDO_VAZIO, NULL, yylineno,0, NULL, NULL, NULL); }
    | Expr ';'
      { $$ = $1; }
    | LEIA LValueExpr ';'
      {
          $$ = criar_no(AST_LEIA, NULL, yylineno, 0,$2, NULL, NULL);
      }
    | ESCREVA Expr ';'
      { $$ = criar_no(AST_ESCREVA, NULL, yylineno, 0,$2, NULL, NULL); }
    | ESCREVA CADEIACARACTERES ';'
      {
          AST *str = criar_no(AST_STRING, $2.lexema, $2.linha, 0,NULL, NULL, NULL);
          $$ = criar_no(AST_ESCREVA, NULL, $2.linha, 0,str, NULL, NULL);
      }
    | NOVALINHA ';'
      {
       $$ = criar_no(AST_NOVALINHA, NULL, yylineno, 0,NULL, NULL, NULL); 
       }
    | SE '(' Expr ')' ENTAO Comando FIMSE
      { 
        $$ = criar_no(AST_SE, NULL, yylineno, 0,$3, $6, NULL); 
      }
    | SE '(' Expr ')' ENTAO Comando SENAO Comando FIMSE
      { 
        $$ = criar_no(AST_SE, NULL, yylineno, 0,$3, $6, $8); 
      }
    | ENQUANTO '(' Expr ')' Comando
      { 
        $$ = criar_no(AST_ENQUANTO, NULL, yylineno,0, $3, $5, NULL); 
      }
    | RETORNE Expr ';'
      { 
        $$ = criar_no(AST_RETORNE, NULL, yylineno,0, $2, NULL, NULL); 
      }
    | Bloco
      { 
        $$ = $1; 
      }
    ;

Expr
    : OrExpr
      { 
        $$ = $1; 
      }
    | LValueExpr '=' Expr
      {
          $$ = criar_no(AST_ATRIB, "=", yylineno, 0,$1, $3, NULL);
      }
    ;

LValueExpr
  : IDENTIFICADOR '[' Expr ']'
    {
       $$ = criar_no(AST_ACESS_VETOR,$1.lexema,$1.linha,0,$3,NULL,NULL);
    }
  | IDENTIFICADOR 
    {
      $$ = criar_no(AST_IDENT,$1.lexema,$1.linha,0,NULL,NULL,NULL);
    }
  ;

OrExpr
    : OrExpr OU AndExpr
      { $$ = criar_no(AST_OP, "||", yylineno, 0,$1, $3, NULL); }
    | AndExpr
      { $$ = $1; }
    ;

AndExpr
    : AndExpr E EqExpr
      { $$ = criar_no(AST_OP, "&", yylineno, 0,$1, $3, NULL); }
    | EqExpr
      { $$ = $1; }
    ;

EqExpr
    : EqExpr IGUAL DesigExpr
      { $$ = criar_no(AST_OP, "==", yylineno,0, $1, $3, NULL); }
    | EqExpr DIFERENTE DesigExpr
      { $$ = criar_no(AST_OP, "!=", yylineno, 0,$1, $3, NULL); }
    | DesigExpr
      { $$ = $1; }
    ;

DesigExpr
    : DesigExpr '<' AddExpr
      { $$ = criar_no(AST_OP, "<", yylineno, 0,$1, $3, NULL); }
    | DesigExpr '>' AddExpr
      { $$ = criar_no(AST_OP, ">", yylineno, 0,$1, $3, NULL); }
    | DesigExpr MAIORIGUAL AddExpr
      { $$ = criar_no(AST_OP, ">=", yylineno, 0,$1, $3, NULL); }
    | DesigExpr MENORIGUAL AddExpr
      { $$ = criar_no(AST_OP, "<=", yylineno, 0,$1, $3, NULL); }
    | AddExpr
      { $$ = $1; }
    ;

AddExpr
    : AddExpr '+' MulExpr
      { $$ = criar_no(AST_OP, "+", yylineno, 0,$1, $3, NULL); }
    | AddExpr '-' MulExpr
      { $$ = criar_no(AST_OP, "-", yylineno, 0,$1, $3, NULL); }
    | MulExpr
      { $$ = $1; }
    ;

MulExpr
    : MulExpr '*' UnExpr
      { $$ = criar_no(AST_OP, "*", yylineno, 0,$1, $3, NULL); }
    | MulExpr '/' UnExpr
      { $$ = criar_no(AST_OP, "/", yylineno, 0,$1, $3, NULL); }
    | UnExpr
      { $$ = $1; }
    ;

UnExpr
    : '-' PrimExpr
      { $$ = criar_no(AST_OP, "unary-", yylineno, 0,$2, NULL, NULL); }
    | '!' PrimExpr
      { $$ = criar_no(AST_OP, "!", yylineno, 0,$2, NULL, NULL); }
    | PrimExpr
      { $$ = $1; }
    ;

PrimExpr
    : IDENTIFICADOR '(' ListExpr ')'
      { $$ = criar_no(AST_CHAMADA_FUNC, $1.lexema, $1.linha, 0,$3, NULL, NULL); }
    | IDENTIFICADOR '(' ')'
      { $$ = criar_no(AST_CHAMADA_FUNC, $1.lexema, $1.linha, 0,NULL, NULL, NULL); }
    | IDENTIFICADOR '[' Expr ']'                   
      { $$ = criar_no(AST_ACESS_VETOR, $1.lexema, $1.linha, 0,$3, NULL, NULL); }
    | IDENTIFICADOR 
      { $$ = criar_no(AST_IDENT, $1.lexema, $1.linha, 0,NULL, NULL, NULL); }
    | CARCONST
      { $$ = criar_no(AST_CARCONST, $1.lexema, $1.linha, 0,NULL, NULL, NULL); }
    | INTCONST
      { $$ = criar_no(AST_INTCONST, $1.lexema, $1.linha, 0,NULL, NULL, NULL); }
    | '(' Expr ')'
      { $$ = $2; }
    ;

ListExpr 
    : Expr
      {
        $$ = $1;
      }
    | ListExpr ',' Expr
      {
        $$ = adicionar_irmao($1, $3);
      }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "ERRO: %s na linha %d\n", s, yylineno-1);
    exit(1);
}

int main(int argc, char **argv) {
  if (argc > 1) {
      yyin = fopen(argv[1], "r");
      if (!yyin) {
          perror(argv[1]);
          return 1;
      }
  }

  iniciar_pilha(&pilha);

  if (yyparse() == 0) {
    analisar_semantico(raiz, &pilha);
    gerar_codigo(raiz, &pilha);
  }

  if (raiz) {      
      imprimir_ast(raiz, 0);
      liberar_ast(raiz);
  }

  liberar_pilha(&pilha); 
  
  if (yyin) fclose(yyin);
  return 0;
}
