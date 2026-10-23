/* ============================================================
 * parser.h - Lo que el parser (Bison) comparte con main.c
 * ============================================================ */
#ifndef PARSER_H
#define PARSER_H
#include "ast.h"

extern Nodo *raiz;                /* AST que construye el parser       */
extern int errores_sintacticos;   /* cuantos errores de sintaxis hubo  */
int yyparse(void);

#endif
