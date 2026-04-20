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

#endif