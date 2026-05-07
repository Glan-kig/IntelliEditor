#include <string.h>
#include <ctype.h>

// Sépare le texte par ponctuation forte (. ! ?) pour analyse sémantique.
void analyze_sentences(const char* full_text) {
    // Crée une copie du texte pour éviter de modifier l'original.
    char* text_copy = strdup(full_text);
    // Utilise strtok pour séparer le texte en phrases basées sur la ponctuation forte.
    char* sentence = strtok(text_copy, ".!?");
    
    // Parcours chaque phrase extraite.
    while (sentence != NULL) {
        // Ici, tu pourras appeler ask_llm_semantic_check sur chaque phrase.
        sentence = strtok(NULL, ".!?");
    }
    // Libère la mémoire allouée pour la copie du texte.
    free(text_copy);
}