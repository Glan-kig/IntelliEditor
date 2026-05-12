#include <stdio.h>

// Ligne par ligne : Transforme la règle JSON en question pour l'IA
void prepare_prompt(char* buffer, size_t size, const char* param, const char* text) {
    // On force l'IA à être brève pour le parsing
    snprintf(buffer, size, 
        "Analyse académique.\nCritère : %s\nTexte : %s\nRéponds par CONFORME ou NON_CONFORME.", 
        param, text);
}