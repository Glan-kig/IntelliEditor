#include "rules.h"
#include <stdlib.h>

// Stub simplifié pour tester l'UI sans cJSON
RuleReport* load_rules(const char* filename) {
    RuleReport* report = malloc(sizeof(RuleReport));
    if (report) {
        report->rules = NULL;
        report->rule_count = 0;
        report->rules_ok = 0;
    }
    return report;
}

void free_rule_report(RuleReport* report) {
    if (report) {
        free(report->rules);
        free(report);
    }
}

void print_compliance_report(RuleReport* report) {
    // Stub
}

void update_report_score(RuleReport* report) {
    // Stub
}

void run_full_diagnostic(RuleReport* report, const char* text) {
    // Stub
}

void run_rule_engine(RuleReport* report, const char* current_text) {
    // Stub
}

RuleStatus check_section_exists(const char* document_text, const char* section_name) {
    return STATUS_CONFORME;
}

RuleStatus check_regex_forbidden(const char* document_text, const char* pattern) {
    return STATUS_CONFORME;
}
