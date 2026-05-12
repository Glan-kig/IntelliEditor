#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "../../include/rules.h"

// Callback amélioré pour éviter les débordements
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    char *buffer = (char *)userdata;
    size_t total_size = size * nmemb;
    
    // On vérifie qu'on a assez de place (9999 pour garder le \0 final)
    size_t current_len = strlen(buffer);
    if (current_len + total_size < 9999) {
        strncat(buffer, (char *)ptr, total_size);
    }
    return total_size;
} 

RuleStatus ask_llm_semantic_check(const char* section_text, const char* instruction) 
{
    CURL *curl = curl_easy_init();
    char response_buffer[10000] = {0}; 
    RuleStatus status = STATUS_NON_CONFORME;

    if(curl) {
        // Construction du JSON
        cJSON *root = cJSON_CreateObject();
        char full_prompt[4096]; // Augmenté pour éviter les troncatures de texte long
        
        snprintf(full_prompt, sizeof(full_prompt), 
                 "Instruction: %s\nTexte: %s\nRéponse (CONFORME/NON_CONFORME):", 
                 instruction, section_text);

        cJSON_AddStringToObject(root, "prompt", full_prompt);
        cJSON_AddNumberToObject(root, "n_predict", 10);

        char *json_body = cJSON_Print(root);

        // Configuration des Headers (Important pour le JSON)
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // Configuration de la requête
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/completion");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_buffer);

        // Exécution de l'appel
        CURLcode res = curl_easy_perform(curl);
        
        if(res == CURLE_OK) {
            // Log pour debug (optionnel)
            printf("Réponse brute : %s\n", response_buffer);

            if (strstr(response_buffer, "CONFORME") != NULL && strstr(response_buffer, "NON_CONFORME") == NULL) {
                status = STATUS_CONFORME;
            } else {
                status = STATUS_NON_CONFORME;
            }
        } else {
            fprintf(stderr, "Erreur CURL: %s\n", curl_easy_strerror(res));
        }

        // Nettoyage rigoureux
        curl_slist_free_all(headers);
        cJSON_Delete(root);
        free(json_body);
        curl_easy_cleanup(curl);
    }
    return status;
}