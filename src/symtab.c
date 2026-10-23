/* ============================================================
 * symtab.c - Tabla de Simbolos (arreglo con busqueda lineal)
 *
 * Ambitos: cada simbolo guarda el nombre de su ambito ("global", una
 * funcion o un bloque if/else). Al buscar un nombre se revisa desde el
 * ambito mas interno hacia afuera, hasta llegar al global.
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "arreglos.h"

#define MAX_SIMBOLOS 512
static Simbolo tabla[MAX_SIMBOLOS];
static int cantidad = 0;
/* Pila de ambitos: global -> funcion -> bloque if -> bloque if anidado ... */
#define MAX_AMBITOS 64
static char pila[MAX_AMBITOS][64];
static int nivel = 0;                       /* 0 = global */

void ts_iniciar(void) { cantidad = 0; nivel = 0; strcpy(pila[0], "global"); }

void ts_entrar_ambito(const char *nombre) {
    if (nivel < MAX_AMBITOS - 1) snprintf(pila[++nivel], sizeof pila[0], "%s", nombre);
}
void ts_salir_ambito(void) { if (nivel > 0) nivel--; }
const char *ts_ambito_actual(void) { return pila[nivel]; }

Simbolo *ts_buscar_en_ambito(const char *nombre, const char *ambito) {
    for (int i = 0; i < cantidad; i++)
        if (strcmp(tabla[i].nombre, nombre) == 0 && strcmp(tabla[i].ambito, ambito) == 0)
            return &tabla[i];
    return NULL;
}

/* Busca desde el ambito mas interno hacia afuera, hasta el global */
Simbolo *ts_buscar(const char *nombre) {
    for (int k = nivel; k >= 0; k--) {
        Simbolo *s = ts_buscar_en_ambito(nombre, pila[k]);
        if (s) return s;
    }
    return NULL;
}

Simbolo *ts_insertar(const char *nombre, TipoDato tipo, Categoria cat, int linea) {
    if (cantidad >= MAX_SIMBOLOS) return NULL;
    Simbolo *s = &tabla[cantidad++];
    memset(s, 0, sizeof *s);
    s->nombre = strdup(nombre);
    s->tipo = tipo;
    s->categoria = cat;
    s->ambito = strdup(pila[nivel]);
    s->es_local = nivel > 0;
    s->linea = linea;
    s->valor.tipo = tipo;
    s->valor.s = "";
    return s;
}

int ts_cantidad(void) { return cantidad; }
Simbolo *ts_obtener(int i) { return &tabla[i]; }

const char *nombre_categoria(Categoria c) {
    switch (c) {
        case S_VARIABLE:  return "variable";
        case S_PARAMETRO: return "parametro";
        default:          return "funcion";
    }
}

void valor_a_texto(Valor v, char *buf, int tam) {
    switch (v.tipo) {
        case T_INT:   snprintf(buf, tam, "%lld", v.i); break;
        case T_FLOAT:
            if (v.f == (long long)v.f) snprintf(buf, tam, "%.1f", v.f);
            else snprintf(buf, tam, "%g", v.f);
            break;
        case T_BOOL:  snprintf(buf, tam, "%s", v.i ? "true" : "false"); break;
        case T_STR:   snprintf(buf, tam, "%s", v.s); break;
        default:
            if (es_arreglo(v.tipo)) arreglo_a_texto(v.arr, buf, tam);   /* [1, 2, 3] */
            else snprintf(buf, tam, "-");
            break;
    }
}

void ts_imprimir(int con_valores) {
    const char *sep = con_valores
        ? "+----+-------------+-------------+---------+-------------+-------+------+------+----------------------+\n"
        : "+----+-------------+-------------+---------+-------------+-------+------+------+\n";
    printf("%s", sep);
    printf("| %-2s | %-11s | %-11s | %-7s | %-11s | %-5s | %-4s | %-4s |", "#", "NOMBRE",
           "CATEGORIA", "TIPO", "AMBITO", "LINEA", "INIC", "USOS");
    if (con_valores) printf(" %-20s |", "VALOR");
    printf("\n%s", sep);
    if (cantidad == 0) printf("| (tabla vacia)\n");
    for (int i = 0; i < cantidad; i++) {
        Simbolo *s = &tabla[i];
        char cat[16];
        if (s->categoria == S_FUNCION) snprintf(cat, sizeof cat, "funcion(%d)", s->num_params);
        else snprintf(cat, sizeof cat, "%s", nombre_categoria(s->categoria));
        printf("| %-2d | %-11.11s | %-11.11s | %-7s | %-11.11s | %-5d | %-4s | %-4d |", i + 1,
               s->nombre, cat, nombre_tipo(s->tipo), s->ambito, s->linea,
               s->categoria == S_FUNCION ? "-" : (s->inicializada ? "si" : "no"), s->usos);
        if (con_valores) {
            char v[256], q[260];
            if (s->categoria == S_FUNCION) strcpy(v, "-");
            else valor_a_texto(s->valor, v, sizeof v);
            if (s->tipo == T_STR && s->categoria != S_FUNCION) snprintf(q, sizeof q, "\"%s\"", v);
            else snprintf(q, sizeof q, "%s", v);
            if (strlen(q) > 20) strcpy(q + 17, "...");   /* cabe en la columna */
            printf(" %-20s |", q);
        }
        printf("\n");
    }
    printf("%s", sep);
}
