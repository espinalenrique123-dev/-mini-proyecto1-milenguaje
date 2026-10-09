# ============================================================
# Makefile - Compilador de MiLenguaje
# Uso:  make          -> compila
#       make run      -> ejecuta el programa de ejemplo
#       make tokens   -> ejecuta mostrando la tabla de tokens
#       make errores  -> ejecuta los ejemplos con errores
#       make funciones -> ejecuta el ejemplo de funciones (semana 7)
#       make clean    -> borra los archivos generados
# ============================================================
CC      = gcc
CFLAGS  = -Wall -g
OBJ     = parser.tab.c lex.yy.c ast.c symtab.c semantico.c ejecutor.c
PROG    = milenguaje

$(PROG): $(OBJ)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJ)

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: scanner.l parser.tab.h
	flex scanner.l

run: $(PROG)
	./$(PROG) ejemplos/programa.mil

tokens: $(PROG)
	./$(PROG) -t ejemplos/programa.mil

funciones: $(PROG)
	./$(PROG) ejemplos/funciones.mil

errores: $(PROG)
	-./$(PROG) ejemplos/error_lexico.mil
	-./$(PROG) ejemplos/error_sintactico.mil
	-./$(PROG) ejemplos/error_semantico.mil
	-./$(PROG) ejemplos/error_ejecucion.mil
	-./$(PROG) ejemplos/error_funciones.mil

clean:
	rm -f $(PROG) parser.tab.c parser.tab.h lex.yy.c

.PHONY: run tokens funciones errores clean
