%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "g-v2.tab.h"

extern int yylineno;
%}

%option noyywrap
%option yylineno

LETRA      [a-zA-Z_]
DIGITO     [0-9]
ID         {LETRA}({LETRA}|{DIGITO})*
INTNUM     {DIGITO}+
CARACT     \'([^\\\n]|\\[ntr0\'\"\\])\'
CADEIA     \"[^\"\n]*\"

%%

"principal"         { return PRINCIPAL; }
"int"               { return INT; }
"car"               { return CAR; }
"leia"              { return LEIA; }
"escreva"           { return ESCREVA; }
"novalinha"         { return NOVALINHA; }
"se"                { return SE; }
"entao"             { return ENTAO; }
"senao"             { return SENAO; }
"fimse"             { return FIMSE; }
"enquanto"          { return ENQUANTO; }
"funcao"            { return FUNCAO; }
"global"            { return GLOBAL; }
"retorne"           { return RETORNE; }

"||"                { return OU; }
"&"                 { return E; }
"=="                { return IGUAL; }
"!="                { return DIFERENTE; }
">="                { return MAIORIGUAL; }
"<="                { return MENORIGUAL; }
"["                 { return '['; }
"]"                 { return ']'; }

{INTNUM} {
    yylval.tk.lexema = strdup(yytext);
    yylval.tk.linha = yylineno;
    return INTCONST;
}

{CARACT} {
    yylval.tk.lexema = strdup(yytext);
    yylval.tk.linha = yylineno;
    return CARCONST;
}

{CADEIA} {
    yylval.tk.lexema = strdup(yytext);
    yylval.tk.linha = yylineno;
    return CADEIACARACTERES;
}

{ID} {
    yylval.tk.lexema = strdup(yytext);
    yylval.tk.linha = yylineno;
    return IDENTIFICADOR;
}

":"                 { return ':'; }
";"                 { return ';'; }
","                 { return ','; }
"="                 { return '='; }
"{"                 { return '{'; }
"}"                 { return '}'; }
"("                 { return '('; }
")"                 { return ')'; }
"<"                 { return '<'; }
">"                 { return '>'; }
"+"                 { return '+'; }
"-"                 { return '-'; }
"*"                 { return '*'; }
"/"                 { return '/'; }
"!"                 { return '!'; }

"/*"                {
                        int c, prev = 0;
                        while ((c = input()) != 0) {
                            if (prev == '*' && c == '/') break;
                            prev = c;
                        }
                        if (c == 0) {
                            printf("ERRO: COMENTARIO NAO TERMINA %d\n", yylineno);
                            exit(1);
                        }
                    }

\"[^\"\n]*\n        {
                        printf("ERRO: CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA %d\n", yylineno);
                        exit(1);
                    }

[ \t\r]+            { /* ignora espaços */ }
\n                  { /* yylineno atualizado automaticamente */ }

.                   {
                        printf("ERRO: CARACTERE INVALIDO %d\n", yylineno);
                        exit(1);
                    }

%%
