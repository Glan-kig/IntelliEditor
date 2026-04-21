#ifndef RULES_H
#define RULES_H

#include <stdbool.h>

// Niveaux de sévérité définis dans le projet [cite: 164, 213]
typedef enum {
    SEVERITY_INFO,
    SEVERITY_WARNING,
    SEVERITY_ERROR
} RuleSeverity;

// États de conformité pour l'affichage 
typedef enum {
    STATUS_CONFORME,
    STATUS_AVERTISSEMENT,
    STATUS_NON_CONFORME,
    STATUS_EN_COURS
} RuleStatus;

// Structure d'une règle individuelle 
typedef struct {
    char id[10];               // ex: "R001" [cite: 211]
    char category[32];         // ex: "structure" [cite: 212]
    char description[256];     // Description textuelle [cite: 214]
    char check_type[32];       // ex: "section_exists" [cite: 215]
    RuleSeverity severity;     // Importance de la règle [cite: 213]
    RuleStatus status;         // État actuel de la règle 
    void* parameter;           // Paramètre flexible (string, int, ou structure) 
} Rule;

// Structure pour le rapport de conformité 
typedef struct {
    Rule* rules;               // Tableau de règles
    int rule_count;            // Nombre total de règles
    int rules_ok;              // Nombre de règles respectées [cite: 130]
} RuleReport;

// Prototype de la fonction de chargement des règles depuis un fichier JSON
RuleReport* load_rules(const char* filename);

// Prototype de la fonction d'affichage du rapport de conformité
void print_compliance_report(RuleReport* report);

// Prototypes des vérificateurs (Checkers)
RuleStatus check_section_exists(const char* document_text, const char* section_name);
RuleStatus check_regex_forbidden(const char* document_text, const char* pattern);

// Prototypes du moteur
void update_report_score(RuleReport* report);
void run_full_diagnostic(RuleReport* report, const char* text);
void run_rule_engine(RuleReport* report, const char* current_text);

#endif