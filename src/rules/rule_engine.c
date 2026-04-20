#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../../include/rules.h"

// Vérificateur simple : cherche si une ligne contient le titre de la section
RuleStatus check_section_exists(const char* document_text, const char* section_name) {
    if (document_text == NULL || section_name == NULL) {
        return STATUS_NON_CONFORME;
    }

    // On cherche la présence du nom de la section (ex: "Introduction")
    // Dans une version plus avancée, on pourrait vérifier si c'est un titre H1/H2
    char* found = strstr(document_text, section_name);

    if (found != NULL) {
        return STATUS_CONFORME;
    }
        return STATUS_NON_CONFORME;
    }

// Fonction de pilotage qui parcourt le rapport
void run_rule_engine(RuleReport* report, const char* current_text) {
    for (int i = 0; i < report->rule_count; i++) {
        Rule* r = &report->rules[i];

        // On compare le type de vérification défini dans le JSON
        if (strcmp(r->check_type, "section_exists") == 0) {
            // On utilise la description ou un paramètre pour le nom de la section
            // Ici, pour l'exemple, on cherche le mot-clé stocké
            r->status = check_section_exists(current_text, "Introduction");
        }
    }
}