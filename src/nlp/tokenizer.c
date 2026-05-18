#include "../../include/tokenizer.h"
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>

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
    
    // Découpage par ponctuation
    char* sentence = strtok(text_copy, ".!?");
    
    while (sentence != NULL) {
        // Optionnel : on pourrait envoyer chaque phrase au thread ici
        sentence = strtok(NULL, ".!?");
    }
    // Libération de la mémoire de la copie
    free(text_copy);
}