##############################################################################
#  Makefile — DiskScheduler
#
#  Compila o simulador de escalonamento de disco a partir das fontes em
#  src/ e dos cabecalhos em include/, gerando o executavel final na raiz
#  do projeto.
#
#  Uso:
#    make            -> compila (incremental) e gera o executavel
#    make run        -> compila (se necessario) e executa o programa
#    make clean      -> remove todos os artefatos de build (.o, .d, bin)
#    make rebuild    -> equivalente a "make clean && make"
#
#  Funciona tanto em Linux/macOS quanto em Windows (via MinGW/mingw32-make
#  ou MSYS2). A deteccao de sistema operacional abaixo ajusta apenas o
#  nome do executavel final (.exe no Windows) e os comandos de remocao de
#  arquivos usados pelo "make clean".
##############################################################################

# ---------------------------------------------------------------------------
# Compilador e flags
# ---------------------------------------------------------------------------

CC      := gcc

# -std=c11       : padrao de linguagem exigido pelo projeto
# -Wall -Wextra  : habilita avisos relevantes (boas praticas de C)
# -Iinclude      : permite '#include "arquivo.h"' encontrar include/
# -MMD -MP       : gera automaticamente arquivos de dependencia (.d) para
#                  cada .o, listando quais headers cada .c inclui. Isso e
#                  o que garante que, se voce editar um .h (por exemplo
#                  scheduler.h), o make saiba que precisa recompilar todo
#                  .c que depende dele — evitando o problema classico de
#                  "recompilei mas o binario nao mudou" por causa de um
#                  .o em cache desatualizado.
CFLAGS  := -std=c11 -Wall -Wextra -Iinclude -MMD -MP

# Flags extras apenas para build de depuracao (uso: make DEBUG=1)
ifdef DEBUG
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2
endif

LDFLAGS :=
LDLIBS  :=

# ---------------------------------------------------------------------------
# Deteccao de sistema operacional (ajusta apenas nome do binario e comandos
# de limpeza; o restante do Makefile e identico nas duas plataformas)
# ---------------------------------------------------------------------------

ifeq ($(OS),Windows_NT)
    TARGET_NAME := disk_scheduler.exe
    RM          := del /Q /F
    RMDIR       := rmdir /S /Q
    FIXPATH      = $(subst /,\,$1)
    NULL_REDIR  := >NUL 2>&1
else
    TARGET_NAME := disk_scheduler
    RM          := rm -f
    RMDIR       := rm -rf
    FIXPATH      = $1
    NULL_REDIR  :=
endif

# ---------------------------------------------------------------------------
# Diretorios e arquivos
# ---------------------------------------------------------------------------

SRC_DIR   := src
BUILD_DIR := build

# Descobre automaticamente todos os .c diretamente dentro de src/
# (NAO entra em subpastas como src/tests ou src/output — evita compilar
# arquivos de teste ou saidas antigas junto do binario principal).
SOURCES := $(wildcard $(SRC_DIR)/*.c)

# Espelha cada arquivo fonte src/xxx.c em um objeto build/xxx.o
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Arquivos de dependencia gerados por -MMD (um por .o); incluidos mais
# abaixo para que o make saiba recompilar quando um header mudar.
DEPS := $(OBJECTS:.o=.d)

TARGET := $(TARGET_NAME)

# ---------------------------------------------------------------------------
# Regras principais
# ---------------------------------------------------------------------------

# Alvo padrao: "make" sem argumentos cai aqui.
.PHONY: all
all: $(TARGET)

# Linkagem final: junta todos os .o no executavel.
# O executavel depende de todos os objetos — se qualquer um for
# recompilado, o link e refeito.
$(TARGET): $(OBJECTS)
	@echo "Linkando $(TARGET)..."
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)
	@echo "Build concluido: $(TARGET)"

# Compilacao de cada .c em seu .o correspondente.
# BUILD_DIR e criado automaticamente (ordem de dependencia via "|") antes
# de qualquer compilacao, sem forcar recompilacao caso ja exista.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Cria o diretorio de build se ainda nao existir.
$(BUILD_DIR):
	mkdir $(call FIXPATH,$(BUILD_DIR))

# Inclui os arquivos de dependencia gerados (.d), se existirem.
# O "-" na frente evita erro no primeiro build, quando eles ainda nao
# foram gerados.
-include $(DEPS)

# ---------------------------------------------------------------------------
# Execucao
# ---------------------------------------------------------------------------

# Compila (se necessario) e roda o simulador em seguida.
.PHONY: run
run: all
	./$(TARGET)

# ---------------------------------------------------------------------------
# Limpeza
# ---------------------------------------------------------------------------

# Remove todos os artefatos de build (objetos, dependencias e o binario),
# forcando a proxima "make" a recompilar tudo do zero.
.PHONY: clean
clean:
	@echo "Limpando artefatos de build..."
	$(RMDIR) $(call FIXPATH,$(BUILD_DIR)) $(NULL_REDIR)
	$(RM) $(call FIXPATH,$(TARGET)) $(NULL_REDIR)

# Limpeza total seguida de build completo — use sempre que quiser ter
# certeza absoluta de que nao ha nenhum .o desatualizado sendo reusado.
.PHONY: rebuild
rebuild: clean all

##############################################################################
# Fim do Makefile
##############################################################################