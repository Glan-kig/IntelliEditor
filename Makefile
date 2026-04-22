# Nom de l'exécutable final
TARGET = intelli_engine

# Compilateur
CC = gcc

# Options de compilation :
# -I./include : pour trouver tes headers
# -Wall -Wextra : pour voir tous les avertissements
CFLAGS = -I./include -Wall -Wextra -g

# Bibliothèques à lier (LDFLAGS)
# -lcjson : pour le JSON
# -lpcre2-8 : pour les Regex
LDFLAGS = -lcjson -lpcre2-8

# Liste de tes fichiers sources (DEV-D)
SRC = src/rules/rules.c \
      src/rules/rule_engine.c \
      src/rules/checkers/regex_checker.c \
      src/rules/test_main.c

# Transformation de la liste .c en .o (fichiers objets)
OBJ = $(SRC:.c=.o)

# Règle par défaut
all: $(TARGET)

# Liaison de l'exécutable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Compilation des fichiers .c en .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage des fichiers temporaires
clean:
	rm -f $(OBJ) $(TARGET)

# Pour éviter les conflits avec des fichiers du même nom
.PHONY: all clean