#ifndef NLP_H
#define NLP_H

#include "rules.h"

// Initialise Hunspell et le thread LLM
void nlp_system_init();

// Vérifie un mot (Hunspell)
int is_word_correct(const char* word);

// Analyse une phrase ou section avec l'IA
RuleStatus ask_llm_semantic_check(const char* section_text, const char* instruction);

// Lance l'analyse complète (Pipeline)
void nlp_process_check(const char* text, const char* rule_instruction);

// Nettoie la mémoire avant de quitter
void nlp_system_cleanup();

#endif