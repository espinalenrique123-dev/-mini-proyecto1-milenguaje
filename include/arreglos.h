/* ============================================================
 * arreglos.h - Arreglos de MiLenguaje en tiempo de ejecucion
 * (semana 9)
 *
 * Un arreglo es una lista que crece sola: push agrega al final,
 * pop quita el ultimo y length dice cuantos elementos tiene.
 * Las variables guardan un puntero al arreglo (como una
 * referencia): si se pasa a una funcion, la funcion ve el mismo
 * arreglo.
 * ============================================================ */
#ifndef ARREGLOS_H
#define ARREGLOS_H
#include "symtab.h"

typedef struct Arreglo {
    TipoDato tipo_elem;        /* int, float, str o bool             */
    int largo;                 /* elementos que tiene ahora          */
    int capacidad;             /* espacio reservado                  */
    Valor *items;              /* los elementos                      */
    struct Arreglo *siguiente; /* lista de todos los arreglos creados */
} Arreglo;

Arreglo *arreglo_nuevo(TipoDato tipo_elem);
int      arreglo_push(Arreglo *a, Valor v);      /* 1 = ok, 0 = sin memoria       */
int      arreglo_pop(Arreglo *a, Valor *salida); /* 1 = ok, 0 = arreglo vacio     */
void     arreglo_a_texto(Arreglo *a, char *buf, int tam);  /* [1, 2, 3]           */
void     arreglos_liberar(void);                 /* libera todos al terminar      */

#endif
