#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "../../include/rules.h"

/**
 * @brief Lit un fichier entier et retourne son contenu dans une chaîne allouée
 * @param filename Chemin vers le fichier à lire
 * @return buffer contenant le contenu du fichier
 */
char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(length + 1);
    if (buffer) {
        fread(buffer, 1, length, file);
        buffer[length] = '\0';
    }
    fclose(file);
    return buffer;
}

/**
 * @brief Charge les règles depuis un fichier JSON avec gestion mémoire robuste
 * @param filename Chemin vers le fichier JSON
 * @return Pointeur vers RuleReport ou NULL en cas d'erreur
 */
RuleReport* load_rules(const char* filename) {
    // Validation des paramètres
    if (filename == NULL) {
        fprintf(stderr, "[ERROR] load_rules: filename est NULL\n");
        return NULL;
    }
    
    char* data = read_file(filename);
    if (!data) {
        fprintf(stderr, "[ERROR] load_rules: échec lecture fichier '%s'\n", filename);
        return NULL;
    }

    cJSON* json = cJSON_Parse(data);
    if (!json) {
        fprintf(stderr, "[ERROR] load_rules: JSON invalide dans '%s'\n", filename);
        free(data);
        return NULL;
    }

    RuleReport* report = malloc(sizeof(RuleReport));
    if (!report) {
        fprintf(stderr, "[ERROR] load_rules: échec allocation RuleReport\n");
        goto cleanup_json;  // Nettoyer et sortir
    }

    // Initialisation sécurisée
    report->rules = NULL;
    report->rule_count = 0;
    report->rules_ok = 0;

    cJSON* rules_array = cJSON_GetObjectItem(json, "rules");
    if (!rules_array || !cJSON_IsArray(rules_array)) {
        fprintf(stderr, "[ERROR] load_rules: clé 'rules' manquante ou invalide\n");
        goto cleanup_report;
    }

    report->rule_count = cJSON_GetArraySize(rules_array);
    if (report->rule_count == 0) {
        fprintf(stderr, "[WARN] load_rules: aucune règle trouvée\n");
        // Pas d'erreur, mais rapport vide
    }

    report->rules = malloc(sizeof(Rule) * report->rule_count);
    if (!report->rules) {
        fprintf(stderr, "[ERROR] load_rules: échec allocation règles (%d éléments)\n", report->rule_count);
        goto cleanup_report;
    }

    // Parcours et remplissage des règles
    int i = 0;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, rules_array) {
        Rule* r = &report->rules[i];
        
        // Vérifications pour chaque champ JSON
        cJSON* id_item = cJSON_GetObjectItem(item, "id");
        if (!id_item || !cJSON_IsString(id_item)) {
            fprintf(stderr, "[ERROR] load_rules: règle %d sans 'id' valide\n", i);
            goto cleanup_rules;
        }
        strncpy(r->id, id_item->valuestring, 10);
        
        cJSON* desc_item = cJSON_GetObjectItem(item, "description");
        if (!desc_item || !cJSON_IsString(desc_item)) {
            fprintf(stderr, "[ERROR] load_rules: règle %d sans 'description' valide\n", i);
            goto cleanup_rules;
        }
        strncpy(r->description, desc_item->valuestring, 256);
        
        cJSON* type_item = cJSON_GetObjectItem(item, "check_type");
        if (!type_item || !cJSON_IsString(type_item)) {
            fprintf(stderr, "[ERROR] load_rules: règle %d sans 'check_type' valide\n", i);
            goto cleanup_rules;
        }
        strncpy(r->check_type, type_item->valuestring, 32);
        
        // Conversion de la sévérité avec vérification
        cJSON* sev_item = cJSON_GetObjectItem(item, "severity");
        if (!sev_item || !cJSON_IsString(sev_item)) {
            fprintf(stderr, "[WARN] load_rules: règle %d sans 'severity', défaut INFO\n", i);
            r->severity = SEVERITY_INFO;
        } else {
            char* sev = sev_item->valuestring;
            if (strcmp(sev, "error") == 0) r->severity = SEVERITY_ERROR;
            else if (strcmp(sev, "warning") == 0) r->severity = SEVERITY_WARNING;
            else r->severity = SEVERITY_INFO;
        }

        r->status = STATUS_EN_COURS;
        r->parameter = NULL;  // Initialiser à NULL pour éviter les accès invalides
        i++;
    }

    // Succès : nettoyer et retourner
    cJSON_Delete(json);
    free(data);
    return report;

    // Labels de nettoyage pour gérer les erreurs proprement
cleanup_rules:
    free(report->rules);
cleanup_report:
    free(report);
cleanup_json:
    cJSON_Delete(json);
    free(data);
    return NULL;
}