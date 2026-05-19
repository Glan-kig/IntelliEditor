#include "../../include/rules.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
    report->issue_count = 3;
    report->issues = calloc(3, sizeof(RuleIssue));
    for (int i = 0; i < 3; i++) {
        report->issues[i].line = i + 1;
        snprintf(report->issues[i].type, sizeof(report->issues[i].type), "stub");
        snprintf(report->issues[i].message, sizeof(report->issues[i].message), "Issue factice %d", i + 1);
        report->issues[i].offset = i * 20;
    }
    return report;
}
