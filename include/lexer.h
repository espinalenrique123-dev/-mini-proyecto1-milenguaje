/* ============================================================
 * lexer.h - Lo que el analizador lexico (Flex) comparte
 * ============================================================ */
#ifndef LEXER_H
#define LEXER_H
#include <stdio.h>

extern FILE *yyin;                /* archivo que se esta leyendo        */
extern char *yytext;              /* texto del token actual             */
extern int errores_lexicos;       /* cuantos errores lexicos hubo       */
extern int linea_token_previo;    /* linea del token anterior           */
extern int mostrar_tokens;        /* 1 si se pidio la tabla de tokens   */
int yylex(void);

#endif
