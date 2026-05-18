/**
 * @file config.h
 * @brief Parser de fichier INI pour configuration utilisateur.
 * @author DEV-A
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

/** Structure opaque de configuration. */
typedef struct Config Config;

/** @brief Charge un fichier .ini et retourne la config (NULL si échec). */
Config *config_load(const char *filepath);

/** @brief Libère la configuration. */
void config_free(Config *cfg);

/**
 * @brief Récupère une valeur chaîne.
 * @param section Nom de section (ex: "editor").
 * @param key Clé.
 * @param default_value Valeur par défaut si absente.
 */
const char *config_get_string(const Config *cfg, const char *section,
                              const char *key, const char *default_value);

/** @brief Récupère une valeur entière. */
int config_get_int(const Config *cfg, const char *section,
                   const char *key, int default_value);

/** @brief Récupère un booléen (true/false/yes/no/1/0). */
bool config_get_bool(const Config *cfg, const char *section,
                     const char *key, bool default_value);

#endif /* CONFIG_H */
