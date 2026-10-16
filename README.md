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
make errores   # ejecuta los ejemplos con errores
./demo.sh      # demostracion completa paso a paso
make clean     # borra los archivos generados
```

## Fases del compilador

| Fase | Archivo | Qué hace |
|---|---|---|
| 1. Análisis léxico | `scanner.l` | Convierte el texto en tokens; reporta caracteres inválidos y cadenas sin cerrar |
| 2. Análisis sintáctico | `parser.y` | Verifica la gramática y construye el **AST** |
| 3. Análisis semántico | `semantico.c` | Llena la **Tabla de Símbolos**, revisa tipos y declara el tipo de cada nodo |
| 4. Ejecución | `ejecutor.c` | Recorre el AST y ejecuta el programa |
| AST | `ast.c` | Árbol gráfico y **Tabla AST** |
| Tabla de símbolos | `symtab.c` | Guarda nombre, tipo, ámbito, línea, inicialización, usos y valor |

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
| Signos | `: ; ( ) { } ,` | |
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
| `ejemplos/error_funciones.mil` | Errores de funciones: argumentos, tipos de retorno, `return` faltante, ámbito |
| `ejemplos/error_lexico.mil` | Caracteres no reconocidos, cadena sin cerrar |
| `ejemplos/error_sintactico.mil` | Falta `;`, falta `:`, falta expresión, falta `)` |
| `ejemplos/error_semantico.mil` | Redeclaración, variable no declarada, tipos incompatibles, división entre cero |
| `ejemplos/error_ejecucion.mil` | División entre cero detectada al ejecutar |

## Extensión respecto al W5

Se agregó la sentencia `print(expr);`, operadores relacionales/lógicos y la concatenación de cadenas, para poder **ejecutar** y demostrar el lenguaje.
