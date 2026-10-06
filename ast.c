/* ============================================================
 * ast.c - Construccion e impresion del AST
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

static Nodo *nuevo(ClaseNodo c, int linea) {
    Nodo *n = calloc(1, sizeof(Nodo));
    n->clase = c;
    n->linea = linea;
    n->tipo  = T_NINGUNO;
    return n;
}

Nodo *ast_programa(Nodo *sentencias) { Nodo *n = nuevo(N_PROGRAMA, 1); n->a = sentencias; return n; }

Nodo *ast_agregar(Nodo *lista, Nodo *s) {
    if (!s) return lista;          /* sentencia descartada por error */
    if (!lista) return s;
    Nodo *p = lista;
    while (p->sig) p = p->sig;
    p->sig = s;
    return lista;
}

Nodo *ast_declaracion(TipoDato t, char *nombre, Nodo *init, int linea) {
    Nodo *n = nuevo(N_DECLARACION, linea);
    n->tipo = t; n->nombre = nombre; n->a = init; return n;
}
Nodo *ast_asignacion(char *nombre, Nodo *e, int linea) {
    Nodo *n = nuevo(N_ASIGNACION, linea); n->nombre = nombre; n->a = e; return n;
}
Nodo *ast_imprimir(Nodo *e, int linea) { Nodo *n = nuevo(N_IMPRIMIR, linea); n->a = e; return n; }
Nodo *ast_int(long long v, int linea)  { Nodo *n = nuevo(N_LIT_INT, linea);   n->ival = v; return n; }
Nodo *ast_float(double v, int linea)   { Nodo *n = nuevo(N_LIT_FLOAT, linea); n->fval = v; return n; }
Nodo *ast_str(char *v, int linea)      { Nodo *n = nuevo(N_LIT_STR, linea);   n->sval = v; return n; }
Nodo *ast_bool(int v, int linea)       { Nodo *n = nuevo(N_LIT_BOOL, linea);  n->ival = v; return n; }
Nodo *ast_id(char *nombre, int linea)  { Nodo *n = nuevo(N_ID, linea); n->nombre = nombre; return n; }
Nodo *ast_binaria(const char *op, Nodo *i, Nodo *d, int linea) {
    Nodo *n = nuevo(N_OP_BINARIA, linea); n->nombre = strdup(op); n->a = i; n->b = d; return n;
}
Nodo *ast_unaria(const char *op, Nodo *e, int linea) {
    Nodo *n = nuevo(N_OP_UNARIA, linea); n->nombre = strdup(op); n->a = e; return n;
}

const char *nombre_tipo(TipoDato t) {
    switch (t) {
        case T_INT: return "int";   case T_FLOAT: return "float";
        case T_STR: return "str";   case T_BOOL:  return "bool";
        case T_ERROR: return "ERROR"; default: return "-";
    }
}

const char *nombre_clase(ClaseNodo c) {
    static const char *n[] = { "PROGRAMA", "DECLARACION", "ASIGNACION", "IMPRIMIR",
        "LIT_INT", "LIT_FLOAT", "LIT_STR", "LIT_BOOL", "IDENTIFICADOR",
        "OP_BINARIA", "OP_UNARIA" };
    return n[c];
}

int ast_contar_sentencias(Nodo *p) {
    int c = 0;
    for (Nodo *s = p ? p->a : NULL; s; s = s->sig) c++;
    return c;
}

/* Texto que describe a cada nodo */
static void detalle(Nodo *n, char *buf, size_t tam) {
    switch (n->clase) {
        case N_PROGRAMA:    snprintf(buf, tam, "%d sentencias", ast_contar_sentencias(n)); break;
        case N_DECLARACION: snprintf(buf, tam, "var:%s %s", nombre_tipo(n->tipo), n->nombre); break;
        case N_ASIGNACION:  snprintf(buf, tam, "%s =", n->nombre); break;
        case N_IMPRIMIR:    snprintf(buf, tam, "print"); break;
        case N_LIT_INT:     snprintf(buf, tam, "%lld", n->ival); break;
        case N_LIT_FLOAT:   if (n->fval == (long long)n->fval) snprintf(buf, tam, "%.1f", n->fval);
                            else snprintf(buf, tam, "%g", n->fval);
                            break;
        case N_LIT_STR:     snprintf(buf, tam, "\"%s\"", n->sval); break;
        case N_LIT_BOOL:    snprintf(buf, tam, "%s", n->ival ? "true" : "false"); break;
        case N_ID:          snprintf(buf, tam, "%s", n->nombre); break;
        default:            snprintf(buf, tam, "%s", n->nombre); break;
    }
}

/* Devuelve los hijos de un nodo en el arreglo h (max 512) */
static int hijos(Nodo *n, Nodo **h) {
    int k = 0;
    if (n->clase == N_PROGRAMA) {
        for (Nodo *s = n->a; s && k < 512; s = s->sig) h[k++] = s;
    } else {
        if (n->a) h[k++] = n->a;
        if (n->b) h[k++] = n->b;
    }
    return k;
}

/* ---------- Arbol en forma grafica ---------- */
static void arbol_rec(Nodo *n, const char *prefijo, int ultimo, int raiz) {
    char d[256];
    detalle(n, d, sizeof d);
    if (raiz) printf("%s  [%s]\n", nombre_clase(n->clase), d);
    else      printf("%s%s%s  [%s]  (linea %d)\n", prefijo, ultimo ? "`-- " : "|-- ",
                     nombre_clase(n->clase), d, n->linea);

    char nuevo_pref[1024];
    snprintf(nuevo_pref, sizeof nuevo_pref, "%s%s", prefijo, raiz ? "" : (ultimo ? "    " : "|   "));
    Nodo *h[512];
    int k = hijos(n, h);
    for (int i = 0; i < k; i++) arbol_rec(h[i], nuevo_pref, i == k - 1, 0);
}

void ast_imprimir_arbol(Nodo *raiz) { if (raiz) arbol_rec(raiz, "", 1, 1); }

/* ---------- Tabla AST ---------- */
static int contador;
static void numerar(Nodo *n) {
    n->id = ++contador;
    Nodo *h[512];
    int k = hijos(n, h);
    for (int i = 0; i < k; i++) numerar(h[i]);
}

static void fila(Nodo *n) {
    char d[256], hs[256] = "";
    detalle(n, d, sizeof d);
    Nodo *h[512];
    int k = hijos(n, h);
    for (int i = 0; i < k; i++) {
        char t[16];
        snprintf(t, sizeof t, i ? ",%d" : "%d", h[i]->id);
        if (strlen(hs) + strlen(t) < sizeof hs - 4) strcat(hs, t);
        else { strcat(hs, "..."); break; }
    }
    if (!k) strcpy(hs, "-");
    if (strlen(hs) > 14) strcpy(hs + 11, "...");   /* cabe en la columna */
    printf("| %-4d | %-14s | %-22.22s | %-6s | %-14.14s | %-5d |\n",
           n->id, nombre_clase(n->clase), d, nombre_tipo(n->tipo), hs, n->linea);
    for (int i = 0; i < k; i++) fila(h[i]);
}

void ast_imprimir_tabla(Nodo *raiz) {
    if (!raiz) return;
    contador = 0;
    numerar(raiz);
    const char *sep = "+------+----------------+------------------------+--------+----------------+-------+\n";
    printf("%s", sep);
    printf("| %-4s | %-14s | %-22s | %-6s | %-14s | %-5s |\n", "ID", "NODO", "VALOR / DETALLE", "TIPO", "HIJOS", "LINEA");
    printf("%s", sep);
    fila(raiz);
    printf("%s", sep);
}
