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
%}

%union {
    AST *ast;
    TokenInfo tk;
}

%token PRINCIPAL INT CAR LEIA ESCREVA NOVALINHA SE ENTAO SENAO FIMSE ENQUANTO
%token OU E IGUAL DIFERENTE MAIORIGUAL MENORIGUAL
%token <tk> IDENTIFICADOR INTCONST CARCONST CADEIACARACTERES
%token FUNCAO RETORNE GLOBAL

%type <ast> Programa DeclPrograma Bloco VarSection ListaDeclVar DeclVar Tipo DeclVetor
%type <ast> ListaComando Comando Expr OrExpr AndExpr EqExpr DesigExpr AddExpr
%type <ast> MulExpr UnExpr PrimExpr
%type <ast> DeclVarGlobais
%type <ast> DeclFunc
%type <ast> ListaParametros
%type <ast> RestoListaParam
%type <ast> ListaArgumentos

%%

Programa
    : DeclVarGlobais DeclFunc DeclPrograma 
      { $$ = criar_no(AST_PROGRAMA,NULL,yylineno,$1,$2,$3); raiz = $$; }
    ;


DeclVarGlobais
    : '[' ListaDeclVar ']'
      {
          $$ = criar_no(AST_VARSECTION,NULL,yylineno,$2,NULL,NULL);
      }
    | %empty
      {
          $$ = NULL;
      }
    ;


DeclPrograma : PRINCIPAL Bloco { $$ = $2; } ;

Bloco
    : '{' ListaComando '}'
      { $$ = criar_no(AST_BLOCO, NULL, yylineno, NULL, $2, NULL); }
    | VarSection '{' ListaComando '}'
      { $$ = criar_no(AST_BLOCO, NULL, yylineno, $1, $3, NULL); }
    ;

VarSection
    : '[' ListaDeclVar ']'
      { $$ = criar_no(AST_VARSECTION, NULL, yylineno, $2, NULL, NULL); }
    ;

ListaDeclVar
    : IDENTIFICADOR DeclVar ':' Tipo ';' ListaDeclVar
      {
          AST *id = criar_no(AST_IDENT, $1.lexema, $1.linha, NULL, NULL, NULL);
          AST *lista_ids = adicionar_irmao(id, $2);
          AST *decl = criar_no(AST_DECLVAR, NULL, $1.linha, lista_ids, $4, NULL);
          $$ = adicionar_irmao(decl, $6);
      }
    | IDENTIFICADOR DeclVar ':' Tipo ';'
      {
          AST *id = criar_no(AST_IDENT, $1.lexema, $1.linha, NULL, NULL, NULL);
          AST *lista_ids = adicionar_irmao(id, $2);
          $$ = criar_no(AST_DECLVAR, NULL, $1.linha, lista_ids, $4, NULL);
      }
    | DeclVetor ListaDeclVar
      {
          $$ = adicionar_irmao($1,$2);
      }
    | DeclVetor
      {
          $$ = $1;
      }
    ;

DeclVar
    : ',' IDENTIFICADOR DeclVar
      {
          AST *id = criar_no(AST_IDENT, $2.lexema, $2.linha, NULL, NULL, NULL);
          $$ = adicionar_irmao(id, $3);
      }
    | %empty
      { $$ = NULL; }
    ;

DeclVetor
    : IDENTIFICADOR '[' INTCONST ']' ':' Tipo ';'
      { 
          AST *id = criar_no(AST_IDENT, $1.lexema, $1.linha, NULL, NULL, NULL);
          AST *tamanho = criar_no(AST_INTCONST, $3.lexema, $3.linha, NULL, NULL, NULL);
          $$ = criar_no(AST_DECL_VETOR, $1.lexema, $1.linha, tamanho, $6, NULL);
      }
    ;

Tipo
    : INT
      { $$ = criar_no(AST_TIPO, "int", yylineno, NULL, NULL, NULL); }
    | CAR
      { $$ = criar_no(AST_TIPO, "car", yylineno, NULL, NULL, NULL); }
    ;

ListaComando
    : Comando
      { $$ = $1; }
    | Comando ListaComando
      { $$ = adicionar_irmao($1, $2); }
    ;

DeclFunc
    : FUNCAO Tipo IDENTIFICADOR '(' ListaParametros ')' Bloco DeclFunc
      {
          AST *func =criar_no(AST_FUNCAO,$3.lexema,$3.linha,$2,$5,$7);
          $$ = adicionar_irmao(func,$8);
      }
    | %empty
      {
          $$ = NULL;
      }
    ;


ListaParametros
    : Tipo IDENTIFICADOR '[' ']' RestoListaParam   
      {
          AST *id = criar_no(AST_IDENT, $2.lexema, $2.linha, NULL, NULL, NULL);
          AST *param = criar_no(AST_PARAM_VETOR, NULL, $2.linha, id, $1, NULL);
          $$ = adicionar_irmao(param, $5);
      }
    | Tipo IDENTIFICADOR RestoListaParam            
      {
          AST *id = criar_no(AST_IDENT, $2.lexema, $2.linha, NULL, NULL, NULL);
          AST *param = criar_no(AST_PARAM, NULL, $2.linha, id, $1, NULL);
          $$ = adicionar_irmao(param, $3);
      }
    | %empty
      { $$ = NULL; }
    ;

RestoListaParam
    : ',' Tipo IDENTIFICADOR '[' ']' RestoListaParam 
      {
          AST *id = criar_no(AST_IDENT, $3.lexema, $3.linha, NULL, NULL, NULL);
          AST *param = criar_no(AST_PARAM_VETOR, NULL, $3.linha, id, $2, NULL);
          $$ = adicionar_irmao(param, $6);
      }
    | ',' Tipo IDENTIFICADOR RestoListaParam    
      {
          AST *id = criar_no(AST_IDENT, $3.lexema, $3.linha, NULL, NULL, NULL);
          AST *param = criar_no(AST_PARAM, NULL, $3.linha, id, $2, NULL);
          $$ = adicionar_irmao(param, $4);
      }
    | %empty
      { $$ = NULL; }
    ;

Comando
    : ';'
      { $$ = criar_no(AST_COMANDO_VAZIO, NULL, yylineno, NULL, NULL, NULL); }
    | Expr ';'
      { $$ = $1; }
    | LEIA IDENTIFICADOR ';'
      {
          AST *id = criar_no(AST_IDENT, $2.lexema, $2.linha, NULL, NULL, NULL);
          $$ = criar_no(AST_LEIA, NULL, $2.linha, id, NULL, NULL);
      }
    | ESCREVA Expr ';'
      { $$ = criar_no(AST_ESCREVA, NULL, yylineno, $2, NULL, NULL); }
    | ESCREVA CADEIACARACTERES ';'
      {
          AST *str = criar_no(AST_STRING, $2.lexema, $2.linha, NULL, NULL, NULL);
          $$ = criar_no(AST_ESCREVA, NULL, $2.linha, str, NULL, NULL);
      }
    | NOVALINHA ';'
      { $$ = criar_no(AST_NOVALINHA, NULL, yylineno, NULL, NULL, NULL); }
    | SE '(' Expr ')' ENTAO Comando FIMSE
      { $$ = criar_no(AST_SE, NULL, yylineno, $3, $6, NULL); }
    | SE '(' Expr ')' ENTAO Comando SENAO Comando FIMSE
      { $$ = criar_no(AST_SE, NULL, yylineno, $3, $6, $8); }
    | ENQUANTO '(' Expr ')' Comando
      { $$ = criar_no(AST_ENQUANTO, NULL, yylineno, $3, $5, NULL); }
    | RETORNE Expr ';'
      { $$ = criar_no(AST_RETORNE, NULL, yylineno, $2, NULL, NULL); }
    | IDENTIFICADOR '[' Expr ']' '=' Expr ';'
      { $$ = criar_no(AST_ATRIB_VETOR, $1.lexema, $1.linha, $3, $6, NULL); }
    | Bloco
      { $$ = $1; }
    ;

Expr
    : OrExpr
      { $$ = $1; }
    | IDENTIFICADOR '=' Expr
      {
          AST *id = criar_no(AST_IDENT, $1.lexema, $1.linha, NULL, NULL, NULL);
          $$ = criar_no(AST_ATRIB, "=", $1.linha, id, $3, NULL);
      }
    ;

OrExpr
    : OrExpr OU AndExpr
      { $$ = criar_no(AST_OP, "||", yylineno, $1, $3, NULL); }
    | AndExpr
      { $$ = $1; }
    ;

AndExpr
    : AndExpr E EqExpr
      { $$ = criar_no(AST_OP, "&", yylineno, $1, $3, NULL); }
    | EqExpr
      { $$ = $1; }
    ;

EqExpr
    : EqExpr IGUAL DesigExpr
      { $$ = criar_no(AST_OP, "==", yylineno, $1, $3, NULL); }
    | EqExpr DIFERENTE DesigExpr
      { $$ = criar_no(AST_OP, "!=", yylineno, $1, $3, NULL); }
    | DesigExpr
      { $$ = $1; }
    ;

DesigExpr
    : DesigExpr '<' AddExpr
      { $$ = criar_no(AST_OP, "<", yylineno, $1, $3, NULL); }
    | DesigExpr '>' AddExpr
      { $$ = criar_no(AST_OP, ">", yylineno, $1, $3, NULL); }
    | DesigExpr MAIORIGUAL AddExpr
      { $$ = criar_no(AST_OP, ">=", yylineno, $1, $3, NULL); }
    | DesigExpr MENORIGUAL AddExpr
      { $$ = criar_no(AST_OP, "<=", yylineno, $1, $3, NULL); }
    | AddExpr
      { $$ = $1; }
    ;

AddExpr
    : AddExpr '+' MulExpr
      { $$ = criar_no(AST_OP, "+", yylineno, $1, $3, NULL); }
    | AddExpr '-' MulExpr
      { $$ = criar_no(AST_OP, "-", yylineno, $1, $3, NULL); }
    | MulExpr
      { $$ = $1; }
    ;

MulExpr
    : MulExpr '*' UnExpr
      { $$ = criar_no(AST_OP, "*", yylineno, $1, $3, NULL); }
    | MulExpr '/' UnExpr
      { $$ = criar_no(AST_OP, "/", yylineno, $1, $3, NULL); }
    | UnExpr
      { $$ = $1; }
    ;

UnExpr
    : '-' PrimExpr
      { $$ = criar_no(AST_OP, "unary-", yylineno, $2, NULL, NULL); }
    | '!' PrimExpr
      { $$ = criar_no(AST_OP, "!", yylineno, $2, NULL, NULL); }
    | PrimExpr
      { $$ = $1; }
    ;

PrimExpr
    : IDENTIFICADOR 
      { $$ = criar_no(AST_IDENT, $1.lexema, $1.linha, NULL, NULL, NULL); }
    | IDENTIFICADOR '[' Expr ']'                   
      { $$ = criar_no(AST_DECL_VETOR, $1.lexema, $1.linha, $3, NULL, NULL); }
    | CARCONST
      { $$ = criar_no(AST_CARCONST, $1.lexema, $1.linha, NULL, NULL, NULL); }
    | INTCONST
      { $$ = criar_no(AST_INTCONST, $1.lexema, $1.linha, NULL, NULL, NULL); }
    | IDENTIFICADOR '(' ListaArgumentos ')'
      { $$ = criar_no(AST_CHAMADA_FUNC, $1.lexema, $1.linha, $3, NULL, NULL); }
    | '(' Expr ')'
      { $$ = $2; }
    ;

ListaArgumentos: Expr { $$ = $1; }
               | ListaArgumentos ',' Expr { $$ = adicionar_irmao($1, $3); }
    | %empty
      {
          $$ = NULL;
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

  if (yyparse() == 0) {
    analisar_semantico(raiz);
    gerar_codigo(raiz);
  }

  if (raiz) {
      //imprimir_ast(raiz, 0);
      liberar_ast(raiz);
  }

  if (yyin) fclose(yyin);
  return 0;
}
