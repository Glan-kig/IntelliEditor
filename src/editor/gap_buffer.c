#include "../../include/gap_buffer.h"
#include <stdlib.h>
#include <string.h>

/**
 * Initialise le buffer avec une capacité donnée.
 */
GapBuffer* gb_create(size_t capacity) {
    if (capacity == 0) capacity = 1024; // Taille par défaut
    GapBuffer *gb = malloc(sizeof(GapBuffer));
    gb->buffer = malloc(capacity);
    gb->size = capacity;
    gb->gap_start = 0;
    gb->gap_end = capacity;
    return gb;
}

/**
 * Calcule la longueur réelle du texte (sans le gap).
 */
size_t gb_get_content_length(GapBuffer *gb) {
    return gb->gap_start + (gb->size - gb->gap_end);
}

/**
 * Déplace le gap à une position donnée. 
 * Essentiel pour l'insertion à n'importe quel endroit du texte [2].
 */
void gb_move_cursor(GapBuffer *gb, size_t pos) {
    size_t len = gb_get_content_length(gb);
    if (pos > len) pos = len;

    // Déplacement vers la gauche : on déplace les caractères après le gap vers la fin
    while (gb->gap_start > pos) {
        gb->gap_start--;
        gb->gap_end--;
        gb->buffer[gb->gap_end] = gb->buffer[gb->gap_start];
    }
    // Déplacement vers la droite : on déplace les caractères avant le gap
    while (gb->gap_start < pos) {
        gb->buffer[gb->gap_start] = gb->buffer[gb->gap_end];
        gb->gap_start++;
        gb->gap_end++;
    }
}

/**
 * Insère un caractère à la position actuelle du gap.
 * Gère l'agrandissement automatique (realloc) si le gap est plein.
 */
void gb_insert_char(GapBuffer *gb, char c) {
    if (gb->gap_start == gb->gap_end) {
        // Le gap est vide, il faut agrandir le buffer
        size_t old_size = gb->size;
        size_t new_size = old_size * 2;
        char *new_buf = realloc(gb->buffer, new_size);
        
        // Déplacer la "queue" du texte à la fin du nouveau buffer agrandi
        size_t tail_len = old_size - gb->gap_end;
        size_t new_gap_end = new_size - tail_len;
        memmove(new_buf + new_gap_end, new_buf + gb->gap_end, tail_len);
        
        gb->buffer = new_buf;
        gb->gap_end = new_gap_end;
        gb->size = new_size;
    }
    gb->buffer[gb->gap_start++] = c;
}

/**
 * Supprime le caractère juste avant le curseur (Backspace).
 */
void gb_delete_backspace(GapBuffer *gb) {
    if (gb->gap_start > 0) {
        gb->gap_start--;
    }
}

/**
 * Extrait le texte du buffer dans une chaîne C classique (null-terminated).
 * Indispensable pour envoyer les données vers l'UI GTK ou Scintilla [4].
 */
char* gb_get_text(GapBuffer *gb) {
    size_t len = gb_get_content_length(gb);
    char *text = malloc(len + 1);
    // Copie de la partie avant le gap
    memcpy(text, gb->buffer, gb->gap_start);
    // Copie de la partie après le gap
    memcpy(text + gb->gap_start, gb->buffer + gb->gap_end, gb->size - gb->gap_end);
    text[len] = '\0';
    return text;
}

/**
 * Libère la mémoire.
 */
void gb_destroy(GapBuffer *gb) {
    if (gb) {
        free(gb->buffer);
        free(gb);
    }
}