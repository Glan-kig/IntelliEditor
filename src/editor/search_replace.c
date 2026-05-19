/**
 * @file search_replace.c
 * @brief Implémentation du moteur de recherche et remplacement.
 *
 * Fournit des fonctionnalités de recherche/remplacement avec options de casse
 * et mot entier.
 *
 * @author DEV-A
 */

#include "../../include/search_replace.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* --------------------------------------------------------------------------
 * Structure de contexte de recherche
 * -------------------------------------------------------------------------- */
struct SearchContext {
    char              *text;           // Texte original
    char              *pattern;        // Motif (peut être en minuscules pour search insensible)
    char              *text_lower;     // Texte en minuscules (pour comparaison insensible)
    SearchOptions     options;         // Options de recherche
    size_t            text_len;        // Longueur du texte
    size_t            pattern_len;     // Longueur du motif
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

/**
 * Convertit une chaîne en minuscules.
 */
char *search_to_lowercase(const char *str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char *lower = (char *)malloc(len + 1);
    if (!lower) return NULL;
    
    for (size_t i = 0; i <= len; i++) {
        lower[i] = (char)tolower((unsigned char)str[i]);
    }
    return lower;
}

/**
 * Vérifie si la position est un début de mot.
 */
bool search_is_word_boundary(const char *text, size_t pos) {
    if (pos == 0) return true;
    char c = text[pos - 1];
    return !isalnum((unsigned char)c) && c != '_';
}

/**
 * Vérifie si la position est une fin de mot.
 */
bool search_is_word_end(const char *text, size_t pos) {
    char c = text[pos];
    return c == '\0' || (!isalnum((unsigned char)c) && c != '_');
}

/**
 * Trouve un match en respectant les options.
 * Retourne la position ou (size_t)-1 si non trouvé.
 */
static size_t find_match(const char *text, size_t text_len,
                         const char *pattern, size_t pattern_len,
                         const SearchOptions *options,
                         size_t start_pos) {
    if (!text || !pattern || start_pos > text_len) return (size_t)-1;
    if (pattern_len == 0 || text_len == 0) return (size_t)-1;

    // Préparation pour recherche insensible à la casse
    char *text_search = (char *)text;
    char *pattern_search = (char *)pattern;
    char *text_lower = NULL;
    char *pattern_lower = NULL;

    if (!options->case_sensitive) {
        text_lower = search_to_lowercase(text);
        pattern_lower = search_to_lowercase(pattern);
        if (!text_lower || !pattern_lower) {
            free(text_lower);
            free(pattern_lower);
            return (size_t)-1;
        }
        text_search = text_lower;
        pattern_search = pattern_lower;
    }

    size_t result = (size_t)-1;

    // Boucle de recherche
    for (size_t i = start_pos; i <= text_len - pattern_len; i++) {
        // Comparaison du motif
        if (strncmp(&text_search[i], pattern_search, pattern_len) == 0) {
            // Vérifier les limites de mot si nécessaire
            if (options->whole_word) {
                if (!search_is_word_boundary(text_search, i) ||
                    !search_is_word_end(text_search, i + pattern_len)) {
                    continue; // Ne correspond pas au mot entier
                }
            }

            // Match trouvé!
            result = i;
            break;
        }
    }

    free(text_lower);
    free(pattern_lower);
    return result;
}

/* --------------------------------------------------------------------------
 * API Recherche
 * -------------------------------------------------------------------------- */

SearchContext *search_create(const char *text, const char *pattern,
                              const SearchOptions *options) {
    if (!text || !pattern) return NULL;

    SearchContext *ctx = (SearchContext *)malloc(sizeof(SearchContext));
    if (!ctx) return NULL;

    size_t text_len = strlen(text);
    size_t pattern_len = strlen(pattern);

    ctx->text = (char *)malloc(text_len + 1);
    ctx->pattern = (char *)malloc(pattern_len + 1);

    if (!ctx->text || !ctx->pattern) {
        free(ctx->text);
        free(ctx->pattern);
        free(ctx);
        return NULL;
    }

    strcpy(ctx->text, text);
    strcpy(ctx->pattern, pattern);
    ctx->text_len = text_len;
    ctx->pattern_len = pattern_len;
    ctx->text_lower = NULL;

    if (options) {
        ctx->options = *options;
    } else {
        ctx->options.case_sensitive = true;
        ctx->options.whole_word = false;
        ctx->options.use_regex = false;
    }

    return ctx;
}

void search_destroy(SearchContext *ctx) {
    if (!ctx) return;
    free(ctx->text);
    free(ctx->pattern);
    free(ctx->text_lower);
    free(ctx);
}

SearchResult search_find_first(SearchContext *ctx) {
    SearchResult result = {false, 0, 0, 0};
    if (!ctx) return result;

    size_t pos = find_match(ctx->text, ctx->text_len,
                            ctx->pattern, ctx->pattern_len,
                            &ctx->options, 0);

    if (pos != (size_t)-1) {
        result.found = true;
        result.start = pos;
        result.length = ctx->pattern_len;
        result.match_count = search_count_matches(ctx);
    }

    return result;
}

SearchResult search_find_next(SearchContext *ctx, size_t start_pos) {
    SearchResult result = {false, 0, 0, 0};
    if (!ctx) return result;

    size_t pos = find_match(ctx->text, ctx->text_len,
                            ctx->pattern, ctx->pattern_len,
                            &ctx->options, start_pos);

    if (pos != (size_t)-1) {
        result.found = true;
        result.start = pos;
        result.length = ctx->pattern_len;
        result.match_count = search_count_matches(ctx);
    }

    return result;
}

size_t search_count_matches(SearchContext *ctx) {
    if (!ctx) return 0;

    size_t count = 0;
    size_t pos = 0;

    while (pos <= ctx->text_len - ctx->pattern_len) {
        size_t match = find_match(ctx->text, ctx->text_len,
                                  ctx->pattern, ctx->pattern_len,
                                  &ctx->options, pos);

        if (match == (size_t)-1) break;

        count++;
        pos = match + (ctx->pattern_len > 0 ? ctx->pattern_len : 1);
    }

    return count;
}

/* --------------------------------------------------------------------------
 * API Remplacement
 * -------------------------------------------------------------------------- */

ReplaceResult search_replace_first(const char *text, const char *pattern,
                                    const char *replacement,
                                    const SearchOptions *options) {
    ReplaceResult result = {NULL, 0, 0};

    if (!text || !pattern || !replacement) {
        return result;
    }

    SearchContext *ctx = search_create(text, pattern, options);
    if (!ctx) return result;

    SearchResult search_res = search_find_first(ctx);

    if (!search_res.found) {
        search_destroy(ctx);
        return result;
    }

    // Construire le résultat
    size_t text_len = strlen(text);
    size_t replacement_len = strlen(replacement);
    size_t new_len = text_len - search_res.length + replacement_len;

    result.result = (char *)malloc(new_len + 1);
    if (!result.result) {
        search_destroy(ctx);
        return result;
    }

    // Copier la partie avant le match
    memcpy(result.result, text, search_res.start);

    // Copier le remplacement
    memcpy(result.result + search_res.start, replacement, replacement_len);

    // Copier la partie après le match
    size_t remaining = text_len - (search_res.start + search_res.length);
    if (remaining > 0) {
        memcpy(result.result + search_res.start + replacement_len,
               text + search_res.start + search_res.length, remaining);
    }

    result.result[new_len] = '\0';
    result.result_length = new_len;
    result.replacements = 1;

    search_destroy(ctx);
    return result;
}

ReplaceResult search_replace_all(const char *text, const char *pattern,
                                  const char *replacement,
                                  const SearchOptions *options) {
    ReplaceResult result = {NULL, 0, 0};

    if (!text || !pattern || !replacement) {
        return result;
    }

    SearchContext *ctx = search_create(text, pattern, options);
    if (!ctx) return result;

    // Compter les matches
    size_t match_count = search_count_matches(ctx);
    if (match_count == 0) {
        search_destroy(ctx);
        return result;
    }

    // Calculer la taille du résultat
    size_t text_len = strlen(text);
    size_t pattern_len = strlen(pattern);
    size_t replacement_len = strlen(replacement);
    size_t new_len = text_len + (match_count * (replacement_len - pattern_len));

    // Construire le résultat en remplaçant tous les matches
    result.result = (char *)malloc(new_len + 1);
    if (!result.result) {
        search_destroy(ctx);
        return result;
    }

    size_t out_pos = 0;
    size_t search_pos = 0;

    while (search_pos < text_len) {
        size_t match = find_match(text, text_len,
                                  pattern, pattern_len,
                                  options, search_pos);

        if (match == (size_t)-1) {
            // Copier le reste du texte
            size_t remaining = text_len - search_pos;
            memcpy(result.result + out_pos, text + search_pos, remaining);
            out_pos += remaining;
            break;
        }

        // Copier la partie avant le match
        size_t chunk_size = match - search_pos;
        memcpy(result.result + out_pos, text + search_pos, chunk_size);
        out_pos += chunk_size;

        // Copier le remplacement
        memcpy(result.result + out_pos, replacement, replacement_len);
        out_pos += replacement_len;

        result.replacements++;
        search_pos = match + pattern_len;
    }

    result.result[out_pos] = '\0';
    result.result_length = out_pos;

    search_destroy(ctx);
    return result;
}

void replace_result_free(ReplaceResult *result) {
    if (result) {
        free(result->result);
        result->result = NULL;
        result->result_length = 0;
        result->replacements = 0;
    }
}
