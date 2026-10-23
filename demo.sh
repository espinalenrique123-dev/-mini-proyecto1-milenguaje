#!/bin/bash
# Demostracion del compilador de MiLenguaje (con colores)
cd "$(dirname "$0")"

M="\033[1;38;5;141m"; R="\033[0m"
pausa() { echo; echo -ne "${M}>>> Enter para continuar: $1 ${R}"; read; clear; }

# Muestra codigo fuente con colores (usa bat si esta instalado)
ver() {
    if command -v batcat >/dev/null; then batcat --paging=never -l swift "$1"
    else cat "$1"; fi
}
# Ejecuta el compilador con la salida en colores
correr() { ./milenguaje "$@" | ./colores.sh | less -R; }

clear
ls --color=auto
echo; ls --color=auto src include
pausa "codigo fuente"
ver ejemplos/programa.mil
pausa "compilar"
make clean && make
pausa "tabla de tokens"
correr -t ejemplos/programa.mil
pausa "ejecucion completa"
correr ejemplos/programa.mil
pausa "funciones (codigo)"
ver ejemplos/funciones.mil
pausa "funciones (resultado)"
correr ejemplos/funciones.mil
pausa "if / else (codigo)"
ver ejemplos/if.mil
pausa "if / else (resultado)"
correr ejemplos/if.mil
pausa "arreglos (codigo)"
ver ejemplos/arreglos.mil
pausa "arreglos (resultado)"
correr ejemplos/arreglos.mil
pausa "errores"
for f in error_lexico error_sintactico error_semantico error_ejecucion error_funciones error_if error_arreglos error_arreglos_ejecucion; do
    ver ejemplos/$f.mil
    pausa "ver el resultado de $f"
    correr ejemplos/$f.mil
    clear
done
echo -e "${M}Fin de la demostracion${R}"
