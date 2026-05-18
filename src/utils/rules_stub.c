#include "../../include/rules.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Stub simplifié : charge/retourne un RuleReport minimal */

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
    if (report->rules) free(report->rules);
    if (report->issues) free(report->issues);
    free(report);
}

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

    for (int i = 0; i < issue_count; ++i) {
        report->issues[i].line = 1 + i;
        report->issues[i].offset = i * 10;
        snprintf(report->issues[i].type, sizeof(report->issues[i].type), "stub");
        snprintf(report->issues[i].message, sizeof(report->issues[i].message), "Issue factice %d", i + 1);
    }

    return report;
}

/* Fonctions non utilisées dans le stub, prévues pour futur moteur */
void run_rule_engine(RuleReport* report, const char* current_text) {
    (void)report;
    (void)current_text;
    /* Implémentation future */
}
