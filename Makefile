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

all:
	$(CC) $(CFLAGS) $(SRC) -o dev_c_test $(LDFLAGS)