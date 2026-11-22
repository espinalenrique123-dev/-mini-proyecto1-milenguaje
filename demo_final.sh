#!/bin/bash
# ============================================================
# demo_final.sh - Demostracion del Proyecto Final
# Muestra el codigo de cada programa y lo ejecuta para que el
# usuario escriba los datos en vivo.
# Uso: ./demo_final.sh
# Moises Moncada - Compiladores e Interpretes
# ============================================================
cd "$(dirname "$0")"

M="\033[1;38;5;141m"; C="\033[1;38;5;81m"; V="\033[1;38;5;114m"; R="\033[0m"
pausa() { echo; echo -ne "${M}>>> Enter para continuar ${R}"; read -r; }

# Muestra codigo fuente con colores (usa bat si esta instalado)
ver() {
    if command -v batcat >/dev/null; then batcat --paging=never -l swift "$1"
    else cat -n "$1"; fi
}

# Muestra el codigo, compila y ejecuta un programa (modo -r: solo la salida)
correr() {
    clear
    echo -e "${C}==================== $1 ====================${R}"
    ver "$1"
    pausa
    clear
    echo -e "${C}==================== Ejecutando $1 ====================${R}"
    echo
    ./milenguaje -r "$1"
    echo
    echo -e "${V}(fin del programa)${R}"
    pausa
}

# Ejecuta un programa con los datos de prueba de programas/entradas/
automatico() {
    local base; base=$(basename "$1" .mil)
    if [ -f "programas/entradas/$base.txt" ]; then
        echo -e "${C}--- $base (datos de programas/entradas/$base.txt) ---${R}"
        ./milenguaje -r "$1" < "programas/entradas/$base.txt" | ./colores.sh
    else
        echo -e "${C}--- $base (no pide datos) ---${R}"
        ./milenguaje -r "$1" | ./colores.sh
    fi
    echo
}

echo -e "${M}Compilando MiLenguaje...${R}"
make -s || { echo "No se pudo compilar"; exit 1; }

while true; do
    clear
    echo -e "${M}=========== PROYECTO FINAL - MiLenguaje ===========${R}"
    echo "  1. Calculadora de calificaciones"
    echo "  2. Buscador de numero mayor y menor"
    echo "  3. Clasificador de pares e impares"
    echo "  4. Invertir el orden de un arreglo"
    echo "  5. Buscador de invitados"
    echo "  6. Mini punto de venta"
    echo "  7. Cajero automatico (ATM)"
    echo "  8. Lo nuevo del lenguaje (ciclos.mil)"
    echo "  9. Errores de lo nuevo"
    echo "  a. Correr los 7 programas con datos de prueba"
    echo "  c. Ver las 4 fases del compilador (tokens, AST, tablas) de un programa"
    echo "  0. Salir"
    echo -ne "${M}Elija una opcion: ${R}"
    read -r op
    case "$op" in
        1) correr programas/01_calificaciones.mil ;;
        2) correr programas/02_mayor_menor.mil ;;
        3) correr programas/03_pares_impares.mil ;;
        4) correr programas/04_invertir.mil ;;
        5) correr programas/05_invitados.mil ;;
        6) correr programas/06_punto_venta.mil ;;
        7) correr programas/07_atm.mil ;;
        8) correr ejemplos/ciclos.mil ;;
        9) correr ejemplos/error_ciclos.mil; correr ejemplos/error_ciclos_ejecucion.mil ;;
        a|A) clear; for f in programas/0*.mil; do automatico "$f"; done; pausa ;;
        c|C) echo -ne "Numero del programa (1-7): "; read -r n
             f=$(ls programas/0"$n"_*.mil 2>/dev/null | head -1)
             if [ -n "$f" ]; then
                 base=$(basename "$f" .mil)
                 if [ -f "programas/entradas/$base.txt" ]; then
                     ./milenguaje -t "$f" < "programas/entradas/$base.txt" | ./colores.sh | less -R
                 else
                     ./milenguaje -t "$f" | ./colores.sh | less -R
                 fi
             fi ;;
        0) echo -e "${M}Fin de la demostracion${R}"; exit 0 ;;
    esac
done
