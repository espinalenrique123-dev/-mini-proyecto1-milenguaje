#!/bin/bash
# Demostracion del compilador de MiLenguaje
cd "$(dirname "$0")"

pausa() { echo; read -p ">>> Enter para continuar: $1 "; clear; }

clear
ls
pausa "codigo fuente"
cat ejemplos/programa.mil
pausa "compilar"
make clean && make
pausa "tabla de tokens"
./milenguaje -t ejemplos/programa.mil | less
pausa "ejecucion completa"
./milenguaje ejemplos/programa.mil | less
pausa "funciones (codigo)"
cat ejemplos/funciones.mil
pausa "funciones (resultado)"
./milenguaje ejemplos/funciones.mil | less
pausa "errores"
for f in error_lexico error_sintactico error_semantico error_ejecucion error_funciones; do
    cat ejemplos/$f.mil
    pausa "ver el resultado de $f"
    ./milenguaje ejemplos/$f.mil | less
    clear
done
echo "Fin de la demostracion"
