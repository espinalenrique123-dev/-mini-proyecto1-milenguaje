/* ============================================================
 * semantico.c - Analizador Semantico
 *  - Llena la Tabla de Simbolos
 *  - Verifica declaracion antes de uso y no redeclaracion
 *  - Verifica compatibilidad de tipos (decora el AST con tipos)
 *  - Detecta division entre cero literal
 *  - Advierte variables sin inicializar o nunca usadas
 * ============================================================ */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "semantico.h"
#include "symtab.h"

static int errores = 0, advertencias = 0;

static void error_sem(int linea, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    printf("  [X] Error semantico (linea %d): ", linea);
    vprintf(fmt, ap); printf("\n");
    va_end(ap);
    errores++;
}
static void advertencia(int linea, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    printf("  [!] Advertencia (linea %d): ", linea);
    vprintf(fmt, ap); printf("\n");
    va_end(ap);
    advertencias++;
}

static int es_numerico(TipoDato t) { return t == T_INT || t == T_FLOAT; }
static int es_op(Nodo *n, const char *op) { return strcmp(n->nombre, op) == 0; }

/* Regla de compatibilidad: mismo tipo, o int -> float (promocion) */
static int compatible(TipoDato destino, TipoDato origen) {
    return destino == origen || (destino == T_FLOAT && origen == T_INT);
}

static int es_cero_literal(Nodo *n) {
    return (n->clase == N_LIT_INT && n->ival == 0) || (n->clase == N_LIT_FLOAT && n->fval == 0.0);
}

/* Calcula el tipo de una expresion y lo guarda en el nodo */
static TipoDato revisar_expr(Nodo *e) {
    switch (e->clase) {
    case N_LIT_INT:   return e->tipo = T_INT;
    case N_LIT_FLOAT: return e->tipo = T_FLOAT;
    case N_LIT_STR:   return e->tipo = T_STR;
    case N_LIT_BOOL:  return e->tipo = T_BOOL;

    case N_ID: {
        Simbolo *s = ts_buscar(e->nombre);
        if (!s) {
            error_sem(e->linea, "la variable '%s' no ha sido declarada", e->nombre);
            return e->tipo = T_ERROR;
        }
        s->usos++;
        if (!s->inicializada)
            advertencia(e->linea, "la variable '%s' se usa sin haber sido inicializada", e->nombre);
        return e->tipo = s->tipo;
    }

    case N_OP_UNARIA: {
        TipoDato t = revisar_expr(e->a);
        if (t == T_ERROR) return e->tipo = T_ERROR;
        if (es_op(e, "-")) {
            if (es_numerico(t)) return e->tipo = t;
            error_sem(e->linea, "el operador '-' unario requiere int o float, se recibio %s", nombre_tipo(t));
        } else { /* ! */
            if (t == T_BOOL) return e->tipo = T_BOOL;
            error_sem(e->linea, "el operador '!' requiere bool, se recibio %s", nombre_tipo(t));
        }
        return e->tipo = T_ERROR;
    }

    case N_OP_BINARIA: {
        TipoDato i = revisar_expr(e->a);
        TipoDato d = revisar_expr(e->b);
        if (i == T_ERROR || d == T_ERROR) return e->tipo = T_ERROR;
        const char *op = e->nombre;

        /* Aritmeticos: + - * / */
        if (es_op(e, "+") || es_op(e, "-") || es_op(e, "*") || es_op(e, "/")) {
            if (es_op(e, "/") && es_cero_literal(e->b)) {
                error_sem(e->linea, "division entre cero");
                return e->tipo = T_ERROR;
            }
            if (es_numerico(i) && es_numerico(d))
                return e->tipo = (i == T_INT && d == T_INT) ? T_INT : T_FLOAT;
            if (es_op(e, "+") && i == T_STR && d == T_STR)
                return e->tipo = T_STR;          /* concatenacion */
        }
        /* Relacionales: < > <= >= */
        else if (es_op(e, "<") || es_op(e, ">") || es_op(e, "<=") || es_op(e, ">=")) {
            if (es_numerico(i) && es_numerico(d)) return e->tipo = T_BOOL;
        }
        /* Igualdad: == != */
        else if (es_op(e, "==") || es_op(e, "!=")) {
            if (i == d || (es_numerico(i) && es_numerico(d))) return e->tipo = T_BOOL;
        }
        /* Logicos: && || */
        else if (es_op(e, "&&") || es_op(e, "||")) {
            if (i == T_BOOL && d == T_BOOL) return e->tipo = T_BOOL;
        }
        error_sem(e->linea, "el operador '%s' no se puede aplicar a los tipos %s y %s",
                  op, nombre_tipo(i), nombre_tipo(d));
        return e->tipo = T_ERROR;
    }
    default: return T_ERROR;
    }
}

static void revisar_sentencia(Nodo *s) {
    switch (s->clase) {
    case N_DECLARACION: {
        /* La expresion se revisa ANTES de insertar: var:int x = x; es un error */
        TipoDato t = s->a ? revisar_expr(s->a) : T_NINGUNO;
        Simbolo *previo = ts_buscar(s->nombre);
        if (previo) {
            error_sem(s->linea, "la variable '%s' ya fue declarada en la linea %d",
                      s->nombre, previo->linea);
            return;
        }
        Simbolo *nuevo = ts_insertar(s->nombre, s->tipo, s->linea);
        if (s->a) {
            if (t != T_ERROR && !compatible(s->tipo, t))
                error_sem(s->linea, "no se puede inicializar '%s' (%s) con un valor de tipo %s",
                          s->nombre, nombre_tipo(s->tipo), nombre_tipo(t));
            nuevo->inicializada = 1;
        }
        break;
    }
    case N_ASIGNACION: {
        Simbolo *sim = ts_buscar(s->nombre);
        TipoDato t = revisar_expr(s->a);
        if (!sim) {
            error_sem(s->linea, "no se puede asignar a '%s': la variable no ha sido declarada", s->nombre);
            s->tipo = T_ERROR;
            return;
        }
        s->tipo = sim->tipo;
        if (t != T_ERROR && !compatible(sim->tipo, t))
            error_sem(s->linea, "no se puede asignar un valor de tipo %s a '%s' que es de tipo %s",
                      nombre_tipo(t), s->nombre, nombre_tipo(sim->tipo));
        sim->inicializada = 1;
        break;
    }
    case N_IMPRIMIR:
        revisar_expr(s->a);
        break;
    default: break;
    }
}

int analizar_semantica(Nodo *programa, int *adv) {
    errores = advertencias = 0;
    ts_iniciar();
    for (Nodo *s = programa->a; s; s = s->sig) revisar_sentencia(s);

    /* Variables declaradas que nunca se leyeron */
    for (int i = 0; i < ts_cantidad(); i++) {
        Simbolo *s = ts_obtener(i);
        if (s->usos == 0)
            advertencia(s->linea, "la variable '%s' se declaro pero nunca se usa", s->nombre);
    }
    if (adv) *adv = advertencias;
    return errores;
}
