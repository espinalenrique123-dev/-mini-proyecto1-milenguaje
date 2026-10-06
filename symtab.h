/* ============================================================
 * symtab.h - Tabla de Simbolos de MiLenguaje
 * ============================================================ */
#ifndef SYMTAB_H
#define SYMTAB_H
#include "ast.h"

/* Valor en memoria de una variable (lo usa la ejecucion) */
typedef struct {
    TipoDato tipo;
    long long i;     /* int / bool */
    double f;        /* float      */
    char *s;         /* str        */
} Valor;

typedef struct {
    char *nombre;
    TipoDato tipo;
    const char *ambito;   /* "global" */
    int linea;            /* linea de declaracion            */
    int inicializada;     /* 1 si recibio valor              */
    int usos;             /* cuantas veces se lee la variable */
    Valor valor;          /* valor actual (ejecucion)        */
} Simbolo;

void     ts_iniciar(void);
Simbolo *ts_buscar(const char *nombre);
Simbolo *ts_insertar(const char *nombre, TipoDato tipo, int linea);
int      ts_cantidad(void);
Simbolo *ts_obtener(int i);
void     ts_imprimir(int con_valores);
void     valor_a_texto(Valor v, char *buf, int tam);

#endif
