/* ============================================================
 * symtab.h - Tabla de Simbolos de MiLenguaje
 * Guarda variables, parametros y funciones, cada uno con su
 * ambito: "global" o el nombre de la funcion donde se declaro.
 * ============================================================ */
#ifndef SYMTAB_H
#define SYMTAB_H
#include "ast.h"

/* Valor en memoria de una variable (lo usa la ejecucion) */
typedef struct {
    TipoDato tipo;
    long long i;     /* int / bool */
    double f;        /* float      */
    char *s;         /* str        */
} Valor;

/* Que clase de simbolo es */
typedef enum { S_VARIABLE, S_PARAMETRO, S_FUNCION } Categoria;

typedef struct {
    char *nombre;
    TipoDato tipo;        /* tipo de la variable o tipo de retorno */
    Categoria categoria;
    char *ambito;         /* "global" o el nombre de la funcion    */
    int linea;            /* linea de declaracion                  */
    int inicializada;     /* 1 si recibio valor                    */
    int usos;             /* lecturas (o llamadas, si es funcion)  */
    Valor valor;          /* valor actual (ejecucion)              */

    /* Solo para variables locales y parametros */
    int es_local;         /* 1 si vive dentro de una funcion       */
    int slot;             /* posicion dentro del marco de la funcion */

    /* Solo para funciones */
    Nodo *funcion;        /* nodo FUNCION del AST                  */
    int num_params;
    int num_locales;      /* parametros + variables locales        */
} Simbolo;

void     ts_iniciar(void);
void     ts_entrar_ambito(const char *nombre);   /* entra a una funcion o a un bloque if/else */
void     ts_salir_ambito(void);                  /* vuelve al ambito anterior                 */
const char *ts_ambito_actual(void);
Simbolo *ts_buscar(const char *nombre);          /* ambito actual y luego global */
Simbolo *ts_buscar_en_ambito(const char *nombre, const char *ambito);
Simbolo *ts_insertar(const char *nombre, TipoDato tipo, Categoria cat, int linea);
int      ts_cantidad(void);
Simbolo *ts_obtener(int i);
void     ts_imprimir(int con_valores);
void     valor_a_texto(Valor v, char *buf, int tam);
const char *nombre_categoria(Categoria c);

#endif
