#include <hunspell/hunspell.h>
#include <stdio.h>
#include <stdlib.h>

// Wrapper pour Hunspell, un correcteur orthographique open-source. Permet d'initialiser le correcteur, de vérifier les mots et de nettoyer les ressources.
//Hunhandle est une structure opaque qui represente une instance de correcteur orthographique. Elle est utilisee pour stocker les ressources et les donnees du correcteur, comme le dictionnaire charge et les regles d'affixation. En utilisant un handle, on peut facilement gerer plusieurs instances de correcteurs orthographiques si necessaire, et encapsuler la logique de correction dans des fonctions simples.


static Hunhandle* handle = NULL;

// Initialise le dictionnaire français[cite: 137].
void init_spell_checker() {
    // Les fichiers .aff et .dic doivent être dans data/
    handle = Hunspell_create("data/fr_FR.aff", "data/fr_FR.dic");
     if (!handle) {
        fprintf(stderr, "[ERROR] init_spell_checker: impossible de charger Hunspell (data/fr_FR.aff ou data/fr_FR.dic manquant)\n");
    }
}

// Vérifie si un mot est correct selon le dictionnaire. Retourne 1 si correct, 0 sinon.
int is_word_correct(const char* word) {
    if (!handle) {
        fprintf(stderr, "[WARN] is_word_correct: correcteur non initialisé\n");
        return 0; // comportement sûr : considérer comme incorrect si Hunspell n'est pas prêt
    }
    if (!word) return 0;
    return Hunspell_spell(handle, word);
}

void cleanup_spell_checker() {
    if (handle) {
        Hunspell_destroy(handle);
        handle = NULL;
    }
}