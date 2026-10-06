/* ============================================================
 * ejecutor.c - Recorre el AST decorado y ejecuta el programa
 * (solo se llama si no hubo errores lexicos/sintacticos/semanticos)
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ejecutor.h"
#include "symtab.h"

static int error_ejecucion = 0;

static double como_double(Valor v) { return v.tipo == T_FLOAT ? v.f : (double)v.i; }

/* Convierte un valor al tipo de destino (promocion int -> float) */
static Valor convertir(Valor v, TipoDato destino) {
    if (destino == T_FLOAT && v.tipo == T_INT) { v.f = (double)v.i; v.tipo = T_FLOAT; }
    return v;
}

static Valor evaluar(Nodo *e) {
    Valor r = { e->tipo, 0, 0.0, "" };
    if (error_ejecucion) return r;

    switch (e->clase) {
    case N_LIT_INT:   r.i = e->ival; break;
    case N_LIT_BOOL:  r.i = e->ival; break;
    case N_LIT_FLOAT: r.f = e->fval; break;
    case N_LIT_STR:   r.s = e->sval; break;
    case N_ID:        r = ts_buscar(e->nombre)->valor; break;

    case N_OP_UNARIA: {
        Valor v = evaluar(e->a);
        if (strcmp(e->nombre, "!") == 0) r.i = !v.i;
        else if (v.tipo == T_FLOAT) r.f = -v.f;
        else r.i = -v.i;
        break;
    }

    case N_OP_BINARIA: {
        const char *op = e->nombre;
        Valor x = evaluar(e->a);
        /* Evaluacion en corto circuito para && y || */
        if (strcmp(op, "&&") == 0 && !x.i) { r.i = 0; break; }
        if (strcmp(op, "||") == 0 &&  x.i) { r.i = 1; break; }
        Valor y = evaluar(e->b);
        if (error_ejecucion) return r;

        if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) { r.i = y.i; break; }

        if (strchr("+-*/", op[0]) && op[1] == '\0') {
            if (e->tipo == T_STR) {                         /* concatenacion */
                char *s = malloc(strlen(x.s) + strlen(y.s) + 1);
                strcpy(s, x.s); strcat(s, y.s); r.s = s;
            } else if (e->tipo == T_INT) {
                if (op[0] == '/' && y.i == 0) {
                    printf("  [X] Error de ejecucion (linea %d): division entre cero\n", e->linea);
                    error_ejecucion = 1; return r;
                }
                switch (op[0]) {
                    case '+': r.i = x.i + y.i; break; case '-': r.i = x.i - y.i; break;
                    case '*': r.i = x.i * y.i; break; case '/': r.i = x.i / y.i; break;
                }
            } else {                                        /* float */
                double a = como_double(x), b = como_double(y);
                if (op[0] == '/' && b == 0.0) {
                    printf("  [X] Error de ejecucion (linea %d): division entre cero\n", e->linea);
                    error_ejecucion = 1; return r;
                }
                switch (op[0]) {
                    case '+': r.f = a + b; break; case '-': r.f = a - b; break;
                    case '*': r.f = a * b; break; case '/': r.f = a / b; break;
                }
            }
            break;
        }

        /* Comparaciones -> bool */
        int cmp;
        if (x.tipo == T_STR)       cmp = strcmp(x.s, y.s);
        else if (x.tipo == T_BOOL) cmp = (int)(x.i - y.i);
        else { double a = como_double(x), b = como_double(y); cmp = (a > b) - (a < b); }

        if      (!strcmp(op, "=="))  r.i = cmp == 0;
        else if (!strcmp(op, "!="))  r.i = cmp != 0;
        else if (!strcmp(op, "<"))   r.i = cmp < 0;
        else if (!strcmp(op, ">"))   r.i = cmp > 0;
        else if (!strcmp(op, "<="))  r.i = cmp <= 0;
        else if (!strcmp(op, ">="))  r.i = cmp >= 0;
        break;
    }
    default: break;
    }
    return r;
}

int ejecutar(Nodo *programa) {
    error_ejecucion = 0;
    /* La memoria arranca con valores por defecto: 0, 0.0, "", false */
    for (int i = 0; i < ts_cantidad(); i++) {
        Simbolo *s = ts_obtener(i);
        Valor v = { s->tipo, 0, 0.0, "" };
        s->valor = v;
    }

    for (Nodo *s = programa->a; s && !error_ejecucion; s = s->sig) {
        switch (s->clase) {
        case N_DECLARACION:
            if (s->a) ts_buscar(s->nombre)->valor = convertir(evaluar(s->a), s->tipo);
            break;
        case N_ASIGNACION:
            ts_buscar(s->nombre)->valor = convertir(evaluar(s->a), s->tipo);
            break;
        case N_IMPRIMIR: {
            Valor v = evaluar(s->a);
            if (!error_ejecucion) {
                char buf[1024];
                valor_a_texto(v, buf, sizeof buf);
                printf("  > %s\n", buf);
            }
            break;
        }
        default: break;
        }
    }
    return error_ejecucion;
}
