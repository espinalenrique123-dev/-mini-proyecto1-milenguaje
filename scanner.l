/* ============================================================
 * scanner.l - Analizador Lexico de MiLenguaje (Flex)
 * Mini Proyecto 1 - IS-505 - Moises Moncada
 * ============================================================ */
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

extern int mostrar_tokens;
int errores_lexicos = 0;
int linea_token_previo = 1, linea_token_actual = 1;

/* Guarda la linea de cada token para los mensajes de error */
#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;

/* Devuelve un token y, si se pidio con -t, lo imprime */
#define TOKEN(t, nombre) do { \
    linea_token_previo = linea_token_actual; linea_token_actual = yylineno; \
    if (mostrar_tokens) printf("  | %-5d | %-14s | %-20.20s |\n", yylineno, nombre, yytext); \
    return t; } while (0)
%}

%option noyywrap yylineno nounput noinput

DIGITO   [0-9]
LETRA    [a-zA-Z_]

%%

"//".*                  { /* comentario: se ignora */ }
[ \t\r\n]+              { /* espacios: se ignoran */ }

 /* ---------- Palabras reservadas (antes que ID) ---------- */
"var"                   { TOKEN(VAR, "VAR"); }
"print"                 { TOKEN(PRINT, "PRINT"); }
"func"                  { TOKEN(FUNC, "FUNC"); }
"return"                { TOKEN(RETURN, "RETURN"); }
"void"                  { yylval.tipo = T_VOID; TOKEN(VOID, "VOID"); }
"int"                   { yylval.tipo = T_INT;   TOKEN(TIPO, "TIPO"); }
"float"                 { yylval.tipo = T_FLOAT; TOKEN(TIPO, "TIPO"); }
"str"                   { yylval.tipo = T_STR;   TOKEN(TIPO, "TIPO"); }
"bool"                  { yylval.tipo = T_BOOL;  TOKEN(TIPO, "TIPO"); }
"true"                  { yylval.ival = 1; TOKEN(BOOL_LIT, "BOOL_LIT"); }
"false"                 { yylval.ival = 0; TOKEN(BOOL_LIT, "BOOL_LIT"); }

 /* ---------- Literales ---------- */
{DIGITO}+"."{DIGITO}+   { yylval.fval = atof(yytext);  TOKEN(FLOAT_LIT, "FLOAT_LIT"); }
{DIGITO}+               { yylval.ival = atoll(yytext); TOKEN(INT_LIT, "INT_LIT"); }
\"[^\"\n]*\"            { yylval.sval = strndup(yytext + 1, yyleng - 2); TOKEN(STR_LIT, "STR_LIT"); }
\"[^\"\n]*              { printf("  [X] Error lexico (linea %d): cadena sin cerrar %s\n", yylineno, yytext);
                          errores_lexicos++; }

 /* ---------- Identificadores ---------- */
{LETRA}({LETRA}|{DIGITO})*  { yylval.sval = strdup(yytext); TOKEN(ID, "ID"); }

 /* ---------- Operadores dobles (antes que los simples) ---------- */
"=="                    { TOKEN(EQ, "IGUAL"); }
"!="                    { TOKEN(NE, "DIFERENTE"); }
"<="                    { TOKEN(LE, "MENOR_IGUAL"); }
">="                    { TOKEN(GE, "MAYOR_IGUAL"); }
"&&"                    { TOKEN(AND, "AND"); }
"||"                    { TOKEN(OR, "OR"); }

 /* ---------- Operadores y signos simples ---------- */
":"                     { TOKEN(':', "DOS_PUNTOS"); }
";"                     { TOKEN(';', "PUNTO_COMA"); }
"="                     { TOKEN('=', "ASIGNACION"); }
"+"                     { TOKEN('+', "SUMA"); }
"-"                     { TOKEN('-', "RESTA"); }
"*"                     { TOKEN('*', "MULTIPLICACION"); }
"/"                     { TOKEN('/', "DIVISION"); }
"<"                     { TOKEN('<', "MENOR"); }
">"                     { TOKEN('>', "MAYOR"); }
"!"                     { TOKEN('!', "NOT"); }
"("                     { TOKEN('(', "PAR_ABRE"); }
")"                     { TOKEN(')', "PAR_CIERRA"); }
"{"                     { TOKEN('{', "LLAVE_ABRE"); }
"}"                     { TOKEN('}', "LLAVE_CIERRA"); }
","                     { TOKEN(',', "COMA"); }

 /* ---------- Cualquier otro caracter es un error lexico ---------- */
.                       { printf("  [X] Error lexico (linea %d): caracter no reconocido '%s'\n", yylineno, yytext);
                          errores_lexicos++; }

%%
