#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../../include/rules.h"

/* ============================================================================
 * MACROS POUR LA MAINTENABILITÉ ET LE DÉBOGAGE
 * ============================================================================ */

#define LOG_ERROR(msg, ...) fprintf(stderr, "[ERROR] " msg "\n", ##__VA_ARGS__)
#define LOG_WARN(msg, ...)  fprintf(stderr, "[WARN]  " msg "\n", ##__VA_ARGS__)
#define LOG_INFO(msg, ...)  fprintf(stderr, "[INFO]  " msg "\n", ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) fprintf(stderr, "[DEBUG] " msg "\n", ##__VA_ARGS__)

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
 * 
 * Recherche la présence du nom de section fourni dans le texte du document,
 * sans tenir compte de la casse. Utilise une comparaison caractère par
 * caractère pour une recherche case-insensitive robuste.
 *
 * @param[in] document_text Texte du document à analyser (peut être NULL)
 * @param[in] section_name Nom de la section à rechercher (peut être NULL)
 * @return STATUS_CONFORME si la section est trouvée, STATUS_NON_CONFORME sinon
 * @note Retourne STATUS_NON_CONFORME pour les paramètres NULL (par sécurité)
 * @warning Recherche sensible à l'ordre mais insensible à la casse
 */
RuleStatus check_section_exists(const char* document_text, const char* section_name) {
    // === Section: Validation des paramètres ===
    if (document_text == NULL) {
        LOG_ERROR("check_section_exists: document_text est NULL");
        return STATUS_NON_CONFORME;
    }

    if (section_name == NULL) {
        LOG_ERROR("check_section_exists: section_name est NULL");
        return STATUS_NON_CONFORME;
    }

    if (strlen(section_name) == 0) {
        LOG_WARN("check_section_exists: section_name est vide");
        return STATUS_NON_CONFORME;
    }

    // === Section: Recherche insensible à la casse ===
    char* found_position = strcasestr_custom(document_text, section_name);

    if (found_position != NULL) {
        LOG_DEBUG("Section '%s' trouvée à la position %ld", 
                section_name, (found_position - document_text));
        return STATUS_CONFORME;
    }

    LOG_DEBUG("Section '%s' introuvable dans le document", section_name);
    return STATUS_NON_CONFORME;
}

/**
 * @brief Exécute le moteur de règles sur le texte fourni
 * 
 * Parcourt toutes les règles du rapport et exécute le vérificateur approprié
 * en fonction du type de vérification (check_type). Met à jour le statut de
 * chaque règle en fonction du résultat de la vérification.
 *
 * @param[in,out] report Rapport contenant les règles (ne doit pas être NULL)
 * @param[in] current_text Texte à analyser (ne doit pas être NULL)
 * @note Valide tous les paramètres avant exécution
 * @warning Les types de vérification inconnus génèrent un avertissement
 * @warning Les paramètres manquants dans les règles causent un échec
 */
void run_rule_engine(RuleReport* report, const char* current_text) {
    // === Section: Validation des paramètres critiques ===
    if (report == NULL) {
        LOG_ERROR("run_rule_engine: report est NULL");
        return;
    }

    if (current_text == NULL) {
        LOG_ERROR("run_rule_engine: current_text est NULL");
        
        // Marquer toutes les règles comme échouées pour intégrité
        for (int rule_idx = 0; rule_idx < report->rule_count; rule_idx++) {
            report->rules[rule_idx].status = STATUS_NON_CONFORME;
        }
        return;
    }

    size_t text_length = strlen(current_text);
    LOG_INFO("Exécution du moteur de règles sur %zu caractères", text_length);

    // === Section: Exécution de chaque règle ===
    for (int rule_idx = 0; rule_idx < report->rule_count; rule_idx++) {
        Rule* current_rule = &report->rules[rule_idx];

        LOG_INFO("Exécution de la règle %s [%s]", 
                current_rule->id, current_rule->check_type);

        // Dispatcher basé sur le type de vérification
        if (strcmp(current_rule->check_type, "section_exists") == 0) {
            // Vérifier la disponibilité du paramètre
            if (current_rule->parameter == NULL) {
                LOG_WARN("Règle %s: paramètre section_name est NULL", current_rule->id);
                current_rule->status = STATUS_NON_CONFORME;
            } else {
                current_rule->status = check_section_exists(
                    current_text, 
                    (char*)current_rule->parameter
                );
            }
        } else if (strcmp(current_rule->check_type, "regex_forbidden") == 0) {
            if (current_rule->parameter == NULL) {
                LOG_WARN("Règle %s: paramètre regex_pattern est NULL", current_rule->id);
                current_rule->status = STATUS_NON_CONFORME;
            } else {
                current_rule->status = check_regex_forbidden(
                    current_text, 
                    (char*)current_rule->parameter
                );
            }
        } else {
            LOG_WARN("Règle %s: type de vérification inconnu '%s'", 
                    current_rule->id, current_rule->check_type);
            current_rule->status = STATUS_NON_CONFORME;
        }

        LOG_INFO("Résultat : %s", 
                (current_rule->status == STATUS_CONFORME) ? "CONFORME" : "NON_CONFORME");
    }
}

/**
 * @brief Met à jour les statistiques du rapport en fonction des statuts actuels
 * 
 * Compte le nombre de règles avec le statut STATUS_CONFORME et met à jour
 * le champ rules_ok du rapport. Cette fonction doit être appelée après
 * l'exécution de toutes les règles pour obtenir un score à jour.
 *
 * @param[in,out] report Rapport dont les statistiques doivent être mises à jour
 * @return void
 * @note Gère gracieusement les rapports NULL ou vides
 * @warning Le rapport doit contenir au moins rule_count et rules initialisés
 */
void update_report_score(RuleReport* report) {
    // === Section: Validation ===
    if (report == NULL) {
        LOG_WARN("update_report_score: report est NULL");
        return;
    }
    
    if (report->rule_count == 0) {
        LOG_DEBUG("update_report_score: rapport vide (0 règles)");
        report->rules_ok = 0;
        return;
    }

    // === Section: Calcul du score ===
    int success_count = 0;
    for (int rule_idx = 0; rule_idx < report->rule_count; rule_idx++) {
        if (report->rules[rule_idx].status == STATUS_CONFORME) {
            success_count++;
        }
    }
    
    report->rules_ok = success_count;
    LOG_DEBUG("Score mis à jour: %d/%d règles conformes", report->rules_ok, report->rule_count);
}

/**
 * @brief Fonction principale de diagnostic : exécute toutes les règles sur le texte fourni
 * 
 * Cette fonction applique toutes les règles définies dans le rapport au texte fourni.
 * Elle utilise des optimisations comme la précompilation de regex pour améliorer les performances.
 * Après exécution, elle met à jour automatiquement le score du rapport.
 * 
 * Les regex constantes sont précompilées une seule fois pour optimisation.
 * Tous les paramètres sont validés. En cas d'erreur, la fonction continue
 * mais met un statut approprié (AVERTISSEMENT ou NON_CONFORME).
 *
 * @param[in,out] report Rapport contenant les règles à exécuter (ne doit pas être NULL)
 * @param[in] text Texte du document à analyser (ne doit pas être NULL)
 * @return void
 * @note Valide tous les paramètres d'entrée avant exécution
 * @warning Les erreurs de compilation regex n'arrêtent pas le diagnostic
 * @warning Met à jour automatiquement le score du rapport à la fin
 */
void run_full_diagnostic(RuleReport* report, const char* text) {
    // === Section: Validation des paramètres critiques ===
    if (report == NULL) {
        LOG_ERROR("run_full_diagnostic: report est NULL");
        return;
    }

    if (text == NULL) {
        LOG_ERROR("run_full_diagnostic: text est NULL");
        // Marquer toutes les règles comme échouées
        for (int rule_idx = 0; rule_idx < report->rule_count; rule_idx++) {
            report->rules[rule_idx].status = STATUS_NON_CONFORME;
        }
        update_report_score(report);
        return;
    }

    size_t text_len = strlen(text);
    LOG_INFO("Démarrage du diagnostic complet sur %zu caractères\n", text_len);

    // === Section: Précompilation des regex constantes ===
    pcre2_code* forbidden_regex = NULL;
    int errornumber;
    PCRE2_SIZE erroroffset;

    forbidden_regex = pcre2_compile(
        (PCRE2_SPTR)"\\b(je|moi|mon)\\b",
        PCRE2_ZERO_TERMINATED,
        PCRE2_CASELESS,
        &errornumber,
        &erroroffset,
        NULL
    );

    if (forbidden_regex == NULL) {
        PCRE2_UCHAR errbuffer[120];
        pcre2_get_error_message(errornumber, errbuffer, sizeof(errbuffer));
        LOG_ERROR("Échec compilation regex interdite: %s (offset %zu)", 
                (char*)errbuffer, erroroffset);
    } else {
        LOG_DEBUG("Regex interdite précompilée avec succès\n");
    }

    // === Section: Exécution de chaque règle ===
    for (int rule_idx = 0; rule_idx < report->rule_count; rule_idx++) {
        Rule* current_rule = &report->rules[rule_idx];

        LOG_INFO("Exécution de la règle %s [%s]", current_rule->id, current_rule->check_type);

        // Dispatcher basé sur le type de vérification
        if (strcmp(current_rule->check_type, "section_exists") == 0) {
            // Vérification de structure : présence de la section "Introduction"
            current_rule->status = check_section_exists(text, "Introduction");
        } 
        else if (strcmp(current_rule->check_type, "regex_forbidden") == 0) {
            // Vérification de style : mots interdits via regex
            if (forbidden_regex == NULL) {
                LOG_WARN("Règle %s: regex non compilée, marquée comme avertissement", 
                        current_rule->id);
                current_rule->status = STATUS_AVERTISSEMENT;
            } else {
                current_rule->status = check_regex_forbidden_optimized(text, text_len, forbidden_regex);
            }
        }
        else if (strcmp(current_rule->check_type, "llm_semantic") == 0) {
            // Placeholder pour l'analyse sémantique via LLM
            LOG_INFO("Règle %s: analyse LLM non implémentée (placeholder)", current_rule->id);
            current_rule->status = STATUS_EN_COURS;
        }
        else {
            // Type de vérification inconnu
            LOG_WARN("Règle %s: type de vérification inconnu '%s'", 
                    current_rule->id, current_rule->check_type);
            current_rule->status = STATUS_NON_CONFORME;
        }

        LOG_DEBUG("Résultat règle %s: %s\n", current_rule->id,
                (current_rule->status == STATUS_CONFORME) ? "CONFORME" : 
                (current_rule->status == STATUS_EN_COURS) ? "EN_COURS" : "NON_CONFORME");
    }

    // === Section: Nettoyage des ressources précompilées ===
    if (forbidden_regex != NULL) {
        pcre2_code_free(forbidden_regex);
        LOG_DEBUG("Regex précompilée nettoyée");
    }

    // === Section: Mise à jour du score ===
    update_report_score(report);
    LOG_INFO("Diagnostic terminé, score mis à jour: %d/%d", 
            report->rules_ok, report->rule_count);
}

/**
 * @brief Affiche le rapport de conformité académique formaté
 * 
 * Affiche un rapport structuré montrant le score global et les détails
 * des règles non-conformes. Les règles conformes peuvent être affichées
 * en mode verbeux (activable via un paramètre futur).
 *
 * @param[in] report Rapport à afficher (ne doit pas être NULL)
 * @return void
 * @note Vérifie la validité du rapport avant affichage
 * @warning Les messages sont envoyés à stdout (printf)
 */
void print_compliance_report(RuleReport* report) {
    // === Section: Validation ===
    if (report == NULL) {
        LOG_ERROR("print_compliance_report: report est NULL");
        return;
    }

    // === Section: Affichage de l'en-tête et du score ===
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║   RAPPORT DE CONFORMITÉ ACADÉMIQUE             ║\n");
    printf("║────────────────────────────────────────────────║\n");
    printf("║   Score global: %d / %d règles respectées", report->rules_ok, report->rule_count);
    printf("        ║\n");
    printf("╚════════════════════════════════════════════════╝\n");

    // === Section: Affichage des détails ===
    int error_count = 0;
    int warning_count = 0;

    for (int rule_idx = 0; rule_idx < report->rule_count; rule_idx++) {
        Rule* current_rule = &report->rules[rule_idx];

        if (current_rule->status != STATUS_CONFORME) {
            // Déterminer le type d'affichage
            const char* status_marker;
            const char* severity_text;

            if (current_rule->severity == SEVERITY_ERROR) {
                status_marker = "❌";
                severity_text = "ERREUR";
                error_count++;
            } else {
                status_marker = "⚠️ ";
                severity_text = "AVERTISSEMENT";
                warning_count++;
            }

            printf("%s [%s] %s\n", status_marker, severity_text, current_rule->description);
        }
    }

    // === Section: Affichage du résumé ===
    printf("\n");
    if (error_count == 0 && warning_count == 0) {
        printf("✅ Excellent! Aucun problème détecté.\n");
    } else {
        printf("Résumé: %d erreur(s), %d avertissement(s)\n", error_count, warning_count);
    }
    printf("════════════════════════════════════════════════\n");
    printf("\n");
}