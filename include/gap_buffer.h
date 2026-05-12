#ifndef GAP_BUFFER_H
#define GAP_BUFFER_H

#include <stddef.h>

/**
 * @brief Structure du Gap Buffer (Fondation de l'éditeur).
 * Utilise quatre pointeurs/indices pour gérer l'espace de texte et le "trou" (gap) [2].
 */
typedef struct {
    char *buffer;      // Espace mémoire total
    size_t size;       // Taille totale du buffer (capacité)
    size_t gap_start;  // Index du début du gap (position du curseur)
    size_t gap_end;    // Index de la fin du gap
} GapBuffer;

/* API de gestion du buffer */
GapBuffer* gb_create(size_t initial_capacity);
void gb_insert_char(GapBuffer *gb, char c);
void gb_delete_backspace(GapBuffer *gb);
void gb_move_cursor(GapBuffer *gb, size_t pos);
size_t gb_get_content_length(GapBuffer *gb);
void gb_destroy(GapBuffer *gb);

/* Fonction utilitaire pour l'interface UI */
char* gb_get_text(GapBuffer *gb);

#endif
