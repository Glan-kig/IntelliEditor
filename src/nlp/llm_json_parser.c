#include <cjson/cJSON.h>
#include <string.h>
#include <stdio.h>
#include "../../include/rules.h"

// On cherche la clé "content" dans le JSON
RuleStatus parse_llama_json(const char* raw_json) {
    if (!raw_json) return STATUS_NON_CONFORME;

    cJSON *json = cJSON_Parse(raw_json);
    if (!json) {
        fprintf(stderr, "[ERROR] parse_llama_json: JSON invalide\n");
        return STATUS_NON_CONFORME;
    }

    RuleStatus s = STATUS_NON_CONFORME;
    cJSON *content = cJSON_GetObjectItem(json, "content");
    if (content && cJSON_IsString(content) && content->valuestring) {
        if (strstr(content->valuestring, "CONFORME") != NULL) {
            s = STATUS_CONFORME;
        }
    } else {
        fprintf(stderr, "[WARN] parse_llama_json: contenu manquant ou non texte\n");
    }

    cJSON_Delete(json);
    return s;
}