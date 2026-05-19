#include <stdio.h>
#include <stdlib.h>

void prepare_prompt(char* buffer, size_t size, const char* consigne, const char* texte) {
    const char* template = 
        "### Rôle\n"
        "Tu es un expert en révision académique. Ta mission est de vérifier si un texte respecte scrupuleusement la règle fournie.\n\n"
        "### Exemples de comportement attendu\n"
        "Règle: Ne pas inclure de salutations\n"
        "Texte: Bonjour, nous allons analyser les données.\n"
        "Réponse: NON_CONFORME\n\n"
        "Règle: Utiliser le temps présent\n"
        "Texte: Nous analysons les données.\n"
        "Réponse: CONFORME\n\n"
        "### Tâche Actuelle\n"
        "Règle: %s\n"
        "Texte: %s\n"
        "Réponse (Réponds UNIQUEMENT par CONFORME ou NON_CONFORME) :";

    snprintf(buffer, size, template, consigne, texte);
}