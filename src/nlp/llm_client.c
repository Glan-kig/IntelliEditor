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
        cJSON *root = cJSON_CreateObject();
        
        // On augmente la taille pour accueillir les exemples du Few-Shot
        char full_prompt[8192] = {0}; 
        
        // ON APPELLE LA FONCTION DE TEMPLATE (déclarée dans llm_prompts.h)
        prepare_prompt(full_prompt, sizeof(full_prompt), instruction, section_text);

        cJSON_AddStringToObject(root, "prompt", full_prompt);
        
        //  Paramètres stricts pour brider le modèle 1B
        cJSON_AddNumberToObject(root, "n_predict", 10);
        cJSON_AddNumberToObject(root, "temperature", 0.0);
        cJSON_AddNumberToObject(root, "top_k", 1);
        // Force le modèle à choisir STRICTEMENT entre les deux chaînes exactes
        cJSON_AddStringToObject(root, "grammar", "root ::= \"CONFORME\" | \"NON_CONFORME\"");

        char *json_body = cJSON_Print(root);
        
        // Configuration des Headers (Important pour le JSON)
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        if (!json_body) {
            fprintf(stderr, "[ERROR] ask_llm_semantic_check: échec cJSON_Print\n");
            cJSON_Delete(root);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return STATUS_NON_CONFORME;
        }
        
        if (!headers) {
            fprintf(stderr, "[ERROR] ask_llm_semantic_check: échec curl_slist_append\n");
            free(json_body);
            cJSON_Delete(root);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return STATUS_NON_CONFORME;
        }

        // Configuration de la requête
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/completion");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_buffer);

        // Exécution de l'appel
        CURLcode res = curl_easy_perform(curl);
        
    if (res == CURLE_OK) {
    printf("Réponse brute : %s\n", response_buffer);

    // On cherche la première occurrence de chaque mot
        char *pos_conforme = strstr(response_buffer, "CONFORME");
        char *pos_non_conforme = strstr(response_buffer, "NON_CONFORME");

        // Si CONFORME apparaît en premier, ou s'il n'y a pas du tout de NON_CONFORME
        if (pos_conforme != NULL && (pos_non_conforme == NULL || pos_conforme < pos_non_conforme)) {
        status = STATUS_CONFORME;
        } else {
        status = STATUS_NON_CONFORME;
       }
    }

        // Nettoyage rigoureux
        curl_slist_free_all(headers);
        cJSON_Delete(root);
        free(json_body);
        curl_easy_cleanup(curl);
    }
    return status;
}