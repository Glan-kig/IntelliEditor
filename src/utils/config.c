/**
 * @file config.c
 * @brief Parser INI minimaliste basé sur fopen/fgets.
 *
 * Format supporté :
 *   ; commentaire
 *   [section]
 *   clé = valeur
 *
 * @author DEV-A
 */
#include "config.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

#define MAX_LINE 512

/** Entrée clé/valeur dans une section. */
typedef struct Entry {
    char         *section;
    char         *key;
    char         *value;
    struct Entry *next;
} Entry;

struct Config {
    Entry *head;
};

/* --------------------------------------------------------------------------
 * Helpers internes
 * -------------------------------------------------------------------------- */

/** Supprime espaces en début/fin (modifie la chaîne in-place). */
static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

/** Duplique une chaîne avec notre allocateur. */
static char *dup_str(const char *src) {
    if (!src) return NULL;
    size_t n = strlen(src) + 1;
    char *p = MEM_ALLOC(n);
    if (p) memcpy(p, src, n);
    return p;
}

/** Ajoute une entrée en tête de liste. */
static bool add_entry(Config *cfg, const char *section,
                      const char *key, const char *value) {
    Entry *e = MEM_ALLOC(sizeof(Entry));
    if (!e) return false;
    e->section = dup_str(section);
    e->key     = dup_str(key);
    e->value   = dup_str(value);
    if (!e->section || !e->key || !e->value) {
        MEM_FREE(e->section); MEM_FREE(e->key);
        MEM_FREE(e->value);   MEM_FREE(e);
        return false;
    }
    e->next = cfg->head;
    cfg->head = e;
    return true;
}

/** Recherche une entrée par section+clé. */
static const Entry *find_entry(const Config *cfg, const char *section,
                               const char *key) {
    for (const Entry *e = cfg->head; e; e = e->next) {
        if (strcasecmp(e->section, section) == 0 &&
            strcasecmp(e->key, key) == 0)
            return e;
    }
    return NULL;
}

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

Config *config_load(const char *filepath) {
    if (!filepath) return NULL;
    FILE *f = fopen(filepath, "r");
    if (!f) return NULL;

    Config *cfg = MEM_CALLOC(1, sizeof(Config));
    if (!cfg) { fclose(f); return NULL; }

    char line[MAX_LINE];
    char current_section[128] = "global";

    while (fgets(line, sizeof(line), f)) {
        char *s = trim(line);

        /* Lignes vides ou commentaires */
        if (*s == '\0' || *s == ';' || *s == '#') continue;

        /* Section [nom] */
        if (*s == '[') {
            char *end = strchr(s, ']');
            if (!end) continue;
            *end = '\0';
            strncpy(current_section, trim(s + 1), sizeof(current_section) - 1);
            current_section[sizeof(current_section) - 1] = '\0';
            continue;
        }

        /* Paire clé = valeur */
        char *eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = trim(s);
        char *val = trim(eq + 1);
        add_entry(cfg, current_section, key, val);
    }

    fclose(f);
    return cfg;
}

void config_free(Config *cfg) {
    if (!cfg) return;
    Entry *e = cfg->head;
    while (e) {
        Entry *next = e->next;
        MEM_FREE(e->section);
        MEM_FREE(e->key);
        MEM_FREE(e->value);
        MEM_FREE(e);
        e = next;
    }
    MEM_FREE(cfg);
}

const char *config_get_string(const Config *cfg, const char *section,
                              const char *key, const char *default_value) {
    if (!cfg) return default_value;
    const Entry *e = find_entry(cfg, section, key);
    return e ? e->value : default_value;
}

int config_get_int(const Config *cfg, const char *section,
                   const char *key, int default_value) {
    const char *v = config_get_string(cfg, section, key, NULL);
    if (!v) return default_value;
    return atoi(v);
}

bool config_get_bool(const Config *cfg, const char *section,
                     const char *key, bool default_value) {
    const char *v = config_get_string(cfg, section, key, NULL);
    if (!v) return default_value;
    if (strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 ||
        strcmp(v, "1") == 0) return true;
    if (strcasecmp(v, "false") == 0 || strcasecmp(v, "no") == 0 ||
        strcmp(v, "0") == 0) return false;
    return default_value;
}
