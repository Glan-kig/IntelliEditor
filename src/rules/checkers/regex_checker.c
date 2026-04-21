#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <stdio.h>
#include <string.h>
#include "../../../include/rules.h"

// Verifie si un motif interdit est present dans le texte
RuleStatus check_regex_forbiden(const char *document_text, const char *pattern){
    pcre2_code *re;
    int errornumber;
    PCRE2_SIZE erroroffset;

    // 1. Compilation de la regex (ex : "\\bje\\b|\\bj'")
    re = pcre2_compile(\
        (PCRE2_SPTR)pattern,
        PCRE2_ZERO_TERMINATED,
        PCRE2_CASELESS, // Ignore la casse
        &errornumber,
        &erroroffset,
        NULL
    );

    if (re == NULL) {
        return STATUS_AVERTISSEMENT; // Erreur dans la regex elle-même
    }

    // 2. Execution de la recherche dans le texte
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);
    int rc = pcre2_match(
        re,
        (PCRE2_SPTR)document_text,
        strlen(document_text),
        0,
        0,
        match_data,
        NULL
    );

    // Nettoyage
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);

    // 3. Resultat : si rc >= 0, on a trouvé un mot interdit
    if (rc >= 0) {
        return STATUS_NON_CONFORME;
    }

    return STATUS_CONFORME; // Aucun mot interdit trouvé
}