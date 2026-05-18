#include "../../include/tokenizer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/**
 * Extrait une section spécifique (ex: "Introduction") pour l'analyse R009.
 * C'est cette fonction qui manquait à la compilation !
 */
char* extract_section(const char* full_text, const char* section_name) {
    if (!full_text || !section_name) return NULL;

    // Cherche le titre de la section dans le texte
    char* start = strstr(full_text, section_name);
    if (!start) return NULL;
    
    // On avance après le nom de la section
    start += strlen(section_name);

    // On duplique les 1000 prochains caractères (ou jusqu'à la fin)
    // strndup est parfait pour extraire une sous-chaîne proprement
    return strndup(start, 1000); 
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