/* ============================================================
 * semantico.c - Analizador Semantico
 *  - Llena la Tabla de Simbolos
 *  - Verifica declaracion antes de uso y no redeclaracion
 *  - Verifica compatibilidad de tipos (decora el AST con tipos)
 *  - Detecta division entre cero literal
 *  - Advierte variables sin inicializar o nunca usadas
 *  - Funciones: ambitos, parametros, llamadas y return
 *  - IF: la condicion debe ser bool; cada bloque tiene su ambito
 *  - Arreglos: elementos del mismo tipo, indice int, metodos
 *    push / pop / length
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

/* Regla de compatibilidad: mismo tipo, o int -> float (promocion).
   El arreglo vacio [] sirve para cualquier tipo de arreglo. */
static int compatible(TipoDato destino, TipoDato origen) {
    return destino == origen || (destino == T_FLOAT && origen == T_INT)
        || (es_arreglo(destino) && origen == T_ARR_VACIO);
}

/* Un literal [ ... ] toma el tipo del lugar donde se guarda:
   var:float[] notas = [10, 9];  -> el literal pasa a ser float[]
   var:str[] vacia = [];         -> el literal pasa a ser str[]        */
static void ajustar_literal(Nodo *e, TipoDato destino) {
    if (e->clase != N_ARREGLO || !es_arreglo(destino) || destino == T_ARR_VACIO) return;
    if (e->tipo == T_ARR_VACIO || (destino == T_FLOAT_ARR && e->tipo == T_INT_ARR))
        e->tipo = destino;
}

static int es_cero_literal(Nodo *n) {
    return (n->clase == N_LIT_INT && n->ival == 0) || (n->clase == N_LIT_FLOAT && n->fval == 0.0);
}

/* Funcion que se esta analizando (NULL = estamos en el programa principal) */
static Simbolo *funcion_actual = NULL;
static int bloques_abiertos = 0;   /* cuantos bloques if/else rodean a la sentencia actual */

static TipoDato revisar_expr(Nodo *e);

/* ---------- Arreglos (semana 9) ---------- */

/* Busca la variable de un arreglo para lista[i] o lista.metodo() */
static Simbolo *buscar_arreglo(Nodo *e) {
    Simbolo *s = ts_buscar(e->nombre);
    if (!s) {
        error_sem(e->linea, "el arreglo '%s' no ha sido declarado", e->nombre);
        return NULL;
    }
    if (s->categoria != S_FUNCION) s->usos++;     /* cuenta como uso aunque luego haya error */
    if (s->categoria == S_FUNCION || !es_arreglo(s->tipo)) {
        error_sem(e->linea, "'%s' no es un arreglo (es %s de tipo %s)", e->nombre,
                  nombre_categoria(s->categoria), nombre_tipo(s->tipo));
        return NULL;
    }
    e->sim = s;
    return s;
}

/* [e1, e2, ...]: todos los elementos deben ser del mismo tipo basico */
static TipoDato revisar_arreglo(Nodo *e) {
    TipoDato elem = T_NINGUNO;
    int hay_error = 0, pos = 0;
    for (Nodo *x = e->a; x; x = x->sig) {
        TipoDato t = revisar_expr(x);
        pos++;
        if (t == T_ERROR) { hay_error = 1; continue; }
        if (es_arreglo(t) || t == T_VOID) {
            error_sem(e->linea, "el elemento %d del arreglo es de tipo %s; "
                      "solo se permiten int, float, str o bool", pos, nombre_tipo(t));
            hay_error = 1; continue;
        }
        if (elem == T_NINGUNO || elem == t) elem = t;
        else if ((elem == T_INT && t == T_FLOAT) || (elem == T_FLOAT && t == T_INT)) elem = T_FLOAT;
        else {
            error_sem(e->linea, "todos los elementos del arreglo deben ser del mismo tipo "
                      "(el elemento %d es %s y los anteriores son %s)", pos, nombre_tipo(t), nombre_tipo(elem));
            hay_error = 1;
        }
    }
    if (hay_error) return e->tipo = T_ERROR;
    return e->tipo = (elem == T_NINGUNO) ? T_ARR_VACIO : tipo_arreglo(elem);
}

/* lista[i]: el indice debe ser int; el resultado es del tipo de los elementos */
static TipoDato revisar_indice(Nodo *e) {
    TipoDato ti = revisar_expr(e->a);
    Simbolo *s = buscar_arreglo(e);
    if (ti != T_ERROR && ti != T_INT)
        error_sem(e->linea, "el indice de '%s' debe ser int, pero es de tipo %s", e->nombre, nombre_tipo(ti));
    if (!s || (ti != T_ERROR && ti != T_INT)) return e->tipo = T_ERROR;
    return e->tipo = tipo_elemento(s->tipo);
}

/* lista.push(x)  lista.pop()  lista.length() */
static TipoDato revisar_metodo(Nodo *e) {
    int n_args = 0;
    for (Nodo *a = e->a; a; a = a->sig) { revisar_expr(a); n_args++; }
    Simbolo *s = buscar_arreglo(e);
    if (!s) return e->tipo = T_ERROR;
    TipoDato elem = tipo_elemento(s->tipo);
    const char *m = e->sval;

    int esperados;
    TipoDato resultado;
    if      (strcmp(m, "push") == 0)   { esperados = 1; resultado = T_VOID; }
    else if (strcmp(m, "pop") == 0)    { esperados = 0; resultado = elem;   }
    else if (strcmp(m, "length") == 0) { esperados = 0; resultado = T_INT;  }
    else {
        error_sem(e->linea, "los arreglos no tienen el metodo '%s' (use push, pop o length)", m);
        return e->tipo = T_ERROR;
    }
    if (n_args != esperados) {
        error_sem(e->linea, "el metodo '%s' espera %d argumento(s), pero recibio %d", m, esperados, n_args);
        return e->tipo = T_ERROR;
    }
    if (esperados == 1 && e->a->tipo != T_ERROR && !compatible(elem, e->a->tipo))
        error_sem(e->linea, "no se puede agregar un valor %s al arreglo '%s' que es %s",
                  nombre_tipo(e->a->tipo), e->nombre, nombre_tipo(s->tipo));
    return e->tipo = resultado;
}

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
        ajustar_literal(a, p->tipo);
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
    case N_ARREGLO:   return revisar_arreglo(e);
    case N_INDICE:    return revisar_indice(e);
    case N_METODO:    return revisar_metodo(e);

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
            if (i != T_VOID && !es_arreglo(i) && (i == d || (es_numerico(i) && es_numerico(d))))
                return e->tipo = T_BOOL;
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
            ajustar_literal(s->a, s->tipo);
            t = s->a->tipo;
            if (t != T_ERROR && !compatible(s->tipo, t))
                error_sem(s->linea, "no se puede inicializar '%s' (%s) con un valor de tipo %s",
                          s->nombre, nombre_tipo(s->tipo), nombre_tipo(t));
            nuevo->inicializada = 1;
        }
        if (es_arreglo(s->tipo)) nuevo->inicializada = 1;   /* var:int[] v;  empieza como [] */
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
        ajustar_literal(s->a, sim->tipo);
        t = s->a->tipo;
        if (t != T_ERROR && !compatible(sim->tipo, t))
            error_sem(s->linea, "no se puede asignar un valor de tipo %s a '%s' que es de tipo %s",
                      nombre_tipo(t), s->nombre, nombre_tipo(sim->tipo));
        sim->inicializada = 1;
        break;
    }
    case N_IMPRIMIR: {
        TipoDato t = revisar_expr(s->a);
        if (t == T_VOID && s->a->clase == N_METODO)
            error_sem(s->linea, "no se puede imprimir '%s.%s(...)' porque no devuelve un valor",
                      s->a->nombre, s->a->sval);
        else if (t == T_VOID)
            error_sem(s->linea, "no se puede imprimir el resultado de '%s' porque es una funcion void",
                      s->a->nombre);
        break;
    }
    case N_LLAMADA:            /* llamada usada como sentencia: f(x); */
        revisar_llamada(s);
        break;
    case N_METODO:             /* lista.push(x);  lista.pop(); */
        revisar_metodo(s);
        break;
    case N_RETORNO: {
        TipoDato t = s->a ? revisar_expr(s->a) : T_VOID;
        if (!funcion_actual) {
            error_sem(s->linea, "'return' solo se puede usar dentro de una funcion");
            return;
        }
        TipoDato esperado = funcion_actual->tipo;
        s->tipo = esperado;
        if (s->a) { ajustar_literal(s->a, esperado); t = s->a->tipo; }
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
