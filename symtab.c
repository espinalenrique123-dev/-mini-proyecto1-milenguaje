/* ============================================================
 * symtab.c - Tabla de Simbolos (arreglo con busqueda lineal)
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

#define MAX_SIMBOLOS 256
static Simbolo tabla[MAX_SIMBOLOS];
static int cantidad = 0;

void ts_iniciar(void) { cantidad = 0; }

Simbolo *ts_buscar(const char *nombre) {
    for (int i = 0; i < cantidad; i++)
        if (strcmp(tabla[i].nombre, nombre) == 0) return &tabla[i];
    return NULL;
}

Simbolo *ts_insertar(const char *nombre, TipoDato tipo, int linea) {
    if (cantidad >= MAX_SIMBOLOS) return NULL;
    Simbolo *s = &tabla[cantidad++];
    memset(s, 0, sizeof *s);
    s->nombre = strdup(nombre);
    s->tipo = tipo;
    s->ambito = "global";
    s->linea = linea;
    s->valor.tipo = tipo;
    s->valor.s = "";
    return s;
}

int ts_cantidad(void) { return cantidad; }
Simbolo *ts_obtener(int i) { return &tabla[i]; }

void valor_a_texto(Valor v, char *buf, int tam) {
    switch (v.tipo) {
        case T_INT:   snprintf(buf, tam, "%lld", v.i); break;
        case T_FLOAT:
            if (v.f == (long long)v.f) snprintf(buf, tam, "%.1f", v.f);
            else snprintf(buf, tam, "%g", v.f);
            break;
        case T_BOOL:  snprintf(buf, tam, "%s", v.i ? "true" : "false"); break;
        case T_STR:   snprintf(buf, tam, "%s", v.s); break;
        default:      snprintf(buf, tam, "?"); break;
    }
}

void ts_imprimir(int con_valores) {
    const char *sep = con_valores
        ? "+----+--------------+--------+---------+-------+--------------+------+----------------------+\n"
        : "+----+--------------+--------+---------+-------+--------------+------+\n";
    printf("%s", sep);
    printf("| %-2s | %-12s | %-6s | %-7s | %-5s | %-12s | %-4s |", "#", "NOMBRE", "TIPO", "AMBITO", "LINEA", "INICIALIZADA", "USOS");
    if (con_valores) printf(" %-20s |", "VALOR");
    printf("\n%s", sep);
    if (cantidad == 0) printf("| (tabla vacia)\n");
    for (int i = 0; i < cantidad; i++) {
        Simbolo *s = &tabla[i];
        printf("| %-2d | %-12.12s | %-6s | %-7s | %-5d | %-12s | %-4d |", i + 1, s->nombre,
               nombre_tipo(s->tipo), s->ambito, s->linea, s->inicializada ? "si" : "no", s->usos);
        if (con_valores) {
            char v[256];
            valor_a_texto(s->valor, v, sizeof v);
            char q[260];
            if (s->tipo == T_STR) snprintf(q, sizeof q, "\"%s\"", v);
            else snprintf(q, sizeof q, "%s", v);
            if (strlen(q) > 20) strcpy(q + 17, "...");   /* cabe en la columna */
            printf(" %-20s |", q);
        }
        printf("\n");
    }
    printf("%s", sep);
}
