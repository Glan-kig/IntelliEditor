#include "../../include/rules.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Stub simplifié (pour l'UI) : pas de parsing JSON, pas d'alloc de règles.

RuleReport* load_rules(const char* filename) {
    (void)filename;

    RuleReport* report = malloc(sizeof(RuleReport));
    if (!report) return NULL;

    report->rules = NULL;
    report->rule_count = 0;
    report->rules_ok = 0;

    report->issues = NULL;
    report->issue_count = 0;

    return report;
}

void free_rule_report(RuleReport* report) {
    if (!report) return;

    if (report->rules) {
        // Dans ce stub, parameter n'est pas alloué.
        free(report->rules);
    }

    if (report->issues) {
        free(report->issues);
    }

    free(report);
}

void print_compliance_report(RuleReport* report) {
    (void)report;
    // Stub
}

void update_report_score(RuleReport* report) {
    (void)report;
    // Stub
}

void run_full_diagnostic(RuleReport* report, const char* text) {
    (void)report;
    (void)text;
    // Stub
}

void run_rule_engine(RuleReport* report, const char* current_text) {
    (void)report;
    (void)current_text;
    // Stub
}

RuleStatus check_section_exists(const char* document_text, const char* section_name) {
    (void)document_text;
    (void)section_name;
    return STATUS_CONFORME;
}

RuleStatus check_regex_forbidden(const char* document_text, const char* pattern) {
    (void)document_text;
    (void)pattern;
    return STATUS_CONFORME;
}

// IMPORTANT: ce stub ne dépend pas de GTK.
// L'UI pourra utiliser line/offset factices.
RuleReport* apply_rules_to_buffer(GtkTextBuffer* buffer) {
    (void)buffer;

    RuleReport* report = malloc(sizeof(RuleReport));
    if (!report) return NULL;

    report->rules = NULL;
    report->rule_count = 0;
    report->rules_ok = 0;

    int issue_count = 3;
    report->issue_count = issue_count;

    report->issues = calloc((size_t)issue_count, sizeof(RuleIssue));
    if (!report->issues) {
        free(report);
        return NULL;
    }

    for (int i = 0; i < issue_count; i++) {
        report->issues[i].line = 1 + i;
        snprintf(report->issues[i].type, sizeof(report->issues[i].type), "stub");
        snprintf(report->issues[i].message, sizeof(report->issues[i].message), "Issue factice %d", i + 1);
        report->issues[i].offset = i * 20;
    }

    return report;
}

