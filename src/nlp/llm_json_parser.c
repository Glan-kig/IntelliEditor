#include <cjson/cJSON.h>
#include "../../include/rules.h"

// Ligne par ligne : On cherche la clé "content" dans le JSON
RuleStatus parse_llama_json(const char* raw_json) {
    cJSON *json = cJSON_Parse(raw_json);
    cJSON *content = cJSON_GetObjectItem(json, "content");
    
    RuleStatus s = STATUS_NON_CONFORME;
    if (content && strstr(content->valuestring, "CONFORME")) {
        s = STATUS_CONFORME;
    }
    cJSON_Delete(json);
    return s;
}