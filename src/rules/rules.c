#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "../../include/rules.h"

// Fonction utilitaire pour lire tout un fichier en mémoire
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

RuleReport* load_rules(const char* filename) {
    char* data = read_file(filename);
    if (!data) return NULL;

    cJSON* json = cJSON_Parse(data);
    if (!json) {
        free(data);
        return NULL;
    }

    RuleReport* report = malloc(sizeof(RuleReport));
    cJSON* rules_array = cJSON_GetObjectItem(json, "rules");
    report->rule_count = cJSON_GetArraySize(rules_array);
    report->rules = malloc(sizeof(Rule) * report->rule_count);
    report->rules_ok = 0;

    int i = 0;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, rules_array) {
        Rule* r = &report->rules[i];
        
        // Extraction des données JSON [cite: 211, 215, 213]
        strncpy(r->id, cJSON_GetObjectItem(item, "id")->valuestring, 10);
        strncpy(r->description, cJSON_GetObjectItem(item, "description")->valuestring, 256);
        strncpy(r->check_type, cJSON_GetObjectItem(item, "check_type")->valuestring, 32);
        
        // Conversion de la sévérité (string -> enum) [cite: 213]
        char* sev = cJSON_GetObjectItem(item, "severity")->valuestring;
        if (strcmp(sev, "error") == 0) r->severity = SEVERITY_ERROR;
        else if (strcmp(sev, "warning") == 0) r->severity = SEVERITY_WARNING;
        else r->severity = SEVERITY_INFO;

        r->status = STATUS_EN_COURS; // État initial [cite: 170]
        i++;
    }

    cJSON_Delete(json);
    free(data);
    return report;
}