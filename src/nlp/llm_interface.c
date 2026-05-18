#include "../../include/nlp.h"
#include "../../include/llm_thread.h"
#include "../../include/hunspell_wrap.h"

void nlp_system_init() {
    init_spell_checker(); // Vient de hunspell_wrap.c
    start_llm_thread();   // Vient de llm_thread.c
}

void nlp_system_cleanup() {
    cleanup_spell_checker();
}