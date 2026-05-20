#ifndef NLP_H
#define NLP_H

#include <stddef.h> /* size_t */
#include "rules.h"

// Initialise tout le système NLP (chargement de modèles, dictionnaires, etc.)
void nlp_system_init();

// Vérifie un mot (Hunspell)
int is_word_correct(const char* word);

// Construit le prompt pour l'appel IA
void prepare_prompt(char* buffer, size_t size, const char* consigne, const char* texte);

// Analyse une phrase ou section avec l'IA
RuleStatus ask_llm_semantic_check(const char* section_text, const char* instruction);

// Lance l'analyse complète (Pipeline)
void nlp_process_check(const char* text, const char* section_name, const char* rule_instruction);

// Stockage/récupération thread-safe de la dernière réponse brute IA
void nlp_set_last_llm_response(const char* text);
const char* nlp_get_last_llm_response(void);

// Nettoie la mémoire avant de quitter
void nlp_system_cleanup();

#endif
