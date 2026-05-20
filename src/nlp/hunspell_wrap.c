#include <hunspell/hunspell.h>
#include <stdio.h>
#include <stdlib.h>

// Wrapper pour Hunspell, un correcteur orthographique open-source. Permet d'initialiser le correcteur, de vérifier les mots et de nettoyer les ressources.
//Hunhandle est une structure opaque qui represente une instance de correcteur orthographique. Elle est utilisee pour stocker les ressources et les donnees du correcteur, comme le dictionnaire charge et les regles d'affixation. En utilisant un handle, on peut facilement gerer plusieurs instances de correcteurs orthographiques si necessaire, et encapsuler la logique de correction dans des fonctions simples.


static Hunhandle* handle = NULL;

/* Initialise le dictionnaire français.
 * Les chemins doivent être robustes quand le binaire est lancé depuis build/ :
 * - sinon on cherche build/data/fr_FR.aff/dic au lieu de project/data/...
 */
void init_spell_checker() {
    const char *aff_paths[] = {
        "data/fr_FR.aff",
        "../data/fr_FR.aff"
    };
    const char *dic_paths[] = {
        "data/fr_FR.dic",
        "../data/fr_FR.dic"
    };

    handle = NULL;
    for (size_t i = 0; i < (sizeof(aff_paths) / sizeof(aff_paths[0])); i++) {
        handle = Hunspell_create(aff_paths[i], dic_paths[i]);
        if (handle) break;
    }

    if (!handle) {
        fprintf(stderr, "[ERROR] init_spell_checker: impossible de charger Hunspell (data/fr_FR.aff/dic ou ../data/fr_FR.aff/dic manquants)\n");
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

// Nettoie les ressources de Hunspell avant de quitter le programme.
void cleanup_spell_checker() {
    if (handle) {
        Hunspell_destroy(handle);
        handle = NULL;
    }
}