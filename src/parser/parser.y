/* ============================================================
 * parser.y - Analizador Sintactico de MiLenguaje (Bison)
 * Verifica la gramatica y construye el AST
 * (el programa principal esta en src/main.c)
 * Mini Proyecto 1 - IS-505 - Moises Moncada
 * ============================================================ */
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.h"
#include "lexer.h"

void yyerror(const char *s);

int errores_sintacticos = 0;
Nodo *raiz = NULL;
%}

%code requires { #include "ast.h" }

%locations
%define parse.error custom
%define parse.lac full

%union {
    long long ival;
    double    fval;
    char     *sval;
    TipoDato  tipo;
    Nodo     *nodo;
}

/* Los textos entre comillas aparecen en los mensajes de error */
%token VAR "var" PRINT "print" FUNC "func" RETURN "return" IF "if" ELSE "else"
%token <tipo> VOID "void"
%token <tipo> TIPO "tipo de dato"
%token <sval> ID "identificador" STR_LIT "cadena"
%token <ival> INT_LIT "entero" BOOL_LIT "booleano"
%token <fval> FLOAT_LIT "decimal"
%token EQ "==" NE "!=" LE "<=" GE ">=" AND "&&" OR "||"

%type <nodo> lista_sentencias sentencia declaracion asignacion impresion expr
%type <nodo> funcion parametros_opt parametros parametro cuerpo retorno llamada argumentos_opt argumentos
%type <nodo> si bloque sino_opt metodo
%type <tipo> tipo tipo_retorno

/* Precedencia: de menor (arriba) a mayor (abajo) */
%left OR
%left AND
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/'
%right '!' UMINUS

%start programa

%%

programa
    : lista_sentencias                  { raiz = ast_programa($1); }
    ;

lista_sentencias
    : %empty                            { $$ = NULL; }
    | lista_sentencias sentencia        { $$ = ast_agregar($1, $2); }
    ;

sentencia
    : declaracion
    | asignacion
    | impresion
    | funcion
    | retorno
    | si
    | llamada ';'                       { $$ = $1; }
    | metodo ';'                        { $$ = $1; }
    | error ';'                         { yyerrok; $$ = NULL; }   /* recuperacion */
    | error '}'                         { yyerrok; $$ = NULL; }
    ;

/* var : <tipo> <identificador> ;   |   var : <tipo> <identificador> = <expresion> ; */
declaracion
    : VAR ':' tipo ID ';'               { $$ = ast_declaracion($3, $4, NULL, @1.first_line); }
    | VAR ':' tipo ID '=' expr ';'      { $$ = ast_declaracion($3, $4, $6,   @1.first_line); }
    ;

/* <tipo> puede ser basico (int) o un arreglo (int[])  -- arreglos: semana 9 */
tipo
    : TIPO
    | TIPO '[' ']'                      { $$ = tipo_arreglo($1); }
    ;

/* <identificador> = <expresion> ; */
asignacion
    : ID '=' expr ';'                   { $$ = ast_asignacion($1, $3, @1.first_line); }
    ;

/* print ( <expresion> ) ; */
impresion
    : PRINT '(' expr ')' ';'            { $$ = ast_imprimir($3, @1.first_line); }
    ;

/* ---------------- FUNCIONES ----------------
   func <nombre> ( <parametros> ) : <tipo> { <sentencias> }
   El tipo de retorno puede ser int, float, str, bool o void. */
funcion
    : FUNC ID '(' parametros_opt ')' ':' tipo_retorno '{' cuerpo '}'
                                        { $$ = ast_funcion($2, $4, $7, $9, @1.first_line); }
    ;

tipo_retorno
    : tipo
    | VOID
    ;

parametros_opt
    : %empty                            { $$ = NULL; }
    | parametros
    ;

/* a:int, b:float, ... */
parametros
    : parametro
    | parametros ',' parametro          { $$ = ast_agregar($1, $3); }
    ;

parametro
    : ID ':' tipo                       { $$ = ast_parametro($1, $3, @1.first_line); }
    ;

/* El cuerpo es una lista de sentencias (el semantico prohibe funciones anidadas) */
cuerpo
    : %empty                            { $$ = NULL; }
    | cuerpo sentencia                  { $$ = ast_agregar($1, $2); }
    ;

/* return <expresion> ;   |   return ; */
retorno
    : RETURN expr ';'                   { $$ = ast_retorno($2, @1.first_line); }
    | RETURN ';'                        { $$ = ast_retorno(NULL, @1.first_line); }
    ;

/* ---------------- IF (semana 8) ----------------
   if <condicion> : { <sentencias> }
   if <condicion> : { <sentencias> } else { <sentencias> }
   Los dos puntos ':' son opcionales:  if (x > y) { ... }  tambien vale. */
si
    : IF expr dos_puntos_opt bloque sino_opt
                                        { $4->nombre = strdup("if");
                                          $$ = ast_si($2, $4, $5, @1.first_line); }
    ;

dos_puntos_opt
    : %empty
    | ':'
    ;

sino_opt
    : %empty                            { $$ = NULL; }
    | ELSE bloque                       { $2->nombre = strdup("else"); $$ = $2; }
    ;

bloque
    : '{' cuerpo '}'                    { $$ = ast_bloque("", $2, @1.first_line); }
    ;

/* ---------------- ARREGLOS (semana 9) ----------------
   <arreglo> . <metodo> ( <argumentos> )     metodos: push(x), pop(), length() */
metodo
    : ID '.' ID '(' argumentos_opt ')'  { $$ = ast_metodo($1, $3, $5, @1.first_line); }
    ;

/* <nombre> ( <argumentos> ) */
llamada
    : ID '(' argumentos_opt ')'         { $$ = ast_llamada($1, $3, @1.first_line); }
    ;

argumentos_opt
    : %empty                            { $$ = NULL; }
    | argumentos
    ;

argumentos
    : expr
    | argumentos ',' expr               { $$ = ast_agregar($1, $3); }
    ;

expr
    : expr '+' expr                     { $$ = ast_binaria("+",  $1, $3, @2.first_line); }
    | expr '-' expr                     { $$ = ast_binaria("-",  $1, $3, @2.first_line); }
    | expr '*' expr                     { $$ = ast_binaria("*",  $1, $3, @2.first_line); }
    | expr '/' expr                     { $$ = ast_binaria("/",  $1, $3, @2.first_line); }
    | expr '<' expr                     { $$ = ast_binaria("<",  $1, $3, @2.first_line); }
    | expr '>' expr                     { $$ = ast_binaria(">",  $1, $3, @2.first_line); }
    | expr LE  expr                     { $$ = ast_binaria("<=", $1, $3, @2.first_line); }
    | expr GE  expr                     { $$ = ast_binaria(">=", $1, $3, @2.first_line); }
    | expr EQ  expr                     { $$ = ast_binaria("==", $1, $3, @2.first_line); }
    | expr NE  expr                     { $$ = ast_binaria("!=", $1, $3, @2.first_line); }
    | expr AND expr                     { $$ = ast_binaria("&&", $1, $3, @2.first_line); }
    | expr OR  expr                     { $$ = ast_binaria("||", $1, $3, @2.first_line); }
    | '-' expr %prec UMINUS             { $$ = ast_unaria("-", $2, @1.first_line); }
    | '!' expr                          { $$ = ast_unaria("!", $2, @1.first_line); }
    | '(' expr ')'                      { $$ = $2; }
    | INT_LIT                           { $$ = ast_int($1,   @1.first_line); }
    | FLOAT_LIT                         { $$ = ast_float($1, @1.first_line); }
    | STR_LIT                           { $$ = ast_str($1,   @1.first_line); }
    | BOOL_LIT                          { $$ = ast_bool((int)$1, @1.first_line); }
    | ID                                { $$ = ast_id($1,    @1.first_line); }
    | llamada
    | '[' argumentos_opt ']'            { $$ = ast_arreglo($2, @1.first_line); }   /* [1, 2, 3] */
    | ID '[' expr ']'                   { $$ = ast_indice($1, $3, @1.first_line); } /* lista[i]  */
    | metodo                                                                         /* lista.pop() */
    ;

%%

/* Errores que no son de sintaxis (por ejemplo, memoria agotada) */
void yyerror(const char *s) {
    printf("  [X] Error (linea %d): %s\n", yylloc.first_line, s);
    errores_sintacticos++;
}

/* Busca si un simbolo esta dentro de la lista de tokens esperados */
static int espera(yysymbol_kind_t *esp, int n, const char *nombre) {
    for (int i = 0; i < n; i++)
        if (strcmp(yysymbol_name(esp[i]), nombre) == 0) return 1;
    return 0;
}

/* Reporte de errores sintacticos en espanol (parse.error custom) */
static int yyreport_syntax_error(const yypcontext_t *ctx) {
    yysymbol_kind_t esp[YYNTOKENS];
    int n = yypcontext_expected_tokens(ctx, esp, YYNTOKENS);
    if (n < 0) n = 0;
    int linea = yypcontext_location(ctx)->first_line;

    char encontrado[128];
    if (yypcontext_token(ctx) == YYSYMBOL_YYEOF) snprintf(encontrado, sizeof encontrado, "el fin de archivo");
    else snprintf(encontrado, sizeof encontrado, "'%s'", yytext);

    errores_sintacticos++;
    printf("  [X] Error sintactico ");

    if (espera(esp, n, "';'") && linea_token_previo < linea) {
        printf("(linea %d): falta ';' al final de la linea\n", linea_token_previo);
    } else if (espera(esp, n, "';'")) {
        printf("(linea %d): se encontro %s, se esperaba ';'\n", linea, encontrado);
    } else if (espera(esp, n, "')'")) {
        printf("(linea %d): se encontro %s, se esperaba ')'\n", linea, encontrado);
    } else if (espera(esp, n, "identificador") && espera(esp, n, "entero")) {
        printf("(linea %d): se encontro %s, se esperaba una expresion\n", linea, encontrado);
    } else {
        printf("(linea %d): se encontro %s", linea, encontrado);
        for (int i = 0; i < n; i++)
            printf("%s%s", i == 0 ? ", se esperaba " : (i == n - 1 ? " o " : ", "),
                   yysymbol_name(esp[i]));
        printf("\n");
    }
    return 0;
}
