#ifndef NLP_H
#define NLP_H
#include "rules.h"


//fonction qui analyse une section du texte avec l'IA et retourne un statut de conformité


RuleStatus ask_llm_semantic_check(const char* section_text, const char*  prompt_instruction);

#endif