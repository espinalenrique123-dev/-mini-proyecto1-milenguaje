/* ============================================================
 * ejecutor.c - Recorre el AST decorado y ejecuta el programa
 * (solo se llama si no hubo errores lexicos/sintacticos/semanticos)
 *
 * Funciones: cada llamada crea un "marco" (un arreglo de valores)
 * con sus parametros y variables locales. Las variables globales
 * viven en la Tabla de Simbolos. Asi una funcion puede llamarse
 * varias veces (o a si misma) sin mezclar sus variables.
 *
 * Arreglos (semana 9): la variable guarda un puntero al arreglo;
 * los elementos viven en arreglos.c.
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ejecutor.h"
#include "symtab.h"
#include "arreglos.h"

static int error_ejecucion = 0;

#define MAX_PROFUNDIDAD 1000      /* limite de llamadas anidadas */
static Valor *marco_actual = NULL; /* variables locales de la funcion en curso */
static int profundidad = 0;
static int retornando = 0;         /* 1 cuando se ejecuto un return  */
static Valor valor_retorno;

/* Lee o escribe una variable: local (en el marco) o global (en la tabla) */
static Valor leer(Simbolo *s) {
    return s->es_local ? marco_actual[s->slot] : s->valor;
}
static void escribir(Simbolo *s, Valor v) {
    if (s->es_local) marco_actual[s->slot] = v;
    s->valor = v;   /* tambien se guarda el ultimo valor para la tabla final */
}

static void ejecutar_lista(Nodo *lista);

static double como_double(Valor v) { return v.tipo == T_FLOAT ? v.f : (double)v.i; }

/* Convierte un valor al tipo de destino (promocion int -> float) */
static Valor convertir(Valor v, TipoDato destino) {
    if (destino == T_FLOAT && v.tipo == T_INT) { v.f = (double)v.i; v.tipo = T_FLOAT; }
    return v;
}

/* Crea un arreglo vacio del tipo indicado (int[] -> arreglo de int) */
static Valor arreglo_vacio(TipoDato tipo) {
    Valor v = { tipo, 0, 0.0, "", NULL };
    v.arr = arreglo_nuevo(tipo_elemento(tipo));
    if (!v.arr) {
        printf("  [X] Error de ejecucion: no hay memoria para crear el arreglo\n");
        error_ejecucion = 1;
    }
    return v;
}

static Valor evaluar(Nodo *e);

/* Ejecuta lista.push(x) / lista.pop() / lista.length() */
static Valor evaluar_metodo(Nodo *e) {
    Valor r = { e->tipo, 0, 0.0, "", NULL };
    Arreglo *a = leer((Simbolo *)e->sim).arr;
    if (!a) return r;
    if (strcmp(e->sval, "push") == 0) {
        Valor v = convertir(evaluar(e->a), a->tipo_elem);
        if (error_ejecucion) return r;
        if (!arreglo_push(a, v)) {
            printf("  [X] Error de ejecucion (linea %d): no hay memoria para agregar a '%s'\n",
                   e->linea, e->nombre);
            error_ejecucion = 1;
        }
    } else if (strcmp(e->sval, "pop") == 0) {
        if (!arreglo_pop(a, &r)) {
            printf("  [X] Error de ejecucion (linea %d): no se puede hacer pop, "
                   "el arreglo '%s' esta vacio\n", e->linea, e->nombre);
            error_ejecucion = 1;
        }
    } else {                                   /* length */
        r.i = a->largo;
    }
    return r;
}

static Valor evaluar(Nodo *e) {
    Valor r = { e->tipo, 0, 0.0, "", NULL };
    if (error_ejecucion) return r;

    switch (e->clase) {
    case N_LIT_INT:   r.i = e->ival; break;
    case N_LIT_BOOL:  r.i = e->ival; break;
    case N_LIT_FLOAT: r.f = e->fval; break;
    case N_LIT_STR:   r.s = e->sval; break;
    case N_ID:        r = leer((Simbolo *)e->sim); break;

    case N_ARREGLO: {                     /* [e1, e2, ...] crea un arreglo nuevo */
        r = arreglo_vacio(e->tipo);
        for (Nodo *x = e->a; x && !error_ejecucion; x = x->sig) {
            Valor v = convertir(evaluar(x), r.arr->tipo_elem);
            if (!error_ejecucion && !arreglo_push(r.arr, v)) {
                printf("  [X] Error de ejecucion (linea %d): no hay memoria para el arreglo\n", e->linea);
                error_ejecucion = 1;
            }
        }
        break;
    }

    case N_INDICE: {                      /* lista[i] */
        Valor idx = evaluar(e->a);
        if (error_ejecucion) return r;
        Arreglo *a = leer((Simbolo *)e->sim).arr;
        int largo = a ? a->largo : 0;
        if (idx.i < 0 || idx.i >= largo) {
            if (largo == 0)
                printf("  [X] Error de ejecucion (linea %d): el indice %lld no existe, "
                       "el arreglo '%s' esta vacio\n", e->linea, idx.i, e->nombre);
            else
                printf("  [X] Error de ejecucion (linea %d): el indice %lld esta fuera del arreglo '%s' "
                       "(indices validos: 0 a %d)\n", e->linea, idx.i, e->nombre, largo - 1);
            error_ejecucion = 1; return r;
        }
        r = a->items[idx.i];
        break;
    }

    case N_METODO: r = evaluar_metodo(e); break;

    case N_LLAMADA: {
        Simbolo *f = (Simbolo *)e->sim;
        if (profundidad >= MAX_PROFUNDIDAD) {
            printf("  [X] Error de ejecucion (linea %d): demasiadas llamadas anidadas a '%s' "
                   "(recursion infinita?)\n", e->linea, f->nombre);
            error_ejecucion = 1; return r;
        }
        /* 1. Evaluar los argumentos en el marco de quien llama */
        Valor *nuevo = calloc(f->num_locales > 0 ? f->num_locales : 1, sizeof(Valor));
        Nodo *p = f->funcion->a;
        for (Nodo *a = e->a; a && p; a = a->sig, p = p->sig) {
            Valor v = convertir(evaluar(a), p->tipo);
            if (error_ejecucion) { free(nuevo); return r; }
            Simbolo *ps = (Simbolo *)p->sim;
            nuevo[ps->slot] = v;
            ps->valor = v;
        }
        /* 2. Cambiar al marco de la funcion y ejecutar su cuerpo */
        Valor *anterior = marco_actual;
        marco_actual = nuevo;
        profundidad++;
        retornando = 0;
        ejecutar_lista(f->funcion->b);
        /* 3. Recuperar el valor de return y volver al marco anterior */
        if (f->tipo != T_VOID && retornando) r = convertir(valor_retorno, f->tipo);
        retornando = 0;
        profundidad--;
        marco_actual = anterior;
        free(nuevo);
        break;
    }

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

/* Ejecuta una sentencia */
static void ejecutar_sentencia(Nodo *s) {
    switch (s->clase) {
    case N_DECLARACION: {
        Simbolo *sim = (Simbolo *)s->sim;
        Valor v = { s->tipo, 0, 0.0, "", NULL };   /* valor por defecto */
        if (s->a) v = convertir(evaluar(s->a), s->tipo);
        else if (es_arreglo(s->tipo)) v = arreglo_vacio(s->tipo);   /* var:int[] v;  ->  [] */
        if (!error_ejecucion) escribir(sim, v);
        break;
    }
    case N_ASIGNACION: {
        Valor v = convertir(evaluar(s->a), s->tipo);
        if (!error_ejecucion) escribir((Simbolo *)s->sim, v);
        break;
    }
    case N_IMPRIMIR: {
        Valor v = evaluar(s->a);
        if (!error_ejecucion) {
            char buf[1024];
            valor_a_texto(v, buf, sizeof buf);
            printf("  > %s\n", buf);
        }
        break;
    }
    case N_LLAMADA:            /* f(x);  el valor devuelto se descarta */
    case N_METODO:             /* lista.push(x);  lista.pop(); */
        evaluar(s);
        break;
    case N_RETORNO:
        if (s->a) valor_retorno = evaluar(s->a);
        retornando = 1;
        break;
    case N_SI: {               /* if: se evalua la condicion y se corre un solo bloque */
        Valor c = evaluar(s->a);
        if (error_ejecucion) break;
        if (c.i)        ejecutar_lista(s->b->a);   /* condicion true  -> bloque if   */
        else if (s->c)  ejecutar_lista(s->c->a);   /* condicion false -> bloque else */
        break;
    }
    case N_FUNCION:            /* la declaracion no se ejecuta; solo las llamadas */
    default: break;
    }
}

/* Ejecuta una lista de sentencias hasta el final, un return o un error */
static void ejecutar_lista(Nodo *lista) {
    for (Nodo *s = lista; s && !error_ejecucion && !retornando; s = s->sig)
        ejecutar_sentencia(s);
}

int ejecutar(Nodo *programa) {
    error_ejecucion = 0;
    retornando = 0;
    profundidad = 0;
    marco_actual = NULL;
    /* La memoria arranca con valores por defecto: 0, 0.0, "", false */
    for (int i = 0; i < ts_cantidad(); i++) {
        Simbolo *s = ts_obtener(i);
        Valor v = { s->tipo, 0, 0.0, "", NULL };
        s->valor = v;
    }
    ejecutar_lista(programa->a);
    return error_ejecucion;
}
