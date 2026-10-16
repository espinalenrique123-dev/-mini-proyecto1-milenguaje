#!/bin/bash
# colores.sh - Pinta con colores la salida del compilador de MiLenguaje
# Uso: ./milenguaje archivo.mil | ./colores.sh
awk '
BEGIN {
  R="\033[0m"; B="\033[1m"; DIM="\033[2m"
  MOR="\033[38;5;141m"; CIA="\033[38;5;81m"; VER="\033[38;5;114m"; ROJ="\033[38;5;203m"
  AMA="\033[38;5;221m"; AZU="\033[38;5;75m"; GRI="\033[38;5;244m"; NAR="\033[38;5;215m"; ROS="\033[38;5;212m"
  titulo=0; cab=0
}
!/^ *[+|].*[+|] *$/ { cab=0 }
/COMPILADOR DE MiLenguaje/ { print B MOR $0 R; next }
/^=+$/ { print MOR $0 R; titulo = !titulo; next }
titulo == 1 { print B CIA $0 R; next }
/\[OK\]/ { sub(/\[OK\]/, B VER "[OK]" R VER); print $0 R; next }
/\[X\]/  { sub(/\[X\]/, B ROJ "[X]" R ROJ); print $0 R; next }
/\[!\]/  { sub(/\[!\]/, B AMA "[!]" R AMA); print $0 R; next }
/^ *\+[-+]+\+ *$/ { print AZU $0 R; cab++; next }
/^ *\|.*\| *$/ {
  line=$0
  if (cab == 1) { gsub(/[^| ]+/, B AMA "&" R, line) }
  gsub(/\|/, AZU "|" R, line)
  print line; next
}
/^  > / { sub(/^  > /, "  " VER "\342\226\266 " R B); print $0 R; next }
/correctamente/ { print B VER "  \342\234\224" $0 R; next }
/Compilacion detenida|error de ejecucion|Error de ejecucion/ { print B ROJ $0 R; next }
/Resultado:/ { print B AMA $0 R; next }
/^[|` -]*[A-Z_]+  \[/ {
  line=$0
  match(line, /[A-Z_]+  /); pre=substr(line,1,RSTART-1); nodo=substr(line,RSTART,RLENGTH); rest=substr(line,RSTART+RLENGTH)
  sub(/\(linea [0-9]+\)/, GRI "&" R, rest)
  sub(/\[[^]]*\]/, NAR "&" R, rest)
  print GRI pre R B CIA nodo R rest; next
}
{ print }
'
