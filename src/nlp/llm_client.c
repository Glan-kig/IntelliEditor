#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "../../include/rules.h"

// Fonction pour stocker la reponse de l'API ou seveur

size_t write_callback(void *ptr, size_t size, size_t nmemb, char *data) {
    strcat(data, (char *)ptr);
    return size * nmemb;
} 

RuleStatus ask_llm_semantic_check(const char* section_text, const char* instruction) 
{
    CURL *curl;
    CURLcode res;
    char response_buffer[10000] = {0}; // Buffer pour stocker la reponse de l'API ou seveur
    RuleStatus status = STATUS_NON_CONFORME;

    curl = curl_easy_init();
    if(curl) {
        //construction du JSON de requete pour llama.cpp
        cJSON *root = cJSON_CreateObject();
        char full_prompt[2048];
        snprintf(full_prompt, sizeof(full_prompt), "instruction: %s\nTexte: %s\nRepondre par CONFORME ou NON_CONFORME", instruction, section_text);

        cJSON_AddStringToObject(root, "prompt", full_prompt);
        cJSON_AddNumberToObject(root, "n_predict", 10);

        char *json_body= cJSON_Print(root);

        // Configuration de la requete HTTP / de l'appel

        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/completion");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_buffer);

        // Execution de la requete

        res = curl_easy_perform(curl);

        if(res==CURLE_OK) {
            // Analyse de la reponse pour determiner le statut
            if (strstr(response_buffer, "CONFORME") != NULL) {
                status = STATUS_CONFORME;
            } else if (strstr(response_buffer, "NON_CONFORME") != NULL) {
                status = STATUS_NON_CONFORME;
            }
        } 
        // Nettoyage
        cJSON_Delete(root);
        free(json_body);
        curl_easy_cleanup(curl);
    }
    return status;
}