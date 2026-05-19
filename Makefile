CC = gcc
CFLAGS = -Wall -I./include
LDFLAGS = -lcurl -lcjson -lhunspell-1.7 -lpthread

# On ne prend QUE tes fichiers NLP et le test_main
SRC = src/nlp/llm_client.c \
      src/nlp/nlp_engine.c \
      src/nlp/llm_json_parser.c \
      src/nlp/llm_prompts.c \
      src/nlp/hunspell_wrap.c \
      src/nlp/tokenizer.c \
      src/nlp/llm_thread.c \
      src/nlp/llm_interface.c \
      src/test_main.c

<<<<<<< HEAD
# Bibliothèques à lier (LDFLAGS)
# -lcjson : pour le JSON
# -lpcre2-8 : pour les Regex
LDFLAGS = -lcjson -lpcre2-8 -lm


# Liste de tes fichiers sources (DEV-D)
SRC = src/rules/rules.c \
      src/rules/rule_engine.c \
      src/rules/checkers/regex_checker.c \
      src/rules/test_main.c

# Liste de tes fichiers sources pour les tests (TESTS)
TEST_SRC_CMOCKA = tests/test_rules_cmocka.c \
                  src/rules/rule_engine.c \
                  src/rules/checkers/regex_checker.c

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

# Règle pour les tests avec cmocka
cmocka:
	$(CC) $(CFLAGS) $(TEST_SRC_CMOCKA) -o $(TEST_TARGET_CMOCKA) $(LDFLAGS) -lcmocka
	./$(TEST_TARGET_CMOCKA)

# Nettoyage des fichiers temporaires
clean:
	rm -f $(OBJ) $(TARGET)

# Pour éviter les conflits avec des fichiers du même nom
.PHONY: all clean
=======
all:
	$(CC) $(CFLAGS) $(SRC) -o dev_c_test $(LDFLAGS)
>>>>>>> 309d346bfb40c8d6c5be0b252351d35b27bd0e82
