#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <stdio.h>
#include <string.h>
#include "../../../include/rules.h"

/**
 * @brief Vérifie si un pattern regex interdit est présent
 * @param document_text Le texte à analyser
 * @param pattern Le pattern regex à rechercher
 * @return STATUS_NON_CONFORME si trouvé, STATUS_CONFORME sinon
 */
RuleStatus check_regex_forbidden(const char *document_text, const char *pattern) {
    // Validation des paramètres
    if (document_text == NULL) {
        fprintf(stderr, "[ERROR] check_regex_forbidden: document_text est NULL\n");
        return STATUS_NON_CONFORME;
    }
    
    if (pattern == NULL) {
        fprintf(stderr, "[ERROR] check_regex_forbidden: pattern est NULL\n");
        return STATUS_NON_CONFORME;
    }
    
    if (strlen(pattern) == 0) {
        fprintf(stderr, "[WARN] check_regex_forbidden: pattern est vide\n");
        return STATUS_CONFORME; // Rien à interdire = conforme
    }
    
    pcre2_code *re;
    int errornumber;
    PCRE2_SIZE erroroffset;

    // Compilation de la regex
    re = pcre2_compile(
        (PCRE2_SPTR)pattern,
        PCRE2_ZERO_TERMINATED,
        PCRE2_CASELESS,
        &errornumber,
        &erroroffset,
        NULL
    );

    if (re == NULL) {
        // Erreur de compilation regex
        PCRE2_UCHAR errbuffer[120];
        pcre2_get_error_message(errornumber, errbuffer, sizeof(errbuffer));
        fprintf(stderr, "[ERROR] Pattern regex invalid '%s': %s (offset %zu)\n", 
                pattern, (char*)errbuffer, erroroffset);
        return STATUS_AVERTISSEMENT; // Avertissement plutôt qu'erreur
    }

    // Exécution de la recherche
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

    // Interprétation du résultat
    if (rc >= 0) {
        fprintf(stderr, "[DEBUG] Pattern '%s' trouvé dans le texte\n", pattern);
        return STATUS_NON_CONFORME; // Mot/pattern interdit trouvé
    }
    
    if (rc == PCRE2_ERROR_NOMATCH) {
        fprintf(stderr, "[DEBUG] Pattern '%s' non trouvé (conforme)\n", pattern);
        return STATUS_CONFORME; // Pas de correspondance = conforme
    }
    
    // Autres erreurs d'exécution
    fprintf(stderr, "[WARN] Erreur lors de l'exécution du pattern '%s' (code: %d)\n", 
            pattern, rc);
    return STATUS_AVERTISSEMENT;
}