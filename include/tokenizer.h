#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>

// Découpe le texte par ponctuation pour l'IA
void analyze_sentences(const char* full_text);

// Extrait spécifiquement une section (ex: "Introduction") pour le JSON R009
char* extract_section(const char* full_text, const char* section_name);

/* Tokenisation mot à mot (ASCII/latin simple) */
typedef struct {
    char* text;      /* mot alloué dynamiquement */
    int offset;      /* offset dans le texte source */
    int line;        /* ligne 1-based */
} WordToken;

/* Extrait tous les mots d’un texte.
 * Retourne un tableau alloué dynamiquement de WordToken, avec *out_count éléments.
 * L'appelant doit libérer avec free_word_tokens(tokens, count).
 */
WordToken* tokenize_words(const char* full_text, int* out_count);

/* Libère les ressources d’un tableau WordToken */
void free_word_tokens(WordToken* tokens, int count);

#endif
