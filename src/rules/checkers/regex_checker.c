#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <stdio.h>
#include <string.h>
#include "../../../include/rules.h"

/**
 * @brief Version optimisée : vérifie avec une regex déjà compilée
 * @param document_text Le texte à analyser
 * @param text_len Longueur précalculée du texte
 * @param re Regex précompilée (doit être valide)
 * @return STATUS_NON_CONFORME si trouvé, STATUS_CONFORME sinon
 */
static RuleStatus check_regex_forbidden_optimized(const char *document_text, size_t text_len, pcre2_code *re) {
    if (document_text == NULL || re == NULL) {
        fprintf(stderr, "[ERROR] check_regex_forbidden_optimized: paramètres invalides\n");
        return STATUS_NON_CONFORME;
    }
    
    // Pas besoin de recompiler : utiliser directement la regex précompilée
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);
    int rc = pcre2_match(
        re,
        (PCRE2_SPTR)document_text,
        text_len,  // Utiliser la longueur précalculée
        0,
        0,
        match_data,
        NULL
    );
    
    pcre2_match_data_free(match_data);
    
    if (rc >= 0) {
        return STATUS_NON_CONFORME;
    }
    return STATUS_CONFORME;
}

/**
 * @brief Version originale (pour compatibilité) - optimisée avec longueur
 * @param document_text Le texte à analyser
 * @param pattern Le pattern regex
 * @return STATUS_NON_CONFORME si trouvé, STATUS_CONFORME sinon
 */
RuleStatus check_regex_forbidden(const char *document_text, const char *pattern) {
    // Validation (inchangée)
    if (document_text == NULL || pattern == NULL) {
        return STATUS_NON_CONFORME;
    }
    
    size_t text_len = strlen(document_text);  // Précalculer une fois
    
    pcre2_code *re;
    int errornumber;
    PCRE2_SIZE erroroffset;
    
    re = pcre2_compile(
        (PCRE2_SPTR)pattern,
        PCRE2_ZERO_TERMINATED,
        PCRE2_CASELESS,
        &errornumber,
        &erroroffset,
        NULL
    );
    
    if (re == NULL) {
        return STATUS_AVERTISSEMENT;
    }
    
    RuleStatus result = check_regex_forbidden_optimized(document_text, text_len, re);
    pcre2_code_free(re);
    return result;
}