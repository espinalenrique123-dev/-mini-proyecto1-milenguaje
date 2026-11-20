#!/bin/bash
# ============================================================
# pruebas.sh - Pruebas automaticas del Proyecto Final
# Corre cada programa con sus datos de programas/entradas/ y
# compara la salida con la esperada en programas/esperado/.
# Uso: ./pruebas.sh            (o: make pruebas)
#      ./pruebas.sh --generar  (vuelve a crear las salidas esperadas)
# ============================================================
cd "$(dirname "$0")"
make -s || exit 1
mkdir -p programas/esperado

ok=0; fallas=0
for entrada in programas/entradas/*.txt; do
    caso=$(basename "$entrada" .txt)
    # 07_atm_bloqueado.txt usa el programa 07_atm.mil
    prog=$(ls programas/"${caso:0:2}"_*.mil | head -1)
    esperado="programas/esperado/$caso.txt"
    salida=$(./milenguaje -r "$prog" < "$entrada" 2>&1)
    if [ "$1" = "--generar" ]; then
        echo "$salida" > "$esperado"; echo "generado: $esperado"; continue
    fi
    if [ "$salida" = "$(cat "$esperado")" ]; then
        echo "  [OK] $caso"; ok=$((ok + 1))
    else
        echo "  [X] $caso: la salida no es la esperada"
        diff <(echo "$salida") "$esperado" | head -10
        fallas=$((fallas + 1))
    fi
done
# Programas sin datos de entrada
for prog in programas/02_mayor_menor.mil ejemplos/ciclos.mil; do
    caso=$(basename "$prog" .mil)
    salida=$(./milenguaje -r "$prog" 2>&1)
    if [ "$1" = "--generar" ]; then echo "$salida" > "programas/esperado/$caso.txt"; continue; fi
    if [ "$salida" = "$(cat "programas/esperado/$caso.txt")" ]; then
        echo "  [OK] $caso"; ok=$((ok + 1))
    else
        echo "  [X] $caso: la salida no es la esperada"; fallas=$((fallas + 1))
    fi
done
[ "$1" = "--generar" ] && exit 0
echo
echo "  Resultado: $ok prueba(s) correctas, $fallas con fallas."
[ "$fallas" -eq 0 ]
