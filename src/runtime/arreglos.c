/* ============================================================
 * arreglos.c - Arreglos de MiLenguaje en tiempo de ejecucion
 * (semana 9)
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arreglos.h"

/* Todos los arreglos creados, para liberarlos al final */
static Arreglo *creados = NULL;

Arreglo *arreglo_nuevo(TipoDato tipo_elem) {
    Arreglo *a = calloc(1, sizeof(Arreglo));
    if (!a) return NULL;
    a->tipo_elem = tipo_elem;
    a->siguiente = creados;
    creados = a;
    return a;
}

/* Duplica el espacio cuando se llena (4, 8, 16, ...) */
static int crecer(Arreglo *a) {
    int nueva = a->capacidad == 0 ? 4 : a->capacidad * 2;
    Valor *items = realloc(a->items, nueva * sizeof(Valor));
    if (!items) return 0;
    a->items = items;
    a->capacidad = nueva;
    return 1;
}

int arreglo_push(Arreglo *a, Valor v) {
    if (a->largo == a->capacidad && !crecer(a)) return 0;
    a->items[a->largo++] = v;
    return 1;
}

int arreglo_pop(Arreglo *a, Valor *salida) {
    if (a->largo == 0) return 0;
    *salida = a->items[--a->largo];
    return 1;
}

/* Escribe el arreglo como texto: [1, 2, 3]  o  ["a", "b"] */
void arreglo_a_texto(Arreglo *a, char *buf, int tam) {
    int k = snprintf(buf, tam, "[");
    for (int i = 0; a && i < a->largo && k < tam; i++) {
        char elem[256];
        Valor v = a->items[i];
        if (v.tipo == T_STR) snprintf(elem, sizeof elem, "\"%s\"", v.s);
        else valor_a_texto(v, elem, sizeof elem);
        k += snprintf(buf + k, tam - k, "%s%s", i ? ", " : "", elem);
    }
    if (k < tam - 1) snprintf(buf + k, tam - k, "]");
    else if (tam > 4) strcpy(buf + tam - 5, "...]");   /* no cabe: se recorta */
}

void arreglos_liberar(void) {
    while (creados) {
        Arreglo *a = creados;
        creados = a->siguiente;
        free(a->items);
        free(a);
    }
}
