
/**
 * @file gap_buffer.h
 * @brief Structure Gap Buffer pour l'édition de texte performante.
 *
 * Le Gap Buffer maintient un "trou" (gap) à la position du curseur,
 * permettant insertions et suppressions en O(1) amorti à cet endroit.
 *
 * @author DEV-A
 */
#ifndef GAP_BUFFER_H
#define GAP_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

/** Structure opaque du Gap Buffer. */
typedef struct GapBuffer GapBuffer;

/**
 * @brief Crée un nouveau Gap Buffer.
 * @param initial_size Capacité initiale en octets.
 * @return Pointeur sur le buffer ou NULL si échec.
 */
GapBuffer *gap_buffer_create(size_t initial_size);

/** @brief Libère le Gap Buffer et toutes ses ressources. */
void gap_buffer_destroy(GapBuffer *gb);

/**
 * @brief Insère un caractère à la position du curseur (gap).
 * @return true si succès, false sinon.
 */
bool gap_buffer_insert_char(GapBuffer *gb, char c);

/**
 * @brief Insère une chaîne UTF-8 à la position du curseur.
 * @param str Chaîne nul-terminée à insérer.
 * @return true si succès.
 */
bool gap_buffer_insert_string(GapBuffer *gb, const char *str);

/** @brief Supprime le caractère avant le curseur (Backspace). */
bool gap_buffer_delete_backward(GapBuffer *gb);

/** @brief Supprime le caractère après le curseur (Delete). */
bool gap_buffer_delete_forward(GapBuffer *gb);

/**
 * @brief Déplace le curseur (gap) à une position absolue.
 * @param pos Position en octets depuis le début.
 */
bool gap_buffer_move_cursor(GapBuffer *gb, size_t pos);

/** @brief Retourne la position courante du curseur. */
size_t gap_buffer_get_cursor(const GapBuffer *gb);

/** @brief Retourne la longueur du contenu (hors gap). */
size_t gap_buffer_length(const GapBuffer *gb);

/**
 * @brief Extrait tout le contenu sous forme de chaîne nul-terminée.
 * @return Chaîne allouée à libérer par l'appelant, ou NULL.
 */
char *gap_buffer_to_string(const GapBuffer *gb);

/** @brief Retourne le caractère à la position donnée. */
char gap_buffer_char_at(const GapBuffer *gb, size_t pos);

#endif /* GAP_BUFFER_H */
