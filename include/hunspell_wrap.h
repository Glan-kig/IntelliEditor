#ifndef HUNSPELL_WRAP_H
#define HUNSPELL_WRAP_H

// Initialise le dictionnaire (Phase 2)
void init_spell_checker();

// Vérifie un mot (1 = OK, 0 = faute)
int is_word_correct(const char* word);

// Libère la mémoire
void cleanup_spell_checker();

#endif