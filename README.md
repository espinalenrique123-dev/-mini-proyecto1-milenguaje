# MiLenguaje — Proyecto Final

**Autor:** Moisés Moncada · **Curso:** IS-505 Análisis Léxico y Sintáctico · **Profesor:** Wilian Martínez · UNADECA

Compilador (con intérprete) para **MiLenguaje**, construido con **Flex + Bison + C**, siguiendo las reglas definidas en el trabajo *W5 – Mi Lenguaje de Programación*. Empezó como el Mini Proyecto 1 y cada semana se le agregó algo (funciones, `if`, arreglos). Para el **Proyecto Final** se le agregaron ciclos, lectura de datos y lo necesario para correr los **7 programas** que pide el profesor (ver [Proyecto Final](#proyecto-final)).

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
make ciclos    # ejecuta el ejemplo de while, for, break y % (Proyecto Final)
make errores   # ejecuta los ejemplos con errores

# Proyecto Final
./milenguaje -r programas/07_atm.mil   # -r: solo ejecuta (como un programa normal, sin las tablas)
make programas                         # corre los 7 programas con datos de prueba
make pruebas                           # pruebas automaticas: compara con la salida esperada
./demo_final.sh                        # demostracion para la defensa (menu)
./demo.sh      # demostracion completa paso a paso
make clean     # borra los archivos generados
```

## Estructura del proyecto

```
.
├── Makefile
├── README.md
├── demo.sh / colores.sh      demostración y salida con colores
├── demo_final.sh             demostración del Proyecto Final (menú)
├── pruebas.sh                pruebas automáticas de los 7 programas
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
├── programas/                los 7 programas del Proyecto Final
│   ├── 01_calificaciones.mil ... 07_atm.mil
│   ├── entradas/             datos que escribiría el usuario (para las pruebas)
│   └── esperado/             salida esperada de cada prueba
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
| Palabras reservadas | `var print func return void if else int float str bool true false while for to step break read` | `if` |
| Comparaciones en palabras | `is` (==), `is not` (!=), `greater` (>), `less` (<), `or` (\|\|) | `x is 10` |
| ID (identificador) | letra o `_`, seguido de letras, dígitos o `_` | `edad`, `_total2` |
| INT_LIT | uno o más dígitos | `25` |
| FLOAT_LIT | dígitos `.` dígitos | `3.75` |
| STR_LIT | texto entre comillas dobles | `"Moises"` |
| Operadores | `+ - * / %` `< > <= >= == !=` `&& \|\| !` `=` | |
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

## Proyecto Final

> *"Si su compilador es un lenguaje de programación deberá crear los programas descritos en el documento adjunto"* — por eso MiLenguaje necesitó ciclos y leer datos del teclado.

### Lo que se agregó al lenguaje

| Nuevo | Sintaxis | Para qué se usa en los programas |
|---|---|---|
| Ciclo **while** | `while <condicion> : { ... }` | Menú del ATM y del punto de venta, intentos del PIN |
| Ciclo **for** | `for i = <inicio> to <fin> : { ... }` | Recorrer los arreglos (el `fin` se incluye) |
| **for** con paso | `for i = 5 to 0 step -1 : { ... }` | Recorrer un arreglo al revés |
| **break** | `break;` | Salir del ciclo (invitado encontrado, opción "Salir") |
| **read** | `read(x);`  `read("mensaje: ", x);` | Pedir datos al usuario (int, float, str o bool) |
| **%** (residuo) | `n % 2` | Saber si un número es par |
| Cambiar un elemento | `lista[i] = valor;` | Invertir un arreglo intercambiando elementos |
| **else if** | `if ... { } else if ... { } else { }` | Menús con varias opciones |
| **print** con varios valores | `print("Total: ", total);` | Mostrar texto y números juntos |
| Texto **+** número | `"Nota " + 5` → `"Nota 5"` | Armar mensajes como `"Estudiante " + i` |
| **formato** | `formato(1.5, 2)` → `"1.50"` | Mostrar dinero con 2 decimales |
| Modo **-r** | `./milenguaje -r archivo.mil` | Correr el programa sin las tablas de las fases |

Gramática nueva:

```
mientras   → while expr [:] bloque
para       → for ID = expr to expr [step expr] [:] bloque
romper     → break ;
lectura    → read ( ID ) ;  |  read ( expr , ID ) ;
asig_indice→ ID [ expr ] = expr ;
impresion  → print ( expr ( , expr )* ) ;
sino       → else bloque  |  else si
expr       → ... | expr % expr
```

### Reglas semánticas de lo nuevo

1. La **condición del `while`** debe ser `bool` (igual que en el `if`).
2. El **inicio, el fin y el paso del `for`** deben ser `int`, y el paso no puede ser `0`.
3. La **variable del `for`** es `int`, se declara sola y **solo existe dentro del ciclo** (en la Tabla de Símbolos su ámbito aparece como `for:<línea>`).
4. **`break`** solo se puede usar dentro de un `while` o un `for`; sale del ciclo más cercano.
5. **`read`** guarda en una variable ya declarada que no sea arreglo. Si el usuario escribe algo que no es del tipo de la variable (por ejemplo `abc` en un `int`), se le vuelve a pedir.
6. **`%`** solo funciona con `int`; residuo entre `0` literal es error semántico.
7. En **`lista[i] = valor`** el índice es `int` y el valor debe ser del tipo de los elementos.
8. **`+`** con un `str` y un `int`, `float` o `bool` concatena y da `str`.
9. **`formato(numero, decimales)`** recibe un `int` o `float` y un `int`, y devuelve `str`.

Errores de ejecución nuevos: índice fuera del arreglo en `lista[i] = valor`, residuo entre cero, un ciclo que da más de 1 000 000 de vueltas (ciclo infinito) y quedarse sin datos para `read`.

En el AST hay 5 nodos nuevos: **WHILE** (condición y bloque), **FOR** (inicio, fin, paso y bloque), **BREAK**, **READ** y **ASIG_INDICE** (índice y valor).

### Los 7 programas

| # | Archivo | Programa | Usa |
|---|---|---|---|
| 1 | `programas/01_calificaciones.mil` | Calculadora de calificaciones: pide 5 notas, promedio, aprobados (≥ 60) y reprobados | `read`, `for`, `while` (valida 0–100), arreglo |
| 2 | `programas/02_mayor_menor.mil` | Mayor y menor de 10 números y su índice, sin `max()`/`min()` | `for`, arreglo |
| 3 | `programas/03_pares_impares.mil` | Pide 8 números: cuántos pares, cuántos impares y suma de los pares | `read`, `for`, `%` |
| 4 | `programas/04_invertir.mil` | Pide 6 datos y los invierte (en un arreglo nuevo y en el mismo) | `read`, `for ... step -1`, `lista[i] = x` |
| 5 | `programas/05_invitados.mil` | Búsqueda lineal de un invitado: "Acceso permitido" y asiento, o "No se encuentra en la lista" | `read`, `for`, `break` |
| 6 | `programas/06_punto_venta.mil` | Punto de venta con arreglos paralelos: catálogo, elegir productos y total | función, `while`, `else if`, `formato` |
| 7 | `programas/07_atm.mil` | Cajero automático: PIN con 3 intentos, menú, depósitos y retiros validados | `while`, `break`, funciones, `else if`, `formato` |

Cada programa se puede correr de tres formas:

```bash
./milenguaje -r programas/07_atm.mil                                   # en vivo: el usuario escribe los datos
./milenguaje -r programas/07_atm.mil < programas/entradas/07_atm.txt   # con los datos de prueba
./milenguaje programas/07_atm.mil < programas/entradas/07_atm.txt      # con las 4 fases y sus tablas
```

`./pruebas.sh` (o `make pruebas`) corre los programas con sus datos de `programas/entradas/` y compara la salida con `programas/esperado/`. Incluye casos como una nota fuera de rango, una letra donde va un número, un invitado que no está y la cuenta bloqueada del ATM.

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
| `ejemplos/ciclos.mil` | Lo nuevo del Proyecto Final: `while`, `for`, `step`, `%`, `break`, `lista[i] = x`, `else if`, ciclos dentro de funciones, `formato` |
| `ejemplos/error_ciclos.mil` | Errores de lo nuevo: condición del `while`, `for` con `float`, `break` fuera de un ciclo, variable del `for` afuera, `%` con `float`, `read` mal usado, `step 0` |
| `ejemplos/error_ciclos_ejecucion.mil` | Un `for` que se pasa del tamaño del arreglo |

## Extensión respecto al W5

Se agregó la sentencia `print(expr);`, operadores relacionales/lógicos y la concatenación de cadenas, para poder **ejecutar** y demostrar el lenguaje.
