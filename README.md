# MiLenguaje — Mini Proyecto 1

**Autor:** Moisés Moncada · **Curso:** IS-505 Análisis Léxico y Sintáctico · **Profesor:** Wilian Martínez · UNADECA

Compilador (con intérprete) para **MiLenguaje**, construido con **Flex + Bison + C**, siguiendo las reglas definidas en el trabajo *W5 – Mi Lenguaje de Programación*.

## Cómo compilar y ejecutar

```bash
make                                   # compila -> genera ./milenguaje
./milenguaje ejemplos/programa.mil     # ejecuta un programa
./milenguaje -t ejemplos/programa.mil  # igual, mostrando la tabla de tokens
make run       # ejecuta el programa de ejemplo
make tokens    # ejemplo + tabla de tokens
make funciones # ejecuta el ejemplo de funciones (semana 7)
make if        # ejecuta el ejemplo de if / else (semana 8)
make arreglos  # ejecuta el ejemplo de arreglos (semana 9)
make errores   # ejecuta los ejemplos con errores
./demo.sh      # demostracion completa paso a paso
make clean     # borra los archivos generados
```

## Estructura del proyecto

```
.
├── Makefile
├── README.md
├── demo.sh / colores.sh      demostración y salida con colores
├── include/                  cabeceras (.h)
│   ├── ast.h  symtab.h  semantico.h  ejecutor.h  arreglos.h
│   └── parser.h  lexer.h
├── src/
│   ├── main.c                programa principal (coordina las fases)
│   ├── symtab.c              tabla de símbolos
│   ├── lexer/scanner.l       análisis léxico (Flex)
│   ├── parser/parser.y       análisis sintáctico (Bison)
│   ├── ast/ast.c             árbol de sintaxis abstracta
│   ├── semantico/semantico.c análisis semántico
│   └── runtime/              ejecución
│       ├── ejecutor.c
│       └── arreglos.c
├── ejemplos/                 programas de prueba (.mil)
└── build/                    archivos generados al compilar (no se suben a GitHub)
```

## Fases del compilador

| Fase | Archivo | Qué hace |
|---|---|---|
| 1. Análisis léxico | `src/lexer/scanner.l` | Convierte el texto en tokens; reporta caracteres inválidos y cadenas sin cerrar |
| 2. Análisis sintáctico | `src/parser/parser.y` | Verifica la gramática y construye el **AST** |
| 3. Análisis semántico | `src/semantico/semantico.c` | Llena la **Tabla de Símbolos**, revisa tipos y declara el tipo de cada nodo |
| 4. Ejecución | `src/runtime/ejecutor.c` | Recorre el AST y ejecuta el programa |
| Arreglos | `src/runtime/arreglos.c` | Crea los arreglos y hace `push`, `pop` y `length` |
| AST | `src/ast/ast.c` | Árbol gráfico y **Tabla AST** |
| Tabla de símbolos | `src/symtab.c` | Guarda nombre, tipo, ámbito, línea, inicialización, usos y valor |
| Programa principal | `src/main.c` | Llama a cada fase en orden y muestra sus tablas |

Si una fase encuentra errores, la compilación se detiene ahí.

## Reglas del lenguaje

### Reglas léxicas (tokens)

| Token | Patrón | Ejemplo |
|---|---|---|
| Palabras reservadas | `var print func return void if else int float str bool true false` | `if` |
| Comparaciones en palabras | `is` (==), `is not` (!=), `greater` (>), `less` (<), `or` (\|\|) | `x is 10` |
| ID (identificador) | letra o `_`, seguido de letras, dígitos o `_` | `edad`, `_total2` |
| INT_LIT | uno o más dígitos | `25` |
| FLOAT_LIT | dígitos `.` dígitos | `3.75` |
| STR_LIT | texto entre comillas dobles | `"Moises"` |
| Operadores | `+ - * /` `< > <= >= == !=` `&& \|\| !` `=` | |
| Signos | `: ; ( ) { } , [ ] .` | |
| Comentarios | `//` hasta el final de la línea (se ignoran) | |

### Reglas sintácticas (gramática)

```
programa     → sentencia*
sentencia    → declaracion | asignacion | impresion
declaracion  → var : TIPO ID ;
             | var : TIPO ID = expr ;
asignacion   → ID = expr ;
impresion    → print ( expr ) ;
TIPO         → int | float | str | bool
expr         → expr OP expr | - expr | ! expr | ( expr )
             | INT_LIT | FLOAT_LIT | STR_LIT | BOOL_LIT | ID
```

Precedencia (de menor a mayor): `||` → `&&` → `== !=` → `< > <= >=` → `+ -` → `* /` → `! -` (unarios). Los paréntesis cambian el orden.

### Funciones (semana 7)

```
funcion      → func ID ( parametros ) : TIPO_RETORNO { sentencia* }
parametros   → ε | ID : TIPO ( , ID : TIPO )*
TIPO_RETORNO → int | float | str | bool | void
retorno      → return expr ; | return ;
llamada      → ID ( argumentos )        (como expresión o como sentencia)
```

Ejemplo:

```
func sumar(a:int, b:int):int {
    return a + b;
}

func mostrar(mensaje:str):void {
    print("[log] " + mensaje);
}

var:int resultado = sumar(30, 20) * 2;
mostrar("listo");
```

Reglas semánticas de las funciones:

1. Una función se declara **antes** de llamarla y su nombre no se puede repetir.
2. Los **parámetros y variables locales** solo existen dentro de su función (ámbito propio). Una variable local puede tener el mismo nombre que una global.
3. La llamada debe tener **la misma cantidad de argumentos** que parámetros, y cada argumento debe ser **compatible** con el tipo de su parámetro (se permite `int → float`).
4. Una función que no es `void` **debe tener `return`** con un valor de su tipo.
5. Una función `void` **no puede devolver un valor** ni usarse dentro de una expresión o en `print`.
6. `return` solo se puede usar dentro de una función, y no se permiten funciones dentro de funciones.
7. Se permite la recursión; si una función se llama demasiadas veces (más de 1000 llamadas anidadas) se produce un error de ejecución.

En la ejecución, cada llamada crea un **marco** con sus parámetros y variables locales, por eso una función puede llamarse muchas veces sin mezclar sus valores. La Tabla de Símbolos muestra la **categoría** (variable, parámetro o función) y el **ámbito** (`global` o el nombre de la función).

### IF / ELSE (semana 8)

```
if <condicion> : {
    <sentencias>
} else {
    <sentencias>
}
```

- El `else` es opcional y los dos puntos `:` también (`if (x > y) { ... }` es válido).
- La condición se puede escribir con símbolos (`x > y`, `x == y`) o con palabras como en el ejemplo del profesor (`x greater y`, `x is y`, `x is not y`, `x less y`, `a or b`).
- Se puede usar en el programa principal, dentro de funciones y dentro de otro `if`.

Reglas semánticas del `if`:

1. **La condición debe ser de tipo `bool`**: `if edad : {...}` es un error porque `edad` es `int`.
2. Cada bloque `{ }` tiene **su propio ámbito**: una variable declarada dentro del `if` no existe afuera. En la Tabla de Símbolos su ámbito aparece como `if:<línea>` o `else:<línea>`.
3. Dentro de un bloque se puede **repetir el nombre** de una variable de afuera; adentro se usa la del bloque.
4. Si una función devuelve un valor y usa `if`, **debe devolver en todos los caminos**: un `if` con `return` necesita su `else` con `return`.
5. No se pueden declarar funciones dentro de un `if`.

En el AST el `if` es un nodo **IF** con tres hijos: la condición, el **BLOQUE** del `if` y (si existe) el **BLOQUE** del `else`. Al ejecutar se evalúa la condición: si es `true` corre el bloque del `if`, si es `false` corre el del `else`.

### Arreglos (semana 9)

```
TIPO_ARREGLO → int[] | float[] | str[] | bool[]
declaracion  → var : TIPO_ARREGLO ID = [ elementos ] ;
             | var : TIPO_ARREGLO ID ;              (empieza vacío: [])
acceso       → ID [ expr ]                          (el primer elemento es el 0)
metodo       → ID . push ( expr )                   agrega al final
             | ID . pop ( )                         quita el último y lo devuelve
             | ID . length ( )                      cantidad de elementos (int)
```

Ejemplo (como el del profesor):

```
var:int[] valores = [10, 20, 30];
var:int elegido = valores[1];          // 20
valores.push(40);                      // [10, 20, 30, 40]
var:int ultimo = valores[valores.length() - 1];
var:int quitado = valores.pop();       // 40
print(valores);                        // [10, 20, 30]
```

Reglas semánticas de los arreglos:

1. **Todos los elementos son del mismo tipo** (`[1, "dos"]` es un error). Si se mezclan `int` y `float`, el arreglo es `float[]`.
2. El arreglo se guarda solo en una variable de **su mismo tipo**: `var:int[] x = ["a"];` es un error. El literal vacío `[]` sirve para cualquier tipo, y `[1, 2]` se puede guardar en un `float[]` (los `int` pasan a `float`).
3. El **índice debe ser `int`** y solo se usa `[ ]` con variables que son arreglos.
4. `push` recibe **un valor del tipo de los elementos**; `pop` y `length` no reciben argumentos. Otros métodos no existen.
5. `push` no devuelve valor (no se puede imprimir); `pop` devuelve un elemento y `length` un `int`.
6. Los arreglos se pueden usar como **parámetros y tipo de retorno** de funciones (`func f(lista:int[]):int[]`). La función recibe el mismo arreglo (referencia), así que un `push` dentro de la función se ve afuera.
7. No se permiten arreglos dentro de arreglos, ni `+`, `==` u otros operadores entre arreglos.

Errores de ejecución: usar un **índice fuera del arreglo** (`valores[5]` cuando tiene 3) o hacer **`pop` a un arreglo vacío**.

En el AST hay tres nodos nuevos: **ARREGLO** (sus hijos son los elementos), **INDICE** (su hijo es el índice) y **METODO** (sus hijos son los argumentos).

### Reglas semánticas

1. Una variable debe **declararse antes de usarse**.
2. Una variable **no puede declararse dos veces** (ámbito global).
3. El valor asignado debe ser **compatible con el tipo** de la variable. Única conversión permitida: `int → float`.
4. `+ - * /` solo con números (`int` op `int` = `int`; si hay un `float`, el resultado es `float`).
5. `+` entre dos `str` **concatena**.
6. `< > <= >=` solo con números; `== !=` con valores del mismo tipo. El resultado es `bool`.
7. `&& || !` solo con `bool`.
8. **División entre cero** con literal → error semántico; con variable → error de ejecución.
9. Advertencias (no detienen): usar una variable sin inicializar, declarar una variable y nunca usarla.

## Ejemplos incluidos

| Archivo | Demuestra |
|---|---|
| `ejemplos/programa.mil` | Programa correcto con todos los tipos y operadores |
| `ejemplos/funciones.mil` | Funciones con parámetros, `return`, `void`, variables locales y llamadas anidadas |
| `ejemplos/if.mil` | `if` / `else`, comparaciones con palabras, condición `bool`, `if` con funciones, `if` anidado y variables del bloque |
| `ejemplos/error_if.mil` | Errores del `if`: condición no `bool`, tipos incompatibles, variable fuera de su bloque, `return` faltante en un camino |
| `ejemplos/arreglos.mil` | Arreglos: declarar, `[i]`, `push`, `pop`, `length`, arreglos vacíos, `float[]`/`bool[]`, arreglos con funciones e `if` |
| `ejemplos/error_arreglos.mil` | Errores de arreglos: tipos mezclados, índice no `int`, `[ ]` en una variable que no es arreglo, método inexistente, argumentos |
| `ejemplos/error_arreglos_ejecucion.mil` | Índice fuera del arreglo detectado al ejecutar |
| `ejemplos/error_funciones.mil` | Errores de funciones: argumentos, tipos de retorno, `return` faltante, ámbito |
| `ejemplos/error_lexico.mil` | Caracteres no reconocidos, cadena sin cerrar |
| `ejemplos/error_sintactico.mil` | Falta `;`, falta `:`, falta expresión, falta `)` |
| `ejemplos/error_semantico.mil` | Redeclaración, variable no declarada, tipos incompatibles, división entre cero |
| `ejemplos/error_ejecucion.mil` | División entre cero detectada al ejecutar |

## Extensión respecto al W5

Se agregó la sentencia `print(expr);`, operadores relacionales/lógicos y la concatenación de cadenas, para poder **ejecutar** y demostrar el lenguaje.
