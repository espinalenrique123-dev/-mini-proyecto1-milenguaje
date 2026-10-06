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
make errores   # ejecuta los 4 ejemplos con errores
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
| Palabras reservadas | `var print int float str bool true false` | `var` |
| ID (identificador) | letra o `_`, seguido de letras, dígitos o `_` | `edad`, `_total2` |
| INT_LIT | uno o más dígitos | `25` |
| FLOAT_LIT | dígitos `.` dígitos | `3.75` |
| STR_LIT | texto entre comillas dobles | `"Moises"` |
| Operadores | `+ - * /` `< > <= >= == !=` `&& \|\| !` `=` | |
| Signos | `: ; ( )` | |
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
| `ejemplos/error_lexico.mil` | Caracteres no reconocidos, cadena sin cerrar |
| `ejemplos/error_sintactico.mil` | Falta `;`, falta `:`, falta expresión, falta `)` |
| `ejemplos/error_semantico.mil` | Redeclaración, variable no declarada, tipos incompatibles, división entre cero |
| `ejemplos/error_ejecucion.mil` | División entre cero detectada al ejecutar |

## Extensión respecto al W5

Se agregó la sentencia `print(expr);`, operadores relacionales/lógicos y la concatenación de cadenas, para poder **ejecutar** y demostrar el lenguaje.
