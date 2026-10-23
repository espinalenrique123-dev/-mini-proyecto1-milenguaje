/* ============================================================
 * main.c - Programa principal del compilador de MiLenguaje
 * Coordina las fases: lexico + sintactico -> AST -> semantico
 * -> ejecucion, y muestra las tablas de cada fase.
 * Mini Proyecto 1 - IS-505 - Moises Moncada
 * ============================================================ */
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"
#include "semantico.h"
#include "ejecutor.h"
#include "arreglos.h"
#include "parser.h"
#include "lexer.h"

int mostrar_tokens = 0;

static void titulo(const char *t) {
    printf("\n==================================================================\n");
    printf("  %s\n", t);
    printf("==================================================================\n");
}

int main(int argc, char **argv) {
    const char *archivo = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0) mostrar_tokens = 1;
        else archivo = argv[i];
    }
    if (!archivo) {
        printf("Uso: %s [-t] archivo.mil\n   -t  muestra la tabla de tokens\n", argv[0]);
        return 1;
    }
    yyin = fopen(archivo, "r");
    if (!yyin) { printf("No se pudo abrir el archivo '%s'\n", archivo); return 1; }

    printf("\n  COMPILADOR DE MiLenguaje  -  archivo: %s\n", archivo);

    /* ---------- FASE 1 y 2: lexico + sintactico ---------- */
    titulo("FASE 1-2: ANALISIS LEXICO Y SINTACTICO");
    if (mostrar_tokens) {
        printf("  +-------+----------------+----------------------+\n");
        printf("  | LINEA | TOKEN          | LEXEMA               |\n");
        printf("  +-------+----------------+----------------------+\n");
    }
    yyparse();
    if (mostrar_tokens) printf("  +-------+----------------+----------------------+\n");

    if (errores_lexicos || errores_sintacticos) {
        printf("\n  Resultado: %d error(es) lexico(s), %d error(es) sintactico(s).\n",
               errores_lexicos, errores_sintacticos);
        printf("  Compilacion detenida.\n\n");
        return 1;
    }
    printf("  [OK] Programa lexica y sintacticamente correcto (%d sentencias).\n",
           ast_contar_sentencias(raiz));

    titulo("ARBOL DE SINTAXIS ABSTRACTA (AST)");
    ast_imprimir_arbol(raiz);

    /* ---------- FASE 3: semantico ---------- */
    titulo("FASE 3: ANALISIS SEMANTICO");
    int advertencias = 0;
    int errores_sem = analizar_semantica(raiz, &advertencias);
    if (errores_sem == 0)
        printf("  [OK] Sin errores semanticos (%d advertencia(s)).\n", advertencias);
    else
        printf("\n  Resultado: %d error(es) semantico(s), %d advertencia(s).\n", errores_sem, advertencias);

    titulo("TABLA AST (decorada con tipos)");
    ast_imprimir_tabla(raiz);

    titulo("TABLA DE SIMBOLOS");
    ts_imprimir(0);

    if (errores_sem) { printf("\n  Compilacion detenida: corrija los errores semanticos.\n\n"); return 1; }

    /* ---------- FASE 4: ejecucion ---------- */
    titulo("FASE 4: EJECUCION DEL PROGRAMA");
    int fallo = ejecutar(raiz);

    titulo("ESTADO FINAL DE LA MEMORIA");
    ts_imprimir(1);
    arreglos_liberar();
    printf("\n  %s\n\n", fallo ? "El programa termino con un error de ejecucion."
                               : "Programa ejecutado correctamente.");
    return fallo;
}
