#include <stdlib.h>
#include "../../include/nlp.h"
#include "../../include/llm_thread.h"

void nlp_process_check(const char* text, const char* rule_instruction) {
    // Vérification rapide (orthographe)
    // ... appel is_word_correct ...

    // Si c'est une règle complexe (Sémantique R009)
    if (rule_instruction != NULL) {
        push_llm_task(text, rule_instruction);
    }
}