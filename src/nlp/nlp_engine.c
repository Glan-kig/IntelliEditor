#include "../../include/nlp.h"
#include "../../include/llm_thread.h"
#include "../../include/tokenizer.h"
#include "../../include/hunspell_wrap.h"
#include <stdlib.h>


void nlp_process_check(const char* text, const char* rule_instruction) {
    // Si la règle demande une analyse sémantique 
    if (rule_instruction != NULL) {
        // On utilise ton tokenizer pour isoler la partie à envoyer
        char* section = extract_section(text, "Introduction");
        
        if (section) {
            // On l'envoie à la file asynchrone (llm_thread.c)
            push_llm_task(section, rule_instruction);
            free(section); // Très important : gestion de la RAM
        } else {
            // Si la section n'est pas trouvée, on envoie le texte brut
            push_llm_task(text, rule_instruction);
        }
    }
}