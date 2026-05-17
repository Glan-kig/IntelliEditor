#include <stdio.h>
#include <string.h>

// Transforme la règle JSON en question pour l'IA
void prepare_prompt(char* buffer, size_t size, const char* param, const char* text) {
    // On utilise un template brut pour plus de clarté
    const char* template = 
        "### Rôle\n"
        "Tu es un expert en révision académique. Ta mission est de vérifier si un texte respecte une règle précise.\n\n"
        "### Exemples\n"
        "Règle: Ne pas utiliser 'je'\n"
        "Texte: Je pense que c'est bien.\n"
        "Réponse: NON_CONFORME\n\n"
        "Règle: Ne pas utiliser 'je'\n"
        "Texte: Nous pensons que c'est bien.\n"
        "Réponse: CONFORME\n\n"
        "### Tâche Actuelle\n"
        "Règle: %s\n"
        "Texte: %s\n"
        "Réponse (Réponds UNIQUEMENT par CONFORME ou NON_CONFORME):";

    // Remplissage du buffer avec sécurité
    snprintf(buffer, size, template, param, text);
}