/**
 * @file search_replace.h
 * @brief Moteur de recherche et remplacement pour l'éditeur.
 *
 * Fournit des fonctionnalités de recherche/remplacement avec support des options
 * (sensibilité casse, mot entier, regex simple).
 *
 * @author DEV-A
 */
#ifndef SEARCH_REPLACE_H
#define SEARCH_REPLACE_H

#include <stdbool.h>
#include <stddef.h>

/** Options de recherche */
typedef struct {
    bool case_sensitive;      // Respecter la casse
    bool whole_word;          // Mot entier uniquement
    bool use_regex;           // Utiliser expressions régulières (non implémenté pour v1)
} SearchOptions;

/** Résultat d'une recherche */
typedef struct {
    bool found;               // Trouvé ou non
    size_t start;             // Position de début (en octets)
    size_t length;            // Longueur du match (en octets)
    size_t match_count;       // Nombre total de matches dans le texte
} SearchResult;

/** Structure opaque pour le contexte de recherche */
typedef struct SearchContext SearchContext;

/* --------------------------------------------------------------------------
 * API Recherche
 * -------------------------------------------------------------------------- */

/**
 * @brief Crée un contexte de recherche.
 * @param text Texte complet à analyser.
 * @param pattern Motif à chercher.
 * @param options Options de recherche.
 * @return Contexte alloué ou NULL si erreur.
 */
SearchContext *search_create(const char *text, const char *pattern,
                              const SearchOptions *options);

/**
 * @brief Libère un contexte de recherche.
 */
void search_destroy(SearchContext *ctx);

/**
 * @brief Trouve la première occurrence du motif.
 * @param ctx Contexte de recherche.
 * @return Résultat de la recherche.
 */
SearchResult search_find_first(SearchContext *ctx);

/**
 * @brief Trouve la prochaine occurrence du motif.
 * @param ctx Contexte de recherche.
 * @param start_pos Position de départ pour la recherche.
 * @return Résultat de la recherche.
 */
SearchResult search_find_next(SearchContext *ctx, size_t start_pos);

/**
 * @brief Compte toutes les occurrences du motif.
 * @param ctx Contexte de recherche.
 * @return Nombre d'occurrences.
 */
size_t search_count_matches(SearchContext *ctx);

/* --------------------------------------------------------------------------
 * API Remplacement
 * -------------------------------------------------------------------------- */

/**
 * @brief Structure pour le remplacement.
 */
typedef struct {
    char *result;             // Texte après remplacement
    size_t result_length;     // Longueur du résultat
    size_t replacements;      // Nombre de remplacements effectués
} ReplaceResult;

/**
 * @brief Remplace la première occurrence du motif.
 * @param text Texte source.
 * @param pattern Motif à chercher.
 * @param replacement Texte de remplacement.
 * @param options Options de recherche.
 * @return Résultat du remplacement (l'appelant doit libérer result avec free).
 */
ReplaceResult search_replace_first(const char *text, const char *pattern,
                                    const char *replacement,
                                    const SearchOptions *options);

/**
 * @brief Remplace toutes les occurrences du motif.
 * @param text Texte source.
 * @param pattern Motif à chercher.
 * @param replacement Texte de remplacement.
 * @param options Options de recherche.
 * @return Résultat du remplacement (l'appelant doit libérer result avec free).
 */
ReplaceResult search_replace_all(const char *text, const char *pattern,
                                  const char *replacement,
                                  const SearchOptions *options);

/**
 * @brief Libère les ressources d'un ReplaceResult.
 */
void replace_result_free(ReplaceResult *result);

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

/**
 * @brief Convertit une chaîne en minuscules (pour recherche case-insensitive).
 * @return Chaîne allouée à libérer avec free() ou NULL si erreur.
 */
char *search_to_lowercase(const char *str);

/**
 * @brief Vérifie si c'est le début d'un mot.
 */
bool search_is_word_boundary(const char *text, size_t pos);

/**
 * @brief Vérifie si c'est la fin d'un mot.
 */
bool search_is_word_end(const char *text, size_t pos);

#endif /* SEARCH_REPLACE_H */
