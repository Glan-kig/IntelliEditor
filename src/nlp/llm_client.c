#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "../../include/rules.h"

// Fonction pour stocker la reponse de l'API ou seveur

size_t write_callback(void *ptr, size_t size, size_t nmemb, char *data) {
    strncat(data, (char *)ptr, 10000 - strlen(data) - 1); // Concatene la reponse dans le buffer
    return size * nmemb;
} 

RuleStatus ask_llm_semantic_check(const char* section_text, const char* instruction) 
{
    CURL *curl = curl_easy_init(); // Initialisation de CURL/la session HTTP
    char response_buffer[10000] = {0}; // Buffer pour stocker la reponse de l'API ou seveur
    RuleStatus status = STATUS_NON_CONFORME;

    curl = curl_easy_init();
    if(curl) {
        //construction du JSON de requete pour llama.cpp
        cJSON *root = cJSON_CreateObject();
        char full_prompt[2048];
        // Construction du prompt pour le LLM
        snprintf(full_prompt, sizeof(full_prompt), "instruction: %s\nTexte: %s\nRepondre par CONFORME ou NON_CONFORME", instruction, section_text);

        cJSON_AddStringToObject(root, "prompt", full_prompt);
        cJSON_AddNumberToObject(root, "n_predict", 10);// Nombre de tokens a generer/longueur de la reponse

        char *json_body= cJSON_Print(root);

        // Configuration de la requete HTTP / de l'appel

        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/completion");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_buffer);

        

        if(curl_easy_perform(curl) == CURLE_OK) {
            // Analyse de la reponse pour determiner le statut/ si il l'IA a repondu positive ou negative
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