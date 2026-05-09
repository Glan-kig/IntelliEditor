#define PCRE2_CODE_UNIT_WIDTH 8
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../../include/rules.h"

/**
 * @brief Recherche une sous-chaîne insensible à la casse
 * @param haystack La chaîne à fouiller (peut être NULL)
 * @param needle La chaîne à chercher (peut être NULL)
 * @return Pointeur vers la première occurrence ou NULL si non trouvée
 */
static char* strcasestr_custom(const char* haystack, const char* needle) {
    if (haystack == NULL || needle == NULL) {
        return NULL;
    }
    
    size_t needle_len = strlen(needle);
    if (needle_len == 0) {
        return (char*)haystack; // Chaîne vide trouvée au début
    }
    
    for (const char* p = haystack; *p; p++) {
        if (strncasecmp(p, needle, needle_len) == 0) {
            return (char*)p; // Trouvé une correspondance
        }
    }
    return NULL; // Non trouvé
}

/**
 * @brief Vérifie si une section existe dans le document (insensible à la casse)
 * @param document_text Le texte du document à analyser
 * @param section_name Nom de la section à rechercher
 * @return STATUS_CONFORME si trouvée, STATUS_NON_CONFORME sinon
 */
RuleStatus check_section_exists(const char* document_text, const char* section_name) {
    // Validation des paramètres
    if (document_text == NULL) {
        fprintf(stderr, "[ERROR] check_section_exists: document_text est NULL\n");
        return STATUS_NON_CONFORME;
    }
    
    if (section_name == NULL) {
        fprintf(stderr, "[ERROR] check_section_exists: section_name est NULL\n");
        return STATUS_NON_CONFORME;
    }
    
    // Éviter les chaînes vides
    if (strlen(section_name) == 0) {
        fprintf(stderr, "[WARN] check_section_exists: section_name est vide\n");
        return STATUS_NON_CONFORME;
    }
    
    // Recherche insensible à la casse
    char* found = strcasestr_custom(document_text, section_name);
    
    if (found != NULL) {
        fprintf(stderr, "[DEBUG] Section '%s' trouvée à la position %ld\n", 
                section_name, (found - document_text));
        return STATUS_CONFORME;
    }
    
    fprintf(stderr, "[DEBUG] Section '%s' introuvable\n", section_name);
    return STATUS_NON_CONFORME;
}

/**
 * @brief Exécute le moteur de règles sur le texte fourni (version optimisée)
 * @param report Rapport contenant les règles
 * @param current_text Texte à analyser
 */
void run_rule_engine(RuleReport* report, const char* current_text) {
    // Validation des paramètres critiques
    if (report == NULL) {
        fprintf(stderr, "[ERROR] run_rule_engine: report est NULL\n");
        return;
    }
    
    size_t text_len = strlen(current_text);  // Précalculer UNE SEULE FOIS
    fprintf(stderr, "[INFO] Exécution du moteur de règles sur %zu caractères\n", text_len);
    
    // Cache pour les regex précompilées (local à la fonction)
    pcre2_code* regex_cache[report->rule_count];  // Tableau temporaire
    memset(regex_cache, 0, sizeof(regex_cache));  // Initialiser à NULL
    
    for (int i = 0; i < report->rule_count; i++) {
        Rule* r = &report->rules[i];
        
        fprintf(stderr, "[INFO] Exécution de la règle %s [%s]\n", r->id, r->check_type);
        
        // Utiliser un switch pour éviter strcmp() répétée
        int check_type_hash = 0;
        if (strcmp(r->check_type, "section_exists") == 0) check_type_hash = 1;
        else if (strcmp(r->check_type, "regex_forbidden") == 0) check_type_hash = 2;
        
        
        switch (check_type_hash) {
            case 1:  // section_exists
                if (r->parameter == NULL) {
                    r->status = STATUS_NON_CONFORME;
                } else {
                    r->status = check_section_exists(current_text, (char*)r->parameter);
                }
                break;
            case 2:  // regex_forbidden
                if (r->parameter == NULL) {
                    r->status = STATUS_NON_CONFORME;
                } else {
                    // Précompiler la regex UNE SEULE FOIS par règle
                    if (regex_cache[i] == NULL) {
                        int errornumber;
                        PCRE2_SIZE erroroffset;
                        regex_cache[i] = pcre2_compile(
                            (PCRE2_SPTR)r->parameter,
                            PCRE2_ZERO_TERMINATED,
                            PCRE2_CASELESS,
                            &errornumber,
                            &erroroffset,
                            NULL
                        );
                        if (regex_cache[i] == NULL) {
                            fprintf(stderr, "[ERROR] Échec compilation regex pour règle %s\n", r->id);
                            r->status = STATUS_AVERTISSEMENT;
                            break;
                        }
                    }
                    // Utiliser la version optimisée
                    r->status = check_regex_forbidden_optimized(current_text, text_len, regex_cache[i]);
                }
                break;
            default:
                fprintf(stderr, "[WARN] Type inconnu '%s'\n", r->check_type);
                r->status = STATUS_NON_CONFORME;
                break;
        }
        
        fprintf(stderr, "[INFO] Résultat : %s\n", 
                (r->status == STATUS_CONFORME) ? "CONFORME" : "NON_CONFORME");
    }
    
    // Nettoyer le cache
    for (int i = 0; i < report->rule_count; i++) {
        if (regex_cache[i] != NULL) {
            pcre2_code_free(regex_cache[i]);
        }
    }
}

// Met à jour les statistiques du rapport
void update_report_score(RuleReport* report) {
    if (report == NULL || report->rule_count == 0) return;

    int success_count = 0;
    for (int i = 0; i < report->rule_count; i++) {
        if (report->rules[i].status == STATUS_CONFORME) {
            success_count++;
        }
    }
    report->rules_ok = success_count;
}

// Fonction principale de diagnostic
void run_full_diagnostic(RuleReport* report, const char* text) {
    for (int i = 0; i < report->rule_count; i++) {
        Rule* r = &report->rules[i];

        // 1. Vérification de structure
        if (strcmp(r->check_type, "section_exists") == 0) {
            r->status = check_section_exists(text, "Introduction"); 
        } 
        // 2. Vérification de style (Regex)
        else if (strcmp(r->check_type, "regex_forbidden") == 0) {
            // Ici le pattern vient du JSON, ex: "\\b(je|moi|mon)\\b"
            r->status = check_regex_forbidden(text, "\\b(je|moi|mon)\\b");
        }
        // 3. Placeholder pour le LLM (Liaison avec le DEV-C)
        else if (strcmp(r->check_type, "llm_semantic") == 0) {
            r->status = STATUS_EN_COURS; 
        }
    }
    
    update_report_score(report);
}

void print_compliance_report(RuleReport* report) {
    printf("\n=== RAPPORT DE CONFORMITÉ ACADÉMIQUE ===\n");
    printf("Score : %d/%d\n", report->rules_ok, report->rule_count);
    
    for (int i = 0; i < report->rule_count; i++) {
        if (report->rules[i].status != STATUS_CONFORME) {
            printf("- [%s] ATTENTION : %s\n", 
                   (report->rules[i].severity == SEVERITY_ERROR) ? "ERREUR" : "AVERTISSEMENT",
                   report->rules[i].description);
        }
    }
    printf("========================================\n");
}