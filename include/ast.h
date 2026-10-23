/* ============================================================
 * ast.h - Arbol de Sintaxis Abstracta (AST) de MiLenguaje
 * Mini Proyecto 1 - IS-505 - Moises Moncada
 * ============================================================ */
#ifndef AST_H
#define AST_H

/* Tipos de dato del lenguaje.
   Arreglos (semana 9): cada tipo basico tiene su arreglo: int[], float[], str[], bool[].
   T_ARR_VACIO es el tipo del literal [] (sirve para cualquier arreglo). */
typedef enum {
    T_INT, T_FLOAT, T_STR, T_BOOL, T_VOID, T_NINGUNO, T_ERROR,
    T_INT_ARR, T_FLOAT_ARR, T_STR_ARR, T_BOOL_ARR, T_ARR_VACIO
} TipoDato;

/* Clases de nodo del AST */
typedef enum {
    N_PROGRAMA,     /* raiz: lista de sentencias          */
    N_DECLARACION,  /* var:<tipo> id [= expr];            */
    N_ASIGNACION,   /* id = expr;                         */
    N_IMPRIMIR,     /* print(expr);                       */
    N_LIT_INT,      /* 25                                 */
    N_LIT_FLOAT,    /* 3.75                               */
    N_LIT_STR,      /* "Moises"                           */
    N_LIT_BOOL,     /* true / false                       */
    N_ID,           /* uso de una variable                */
    N_OP_BINARIA,   /* a + b, a < b, a && b ...           */
    N_OP_UNARIA,    /* -a, !a                             */
    N_FUNCION,      /* func f(a:int):int { ... }          */
    N_PARAMETRO,    /* a:int  (dentro de una funcion)     */
    N_RETORNO,      /* return expr;  /  return;           */
    N_LLAMADA,      /* f(x, y)  (expresion o sentencia)   */
    N_SI,           /* if cond : { ... } else { ... }     */
    N_BLOQUE,       /* { sentencias }  (bloque del if/else) */
    N_ARREGLO,      /* [1, 2, 3]   literal de arreglo     */
    N_INDICE,       /* lista[i]    acceso a un elemento   */
    N_METODO        /* lista.push(x) / .pop() / .length() */
} ClaseNodo;

typedef struct Nodo {
    ClaseNodo clase;
    int id;              /* numero de nodo (para la Tabla AST)      */
    int linea;           /* linea del codigo fuente                 */
    TipoDato tipo;       /* tipo declarado o calculado (semantico)  */
    char *nombre;        /* identificador u operador                */
    long long ival;      /* valor de literal int / bool             */
    double fval;         /* valor de literal float                  */
    char *sval;          /* valor de literal str                    */
    struct Nodo *a, *b;  /* hijos                                   */
                         /* FUNCION: a = parametros, b = cuerpo     */
                         /* LLAMADA: a = argumentos                 */
                         /* SI: a = condicion, b = bloque if,       */
                         /*     c = bloque else (o NULL)            */
                         /* BLOQUE: a = lista de sentencias         */
                         /* ARREGLO: a = lista de elementos         */
                         /* INDICE: nombre = arreglo, a = indice    */
                         /* METODO: nombre = arreglo, sval = metodo,*/
                         /*         a = argumentos                  */
    struct Nodo *c;      /* tercer hijo (solo el else del IF)       */
    struct Nodo *sig;    /* siguiente elemento (sentencias, params, */
                         /* argumentos)                             */
    void *sim;           /* simbolo resuelto por el semantico       */
} Nodo;

/* Constructores (los llama el parser) */
Nodo *ast_programa(Nodo *sentencias);
Nodo *ast_agregar(Nodo *lista, Nodo *sentencia);
Nodo *ast_declaracion(TipoDato t, char *nombre, Nodo *init, int linea);
Nodo *ast_asignacion(char *nombre, Nodo *expr, int linea);
Nodo *ast_imprimir(Nodo *expr, int linea);
Nodo *ast_int(long long v, int linea);
Nodo *ast_float(double v, int linea);
Nodo *ast_str(char *v, int linea);
Nodo *ast_bool(int v, int linea);
Nodo *ast_id(char *nombre, int linea);
Nodo *ast_binaria(const char *op, Nodo *izq, Nodo *der, int linea);
Nodo *ast_unaria(const char *op, Nodo *e, int linea);
Nodo *ast_funcion(char *nombre, Nodo *params, TipoDato ret, Nodo *cuerpo, int linea);
Nodo *ast_parametro(char *nombre, TipoDato t, int linea);
Nodo *ast_retorno(Nodo *expr, int linea);
Nodo *ast_llamada(char *nombre, Nodo *args, int linea);
Nodo *ast_si(Nodo *cond, Nodo *bloque_si, Nodo *bloque_sino, int linea);
Nodo *ast_bloque(const char *etiqueta, Nodo *sentencias, int linea);
Nodo *ast_arreglo(Nodo *elementos, int linea);
Nodo *ast_indice(char *nombre, Nodo *indice, int linea);
Nodo *ast_metodo(char *nombre, char *metodo, Nodo *args, int linea);
int  ast_contar_lista(Nodo *lista);

/* Utilidades */
const char *nombre_tipo(TipoDato t);
int      es_arreglo(TipoDato t);          /* 1 si t es int[], float[], str[], bool[] o [] */
TipoDato tipo_elemento(TipoDato arr);     /* int[] -> int                                */
TipoDato tipo_arreglo(TipoDato elem);     /* int   -> int[]                              */
const char *nombre_clase(ClaseNodo c);
int  ast_contar_sentencias(Nodo *programa);
void ast_imprimir_arbol(Nodo *raiz);
void ast_imprimir_tabla(Nodo *raiz);

#endif
