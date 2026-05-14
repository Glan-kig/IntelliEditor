#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "../../include/rules.h"

/* ============================================================================
 * MACROS ET CONSTANTES POUR LA MAINTENABILITÉ
 * ============================================================================ */

#define LOG_ERROR(msg, ...) fprintf(stderr, "[ERROR] " msg "\n", ##__VA_ARGS__)
#define LOG_WARN(msg, ...)  fprintf(stderr, "[WARN]  " msg "\n", ##__VA_ARGS__)
#define LOG_INFO(msg, ...)  fprintf(stderr, "[INFO]  " msg "\n", ##__VA_ARGS__)

#define FILE_READ_ERROR     "échec lecture fichier '%s'"
#define JSON_PARSE_ERROR    "JSON invalide dans '%s'"
#define JSON_KEY_ERROR      "clé '%s' manquante ou invalide"
#define RULE_FIELD_ERROR    "règle %d: champ '%s' manquant ou invalide"
#define MEMORY_ALLOC_ERROR  "échec allocation mémoire (%s)"

/* ============================================================================
 * FONCTION UTILITAIRE : read_file()
 * ============================================================================ */

/**
 * @brief Lit un fichier entier et retourne son contenu dans une chaîne allouée
 * 
 * Utilise l'allocation dynamique pour charger le fichier en mémoire. Le
 * contenu est nullifié à la fin pour assurer une chaîne C valide.
 * 
 * Gère les erreurs I/O robustement :
 * - Vérification des appels fseek/ftell
 * - Validation de la taille du fichier
 * - Vérification que fread a lu le nombre attendu d'octets
 * - Nettoyage mémoire en cas d'erreur
 *
 * @param[in] filename Chemin vers le fichier à lire
 * @return Pointeur vers le buffer alloué contenant le contenu, ou NULL en cas d'erreur
 * @note L'appelant doit libérer la mémoire avec free()
 * @warning Retourne NULL en cas d'erreur d'ouverture, taille invalide, ou lecture
 */
char* read_file(const char* filename) {
    // === Section: Validation des paramètres ===
    if (filename == NULL) {
        LOG_ERROR("read_file: filename est NULL");
        return NULL;
    }

    // === Section: Ouverture du fichier ===
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        LOG_ERROR(FILE_READ_ERROR, filename);
        return NULL;
    }

    // === Section: Déterminer la taille du fichier ===
    // Aller à la fin du fichier
    if (fseek(file, 0, SEEK_END) != 0) {
        LOG_ERROR("read_file: fseek SEEK_END échoué pour '%s'", filename);
        fclose(file);
        return NULL;
    }

    long file_length = ftell(file);
    
    // Vérifier que ftell a réussi (-1L indique une erreur)
    if (file_length < 0) {
        LOG_ERROR("read_file: ftell échoué pour '%s'", filename);
        fclose(file);
        return NULL;
    }

    // Revenir au début du fichier
    if (fseek(file, 0, SEEK_SET) != 0) {
        LOG_ERROR("read_file: fseek SEEK_SET échoué pour '%s'", filename);
        fclose(file);
        return NULL;
    }

    LOG_DEBUG("Taille du fichier '%s': %ld octets\n", filename, file_length);

    // === Section: Allocation mémoire ===
    // Toujours allouer au moins 1 octet pour la terminaison NULL
    char* file_buffer = malloc((size_t)file_length + 1);
    if (file_buffer == NULL) {
        LOG_ERROR(MEMORY_ALLOC_ERROR, "lecture fichier");
        fclose(file);
        return NULL;
    }

    // === Section: Lecture du fichier ===
    // Pour un fichier vide, fread retournera 0, ce qui est correct
    if (file_length > 0) {
        size_t bytes_read = fread(file_buffer, 1, (size_t)file_length, file);
        
        // Vérifier que nous avons bien lu le nombre d'octets attendu
        if (bytes_read != (size_t)file_length) {
            LOG_ERROR("read_file: lecture incomplète '%s' (%zu/%ld octets lus)", 
                    filename, bytes_read, file_length);
            free(file_buffer);
            fclose(file);
            return NULL;
        }
    }

    // === Section: Finalisation ===
    // Nullifier la chaîne (fonctionne même si file_length == 0)
    file_buffer[file_length] = '\0';

    // Vérifier que le fichier a bien été lu entièrement
    int read_check = fgetc(file);
    if (read_check != EOF) {
        LOG_WARN("read_file: fichier '%s' contient plus de données que prévu", filename);
    }

    // === Section: Nettoyage des ressources ===
    if (fclose(file) != 0) {
        LOG_WARN("read_file: fclose échoué pour '%s'", filename);
        // On ne retourne pas NULL ici car le fichier a été lu avec succès
    }

    LOG_INFO("Fichier '%s' lu avec succès (%ld octets)\n", filename, file_length);
    return file_buffer;
}

/* ============================================================================
 * FONCTION PRINCIPALE : load_rules()
 * ============================================================================ */

/**
 * @brief Charge les règles depuis un fichier JSON avec gestion mémoire robuste
 * 
 * Lit un fichier JSON, parse le tableau "rules", et initialise une structure
 * RuleReport. Chaque règle est validée : les champs obligatoires (id,
 * description, check_type, severity) sont vérifiés. En cas d'erreur à tout
 * point du processus, les ressources sont nettoyées proprement et NULL est
 * retourné.
 *
 * Format JSON attendu:
 * {
 *   "rules": [
 *     {
 *       "id": "R001",
 *       "description": "Vérifier introduction",
 *       "check_type": "section_exists",
 *       "severity": "error"
 *     }
 *   ]
 * }
 *
 * @param[in] filename Chemin vers le fichier JSON contenant les règles
 * @return Pointeur vers RuleReport alloué et initialisé, ou NULL en cas d'erreur
 * @note L'appelant doit libérer le RuleReport et ses membres avec free()
 * @warning Les messages d'erreur sont envoyés à stderr pour le debugging
 * @warning En cas d'erreur, vérifier stderr pour les détails
 */
RuleReport* load_rules(const char* filename) {
    // === Section: Validation des paramètres d'entrée ===
    if (filename == NULL) {
        LOG_ERROR("load_rules: filename est NULL");
        return NULL;
    }

    // === Section: Lecture du fichier ===
    char* file_content = read_file(filename);
    if (file_content == NULL) {
        LOG_ERROR(FILE_READ_ERROR, filename);
        return NULL;
    }

    // === Section: Parsing JSON ===
    cJSON* json_root = cJSON_Parse(file_content);
    if (json_root == NULL) {
        LOG_ERROR(JSON_PARSE_ERROR, filename);
        goto cleanup_file_content;
    }

    // === Section: Allocation de la structure RuleReport ===
    RuleReport* rule_report = malloc(sizeof(RuleReport));
    if (rule_report == NULL) {
        LOG_ERROR(MEMORY_ALLOC_ERROR, "RuleReport");
        goto cleanup_json;
    }

    // Initialisation sécurisée des membres (prévient les accès invalides)
    rule_report->rules = NULL;
    rule_report->rule_count = 0;
    rule_report->rules_ok = 0;

    // === Section: Extraction et validation du tableau "rules" ===
    cJSON* rules_array = cJSON_GetObjectItem(json_root, "rules");
    if (rules_array == NULL || !cJSON_IsArray(rules_array)) {
        LOG_ERROR(JSON_KEY_ERROR, "rules");
        goto cleanup_report;
    }

    rule_report->rule_count = cJSON_GetArraySize(rules_array);
    if (rule_report->rule_count == 0) {
        LOG_WARN("aucune règle trouvée dans le fichier");
        // Pas d'erreur critique : on retourne un rapport vide
    } else {
        // === Section: Allocation du tableau de règles ===
        rule_report->rules = malloc(sizeof(Rule) * rule_report->rule_count);
        if (rule_report->rules == NULL) {
            LOG_ERROR(MEMORY_ALLOC_ERROR, "tableau de règles");
            goto cleanup_report;
        }

        // === Section: Remplissage du tableau de règles ===
        int rule_index = 0;
        cJSON* rule_item = NULL;

        cJSON_ArrayForEach(rule_item, rules_array) {
            Rule* current_rule = &rule_report->rules[rule_index];

            // Extraction du champ "id"
            cJSON* id_item = cJSON_GetObjectItem(rule_item, "id");
            if (id_item == NULL || !cJSON_IsString(id_item)) {
                LOG_ERROR(RULE_FIELD_ERROR, rule_index, "id");
                goto cleanup_rules;
            }
            strncpy(current_rule->id, id_item->valuestring, 
                    sizeof(current_rule->id) - 1);
            current_rule->id[sizeof(current_rule->id) - 1] = '\0';

            // Extraction du champ "description"
            cJSON* description_item = cJSON_GetObjectItem(rule_item, "description");
            if (description_item == NULL || !cJSON_IsString(description_item)) {
                LOG_ERROR(RULE_FIELD_ERROR, rule_index, "description");
                goto cleanup_rules;
            }
            strncpy(current_rule->description, description_item->valuestring, 
                    sizeof(current_rule->description) - 1);
            current_rule->description[sizeof(current_rule->description) - 1] = '\0';

            // Extraction du champ "check_type"
            cJSON* check_type_item = cJSON_GetObjectItem(rule_item, "check_type");
            if (check_type_item == NULL || !cJSON_IsString(check_type_item)) {
                LOG_ERROR(RULE_FIELD_ERROR, rule_index, "check_type");
                goto cleanup_rules;
            }
            strncpy(current_rule->check_type, check_type_item->valuestring, 
                    sizeof(current_rule->check_type) - 1);
            current_rule->check_type[sizeof(current_rule->check_type) - 1] = '\0';

            // Extraction et conversion du champ "severity"
            cJSON* severity_item = cJSON_GetObjectItem(rule_item, "severity");
            if (severity_item == NULL || !cJSON_IsString(severity_item)) {
                LOG_WARN("règle %d: 'severity' manquante, défaut INFO", rule_index);
                current_rule->severity = SEVERITY_INFO;
            } else {
                const char* severity_str = severity_item->valuestring;
                if (strcmp(severity_str, "error") == 0) {
                    current_rule->severity = SEVERITY_ERROR;
                } else if (strcmp(severity_str, "warning") == 0) {
                    current_rule->severity = SEVERITY_WARNING;
                } else {
                    current_rule->severity = SEVERITY_INFO;
                }
            }

            // Extraction du champ "parameter" (optionnel)
            cJSON* parameter_item = cJSON_GetObjectItem(rule_item, "parameter");
            if (parameter_item != NULL && cJSON_IsString(parameter_item)) {
                current_rule->parameter = strdup(parameter_item->valuestring);
                if (current_rule->parameter == NULL) {
                    LOG_ERROR(MEMORY_ALLOC_ERROR, "parameter (règle %d)", rule_index);
                    goto cleanup_rules;
                }
                LOG_DEBUG("Paramètre chargé pour règle %d: %s", rule_index, 
                        (char*)current_rule->parameter);
            } else {
                current_rule->parameter = NULL;
                LOG_DEBUG("Règle %d: pas de paramètre défini", rule_index);
            }

            // Initialisation des champs restants
            current_rule->status = STATUS_EN_COURS;

            LOG_INFO("Règle chargée: %s [%s]", current_rule->id, current_rule->check_type);
            rule_index++;
        }
    }

    // === Section: Nettoyage et retour du résultat (succès) ===
    LOG_INFO("Chargement réussi: %d règles", rule_report->rule_count);
    cJSON_Delete(json_root);
    free(file_content);
    return rule_report;

    /* === Section: Labels de nettoyage pour gestion d'erreurs ===
     * Ces labels garantissent que les ressources sont libérées
     * proprement en cas d'erreur, dans le bon ordre inverse d'allocation.
     */

cleanup_rules:
    // Libérer les paramètres alloués avec strdup()
    if (rule_report->rules != NULL) {
        for (int cleanup_idx = 0; cleanup_idx < rule_report->rule_count; cleanup_idx++) {
            if (rule_report->rules[cleanup_idx].parameter != NULL) {
                free(rule_report->rules[cleanup_idx].parameter);
                rule_report->rules[cleanup_idx].parameter = NULL;
            }
        }
    }
    free(rule_report->rules);

cleanup_report:
    free(rule_report);

cleanup_json:
    cJSON_Delete(json_root);

cleanup_file_content:
    free(file_content);

    return NULL;
}