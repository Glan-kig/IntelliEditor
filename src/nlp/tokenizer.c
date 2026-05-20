#include "../../include/tokenizer.h"
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>

static int is_word_char(unsigned char c) {
    return isalpha(c) || c == '\'' || c == '-';
}

/**
 * Extrait une section spécifique (ex: "Introduction") pour l'analyse R009.
 * C'est cette fonction qui manquait à la compilation !
 */
char* extract_section(const char* full_text, const char* section_name) {
    if (!full_text || !section_name) return NULL;

    const char* p = full_text;
    size_t name_len = strlen(section_name);

    while (*p) {
        const char* line_start = p;

        // sauter les espaces de début de ligne
        while (*p == ' ' || *p == '\t') p++;

        // accepter une ligne commençant par # ou pas
        const char* candidate = p;
        if (*candidate == '#') {
            while (*candidate == '#') candidate++;
            while (*candidate == ' ' || *candidate == '\t') candidate++;
        }

        if (strncasecmp(candidate, section_name, name_len) == 0
            && (candidate[name_len] == '\0' || candidate[name_len] == '\n' || isspace((unsigned char)candidate[name_len]))) {
            // trouvé le titre
            const char* content_start = strchr(candidate, '\n');
            if (!content_start) return NULL;
            content_start++; // après le saut de ligne

            // chercher prochain titre Markdown
            const char* next = content_start;
            while (*next) {
                if (*next == '\n') {
                    const char* look = next + 1;
                    while (*look == ' ' || *look == '\t') look++;
                    if (*look == '#') break;
                }
                next++;
            }

            size_t len = next ? (size_t)(next - content_start) : strlen(content_start);
            char* section = malloc(len + 1);
            if (!section) return NULL;
            memcpy(section, content_start, len);
            section[len] = '\0';
            return section;
        }

        // passer à la ligne suivante
        p = strchr(p, '\n');
        if (!p) break;
        p++;
    }

    return NULL;
}

/**
 * Sépare le texte par ponctuation forte (. ! ?) pour analyse sémantique.
 */
void analyze_sentences(const char* full_text) {
    if (!full_text) return;

    // Crée une copie du texte pour éviter de modifier l'original (indispensable pour strtok)
    char* text_copy = strdup(full_text);
    if (!text_copy) return;

    // Découpage par ponctuation
    char* sentence = strtok(text_copy, ".!?");

    while (sentence != NULL) {
        // Optionnel : on pourrait envoyer chaque phrase au thread ici
        sentence = strtok(NULL, ".!?");
    }
    // Libération de la mémoire de la copie
    free(text_copy);
}

WordToken* tokenize_words(const char* full_text, int* out_count) {
    if (out_count) *out_count = 0;
    if (!full_text) return NULL;

    size_t cap = 32;
    size_t count = 0;
    WordToken* tokens = calloc(cap, sizeof(WordToken));
    if (!tokens) return NULL;

    int line = 1;
    int i = 0;
    while (full_text[i] != '\0') {
        unsigned char ch = (unsigned char)full_text[i];
        if (ch == '\n') {
            line++;
            i++;
            continue;
        }

        if (!is_word_char(ch)) {
            i++;
            continue;
        }

        int start = i;
        while (full_text[i] != '\0' && is_word_char((unsigned char)full_text[i])) {
            i++;
        }
        int end = i;
        int len = end - start;
        if (len <= 0) continue;

        if (count == cap) {
            cap *= 2;
            WordToken* grown = realloc(tokens, cap * sizeof(WordToken));
            if (!grown) {
                free_word_tokens(tokens, (int)count);
                return NULL;
            }
            tokens = grown;
        }

        tokens[count].text = malloc((size_t)len + 1);
        if (!tokens[count].text) {
            free_word_tokens(tokens, (int)count);
            return NULL;
        }
        memcpy(tokens[count].text, &full_text[start], (size_t)len);
        tokens[count].text[len] = '\0';
        tokens[count].offset = start;
        tokens[count].line = line;
        count++;
    }

    if (out_count) *out_count = (int)count;
    return tokens;
}

void free_word_tokens(WordToken* tokens, int count) {
    if (!tokens) return;
    for (int i = 0; i < count; i++) {
        free(tokens[i].text);
        tokens[i].text = NULL;
    }
    free(tokens);
}
