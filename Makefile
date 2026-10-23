# ============================================================
# Makefile - Compilador de MiLenguaje
# Uso:  make           -> compila (genera ./milenguaje)
#       make run       -> ejecuta el programa de ejemplo
#       make tokens    -> ejecuta mostrando la tabla de tokens
#       make funciones -> ejemplo de funciones (semana 7)
#       make if        -> ejemplo de if / else (semana 8)
#       make arreglos  -> ejemplo de arreglos (semana 9)
#       make errores   -> ejecuta los ejemplos con errores
#       make clean     -> borra los archivos generados
#
# Estructura:
#   include/        cabeceras (.h)
#   src/lexer/      analizador lexico  (Flex)
#   src/parser/     analizador sintactico (Bison)
#   src/ast/        arbol de sintaxis abstracta
#   src/semantico/  analisis semantico
#   src/runtime/    ejecucion (interprete) y arreglos
#   build/          archivos generados al compilar
#   ejemplos/       programas de prueba (.mil)
# ============================================================
CC        = gcc
CFLAGS    = -Wall -g -Iinclude -Ibuild
PROG      = milenguaje
BUILD     = build

LEX_SRC   = src/lexer/scanner.l
YACC_SRC  = src/parser/parser.y
BISON_C   = $(BUILD)/parser.tab.c
BISON_H   = $(BUILD)/parser.tab.h
LEX_C     = $(BUILD)/lex.yy.c

FUENTES   = $(wildcard src/*.c src/ast/*.c src/semantico/*.c src/runtime/*.c)
CABECERAS = $(wildcard include/*.h)
OBJETOS   = $(patsubst src/%.c,$(BUILD)/%.o,$(FUENTES)) $(BUILD)/parser.tab.o $(BUILD)/lex.yy.o

$(PROG): $(OBJETOS)
	$(CC) $(CFLAGS) -o $@ $(OBJETOS)

$(BUILD)/%.o: src/%.c $(CABECERAS) $(BISON_H)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/parser.tab.o: $(BISON_C) $(CABECERAS)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/lex.yy.o: $(LEX_C) $(CABECERAS)
	$(CC) $(CFLAGS) -c $< -o $@

$(BISON_C) $(BISON_H): $(YACC_SRC)
	@mkdir -p $(BUILD)
	bison -d -o $(BISON_C) $(YACC_SRC)

$(LEX_C): $(LEX_SRC) $(BISON_H)
	@mkdir -p $(BUILD)
	flex -o $(LEX_C) $(LEX_SRC)

run: $(PROG)
	./$(PROG) ejemplos/programa.mil

tokens: $(PROG)
	./$(PROG) -t ejemplos/programa.mil

funciones: $(PROG)
	./$(PROG) ejemplos/funciones.mil

if: $(PROG)
	./$(PROG) ejemplos/if.mil

arreglos: $(PROG)
	./$(PROG) ejemplos/arreglos.mil

errores: $(PROG)
	-./$(PROG) ejemplos/error_lexico.mil
	-./$(PROG) ejemplos/error_sintactico.mil
	-./$(PROG) ejemplos/error_semantico.mil
	-./$(PROG) ejemplos/error_ejecucion.mil
	-./$(PROG) ejemplos/error_funciones.mil
	-./$(PROG) ejemplos/error_if.mil
	-./$(PROG) ejemplos/error_arreglos.mil
	-./$(PROG) ejemplos/error_arreglos_ejecucion.mil

clean:
	rm -rf $(BUILD) $(PROG)

.PHONY: run tokens funciones if arreglos errores clean
