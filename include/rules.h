#ifndef RULES_H
#define RULES_H
#include <stdbool.h>

typedef struct _GtkTextBuffer GtkTextBuffer;
typedef struct _GtkWidget GtkWidget;

typedef enum { SEVERITY_INFO, SEVERITY_WARNING, SEVERITY_ERROR } RuleSeverity;
typedef enum { STATUS_CONFORME, STATUS_AVERTISSEMENT, STATUS_NON_CONFORME, STATUS_EN_COURS } RuleStatus;

typedef struct {
    char id[10];
    char category[32];
    char description[256];
    char check_type[32];
    RuleSeverity severity;
    RuleStatus status;
    void* parameter;
} Rule;

typedef struct {
    int line;
    char type[32];
    char message[256];
    int offset;
} RuleIssue;

typedef struct {
    Rule* rules;
    int rule_count;
    int rules_ok;
    RuleIssue* issues;
    int issue_count;
} RuleReport;

RuleReport* load_rules(const char* filename);
void print_compliance_report(RuleReport* report);
RuleStatus check_section_exists(const char* document_text, const char* section_name);
RuleStatus check_regex_forbidden(const char* document_text, const char* pattern);
void update_report_score(RuleReport* report);
void run_full_diagnostic(RuleReport* report, const char* text);
void run_rule_engine(RuleReport* report, const char* current_text);
void free_rule_report(RuleReport* report);

GtkWidget* create_rules_panel(void);
RuleReport* apply_rules_to_buffer(GtkTextBuffer* buffer);

#endif
