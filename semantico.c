/* ============================================================
 * semantico.c - Analizador Semantico
 *  - Llena la Tabla de Simbolos
 *  - Verifica declaracion antes de uso y no redeclaracion
 *  - Verifica compatibilidad de tipos (decora el AST con tipos)
 *  - Detecta division entre cero literal
 *  - Advierte variables sin inicializar o nunca usadas
 *  - Funciones: ambitos, parametros, llamadas y return
 *  - IF: la condicion debe ser bool; cada bloque tiene su ambito
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

/* Funcion que se esta analizando (NULL = estamos en el programa principal) */
static Simbolo *funcion_actual = NULL;
static int bloques_abiertos = 0;   /* cuantos bloques if/else rodean a la sentencia actual */

static TipoDato revisar_expr(Nodo *e);

/* ---------- Llamada a funcion: f(arg1, arg2, ...) ---------- */
static TipoDato revisar_llamada(Nodo *e) {
    Simbolo *f = ts_buscar(e->nombre);
    /* Los argumentos se revisan siempre, para reportar sus errores */
    int n_args = 0;
    for (Nodo *a = e->a; a; a = a->sig) { revisar_expr(a); n_args++; }

    if (!f) {
        error_sem(e->linea, "la funcion '%s' no ha sido declarada", e->nombre);
        return e->tipo = T_ERROR;
    }
    if (f->categoria != S_FUNCION) {
        error_sem(e->linea, "'%s' es una %s, no una funcion", e->nombre, nombre_categoria(f->categoria));
        return e->tipo = T_ERROR;
    }
    e->sim = f;
    f->usos++;
    if (n_args != f->num_params) {
        error_sem(e->linea, "la funcion '%s' espera %d argumento(s), pero recibio %d",
                  e->nombre, f->num_params, n_args);
        return e->tipo = f->tipo;
    }
    /* Cada argumento debe ser compatible con su parametro */
    Nodo *p = f->funcion->a, *a = e->a;
    for (int i = 1; p && a; p = p->sig, a = a->sig, i++) {
        if (a->tipo != T_ERROR && !compatible(p->tipo, a->tipo))
            error_sem(e->linea, "argumento %d de '%s': se esperaba %s ('%s') y se recibio %s",
                      i, e->nombre, nombre_tipo(p->tipo), p->nombre, nombre_tipo(a->tipo));
    }
    return e->tipo = f->tipo;
}

/* Calcula el tipo de una expresion y lo guarda en el nodo */
static TipoDato revisar_expr(Nodo *e) {
    switch (e->clase) {
    case N_LIT_INT:   return e->tipo = T_INT;
    case N_LIT_FLOAT: return e->tipo = T_FLOAT;
    case N_LIT_STR:   return e->tipo = T_STR;
    case N_LIT_BOOL:  return e->tipo = T_BOOL;
    case N_LLAMADA:   return revisar_llamada(e);

    case N_ID: {
        Simbolo *s = ts_buscar(e->nombre);
        if (!s) {
            error_sem(e->linea, "la variable '%s' no ha sido declarada", e->nombre);
            return e->tipo = T_ERROR;
        }
        if (s->categoria == S_FUNCION) {
            error_sem(e->linea, "'%s' es una funcion; para usarla hay que llamarla: %s(...)",
                      e->nombre, e->nombre);
            return e->tipo = T_ERROR;
        }
        e->sim = s;
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
            if (i != T_VOID && (i == d || (es_numerico(i) && es_numerico(d)))) return e->tipo = T_BOOL;
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

/* Inserta una variable o parametro en el ambito actual.
   Si estamos dentro de una funcion, le reserva un lugar en su marco. */
static Simbolo *declarar(const char *nombre, TipoDato t, Categoria cat, int linea) {
    Simbolo *previo = ts_buscar_en_ambito(nombre, ts_ambito_actual());
    if (previo) {
        error_sem(linea, "'%s' ya fue declarado en la linea %d (ambito %s)",
                  nombre, previo->linea, previo->ambito);
        return NULL;
    }
    Simbolo *s = ts_insertar(nombre, t, cat, linea);
    if (!s) return NULL;
    /* Solo lo que vive dentro de una funcion va en su marco; las variables de un
       bloque if del programa principal se guardan como las globales */
    s->es_local = funcion_actual != NULL;
    if (s->es_local) s->slot = funcion_actual->num_locales++;
    return s;
}

static void revisar_funcion(Nodo *f);
static void revisar_sentencia(Nodo *s);

/* Revisa un bloque { ... } del if/else dentro de su propio ambito */
static void revisar_bloque(Nodo *bloque, int linea_if) {
    char nombre[64];
    if (funcion_actual)
        snprintf(nombre, sizeof nombre, "%s>%s%d", funcion_actual->nombre, bloque->nombre, linea_if);
    else
        snprintf(nombre, sizeof nombre, "%s:%d", bloque->nombre, linea_if);
    ts_entrar_ambito(nombre);
    bloques_abiertos++;
    for (Nodo *s = bloque->a; s; s = s->sig) revisar_sentencia(s);
    bloques_abiertos--;
    ts_salir_ambito();
}

/* 1 si la sentencia termina SIEMPRE con return (por todos los caminos) */
static int siempre_retorna(Nodo *s);
static int lista_siempre_retorna(Nodo *l) {
    for (; l; l = l->sig) if (siempre_retorna(l)) return 1;
    return 0;
}
static int siempre_retorna(Nodo *s) {
    if (s->clase == N_RETORNO) return 1;
    if (s->clase == N_SI)      /* solo si hay else y ambos caminos devuelven */
        return s->c && lista_siempre_retorna(s->b->a) && lista_siempre_retorna(s->c->a);
    return 0;
}
static int tiene_algun_return(Nodo *l) {
    for (; l; l = l->sig) {
        if (l->clase == N_RETORNO) return 1;
        if (l->clase == N_SI && (tiene_algun_return(l->b->a) || (l->c && tiene_algun_return(l->c->a))))
            return 1;
    }
    return 0;
}

static void revisar_sentencia(Nodo *s) {
    switch (s->clase) {
    case N_DECLARACION: {
        /* La expresion se revisa ANTES de insertar: var:int x = x; es un error */
        TipoDato t = s->a ? revisar_expr(s->a) : T_NINGUNO;
        Simbolo *nuevo = declarar(s->nombre, s->tipo, S_VARIABLE, s->linea);
        if (!nuevo) return;
        s->sim = nuevo;
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
        if (sim->categoria == S_FUNCION) {
            error_sem(s->linea, "no se puede asignar un valor a '%s' porque es una funcion", s->nombre);
            return;
        }
        s->sim = sim;
        s->tipo = sim->tipo;
        if (t != T_ERROR && !compatible(sim->tipo, t))
            error_sem(s->linea, "no se puede asignar un valor de tipo %s a '%s' que es de tipo %s",
                      nombre_tipo(t), s->nombre, nombre_tipo(sim->tipo));
        sim->inicializada = 1;
        break;
    }
    case N_IMPRIMIR: {
        TipoDato t = revisar_expr(s->a);
        if (t == T_VOID)
            error_sem(s->linea, "no se puede imprimir el resultado de '%s' porque es una funcion void",
                      s->a->nombre);
        break;
    }
    case N_LLAMADA:            /* llamada usada como sentencia: f(x); */
        revisar_llamada(s);
        break;
    case N_RETORNO: {
        TipoDato t = s->a ? revisar_expr(s->a) : T_VOID;
        if (!funcion_actual) {
            error_sem(s->linea, "'return' solo se puede usar dentro de una funcion");
            return;
        }
        TipoDato esperado = funcion_actual->tipo;
        s->tipo = esperado;
        if (esperado == T_VOID && s->a)
            error_sem(s->linea, "la funcion '%s' es void y no puede devolver un valor",
                      funcion_actual->nombre);
        else if (esperado != T_VOID && !s->a)
            error_sem(s->linea, "la funcion '%s' debe devolver un valor de tipo %s",
                      funcion_actual->nombre, nombre_tipo(esperado));
        else if (esperado != T_VOID && t != T_ERROR && !compatible(esperado, t))
            error_sem(s->linea, "la funcion '%s' debe devolver %s, pero devuelve %s",
                      funcion_actual->nombre, nombre_tipo(esperado), nombre_tipo(t));
        break;
    }
    case N_FUNCION:
        if (bloques_abiertos) {
            error_sem(s->linea, "no se puede declarar la funcion '%s' dentro de un bloque if/else", s->nombre);
            return;
        }
        revisar_funcion(s);
        break;
    case N_SI: {
        /* La condicion del if debe ser de tipo bool */
        TipoDato t = revisar_expr(s->a);
        s->tipo = T_BOOL;
        if (t != T_ERROR && t != T_BOOL)
            error_sem(s->linea, "la condicion del 'if' debe ser bool, pero es de tipo %s", nombre_tipo(t));
        revisar_bloque(s->b, s->linea);
        if (s->c) revisar_bloque(s->c, s->linea);
        break;
    }
    default: break;
    }
}

/* ---------- Declaracion de funcion ---------- */
static void revisar_funcion(Nodo *f) {
    if (funcion_actual) {
        error_sem(f->linea, "no se puede declarar la funcion '%s' dentro de otra funcion", f->nombre);
        return;
    }
    /* 1. Registrar la funcion en el ambito global (antes del cuerpo: permite recursion) */
    Simbolo *previo = ts_buscar_en_ambito(f->nombre, "global");
    if (previo) {
        error_sem(f->linea, "el nombre '%s' ya fue declarado en la linea %d", f->nombre, previo->linea);
        return;
    }
    Simbolo *fs = ts_insertar(f->nombre, f->tipo, S_FUNCION, f->linea);
    fs->funcion = f;
    fs->num_params = ast_contar_lista(f->a);
    fs->inicializada = 1;
    f->sim = fs;

    /* 2. Entrar al ambito de la funcion y declarar los parametros */
    funcion_actual = fs;
    ts_entrar_ambito(f->nombre);
    for (Nodo *p = f->a; p; p = p->sig) {
        Simbolo *ps = declarar(p->nombre, p->tipo, S_PARAMETRO, p->linea);
        if (ps) { ps->inicializada = 1; p->sim = ps; }
    }

    /* 3. Revisar el cuerpo */
    int tiene_return = 0;
    for (Nodo *s = f->b; s; s = s->sig) {
        if (tiene_return)
            advertencia(s->linea, "este codigo esta despues de 'return' y nunca se ejecuta");
        revisar_sentencia(s);
        if (siempre_retorna(s)) tiene_return = 1;   /* return, o if/else que devuelve en ambos caminos */
    }
    if (f->tipo != T_VOID && !tiene_return) {
        if (tiene_algun_return(f->b))
            error_sem(f->linea, "la funcion '%s' no devuelve un valor en todos los caminos "
                      "(falta un 'else' con 'return')", f->nombre);
        else
            error_sem(f->linea, "la funcion '%s' debe devolver un valor de tipo %s, pero no tiene 'return'",
                      f->nombre, nombre_tipo(f->tipo));
    }

    /* 4. Salir del ambito de la funcion */
    ts_salir_ambito();
    funcion_actual = NULL;
}

int analizar_semantica(Nodo *programa, int *adv) {
    errores = advertencias = 0;
    funcion_actual = NULL;
    bloques_abiertos = 0;
    ts_iniciar();
    for (Nodo *s = programa->a; s; s = s->sig) revisar_sentencia(s);

    /* Simbolos declarados que nunca se usaron */
    for (int i = 0; i < ts_cantidad(); i++) {
        Simbolo *s = ts_obtener(i);
        if (s->usos > 0) continue;
        if (s->categoria == S_FUNCION)
            advertencia(s->linea, "la funcion '%s' se declaro pero nunca se llama", s->nombre);
        else if (s->categoria == S_PARAMETRO)
            advertencia(s->linea, "el parametro '%s' de '%s' nunca se usa", s->nombre, s->ambito);
        else
            advertencia(s->linea, "la variable '%s' se declaro pero nunca se usa", s->nombre);
    }
    if (adv) *adv = advertencias;
    return errores;
}
