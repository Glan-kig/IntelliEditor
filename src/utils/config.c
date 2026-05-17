#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <cjson/cJSON.h>
#include "../../include/config.h"

int load_config(const char *filename, AppConfig *config) {
    if (!filename || !config) return 0;

    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Impossible d'ouvrir le fichier de config '%s': %s\n", filename, strerror(errno));
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Erreur lors du positionnement dans le fichier de config\n");
        fclose(file);
        return 0;
    }
    long length = ftell(file);
    if (length < 0) {
        fprintf(stderr, "Impossible d'obtenir la taille du fichier de config\n");
        fclose(file);
        return 0;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Erreur lors du repositionnement du fichier de config\n");
        fclose(file);
        return 0;
    }

    char *data = malloc((size_t)length + 1);
    if (!data) {
        fprintf(stderr, "Échec d'allocation mémoire pour la lecture de la config\n");
        fclose(file);
        return 0;
    }

    size_t total_read = 0;
    while (total_read < (size_t)length) {
        size_t chunk = fread(data + total_read, 1, (size_t)length - total_read, file);
        if (chunk == 0) {
            if (ferror(file)) {
                fprintf(stderr, "Erreur de lecture du fichier de config\n");
                free(data);
                fclose(file);
                return 0;
            }
            break;
        }
        total_read += chunk;
    }
    data[total_read] = '\0';
    fclose(file);

    cJSON *root = cJSON_Parse(data);
    if (!root) {
        fprintf(stderr, "Erreur de format JSON dans la config\n");
        free(data);
        return 0;
    }

    /* Valeurs par défaut */
    config->server_url[0] = '\0';
    config->rules_path[0] = '\0';
    config->timeout_seconds = 30;
    config->temperature = 1.0f;

    cJSON *item = NULL;
    item = cJSON_GetObjectItem(root, "server_url");
    if (cJSON_IsString(item) && item->valuestring) {
        snprintf(config->server_url, sizeof(config->server_url), "%s", item->valuestring);
    }

    item = cJSON_GetObjectItem(root, "rules_path");
    if (cJSON_IsString(item) && item->valuestring) {
        snprintf(config->rules_path, sizeof(config->rules_path), "%s", item->valuestring);
    }

    item = cJSON_GetObjectItem(root, "timeout");
    if (cJSON_IsNumber(item)) {
        config->timeout_seconds = item->valueint;
    }

    item = cJSON_GetObjectItem(root, "temperature");
    if (cJSON_IsNumber(item)) {
        config->temperature = (float)item->valuedouble;
    }

    cJSON_Delete(root);
    free(data);
    return 1;
}