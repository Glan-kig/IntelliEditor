#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "../../include/config.h"

int load_config(const char *filename, AppConfig *config) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Impossible d'ouvrir le fichier de config");
        return 0;
    }

    // Lire tout le fichier
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(length + 1);
    fread(data, 1, length, file);
    fclose(file);
    data[length] = '\0';

    // Parser le JSON
    cJSON *root = cJSON_Parse(data);
    if (!root) {
        fprintf(stderr, "Erreur de format JSON dans la config\n");
        free(data);
        return 0;
    }

    // Remplir la structure avec des valeurs par défaut si absent
    strncpy(config->server_url, cJSON_GetObjectItem(root, "server_url")->valuestring, 255);
    strncpy(config->rules_path, cJSON_GetObjectItem(root, "rules_path")->valuestring, 255);
    config->timeout_seconds = cJSON_GetObjectItem(root, "timeout")->valueint;
    config->temperature = (float)cJSON_GetObjectItem(root, "temperature")->valuedouble;

    cJSON_Delete(root);
    free(data);
    return 1;
}