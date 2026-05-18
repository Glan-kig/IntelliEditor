#include "../../include/gap_buffer.h"
#include <stdlib.h>
#include <string.h>

/**
 * Crée un nouveau Gap Buffer avec une capacité initiale.
 */
GapBuffer* gap_buffer_create(size_t initial_size) {
    if (initial_size == 0) initial_size = 1024;
    
    GapBuffer *gb = (GapBuffer *)malloc(sizeof(GapBuffer));
    if (!gb) return NULL;
    
    gb->buffer = (char *)malloc(initial_size);
    if (!gb->buffer) {
        free(gb);
        return NULL;
    }
    
    gb->size = initial_size;
    gb->gap_start = 0;
    gb->gap_end = initial_size;
    gb->cursor = 0;
    return gb;
}

/**
 * Libère toutes les ressources du Gap Buffer.
 */
void gap_buffer_destroy(GapBuffer *gb) {
    if (gb) {
        free(gb->buffer);
        free(gb);
    }
}

/**
 * Retourne la longueur totale du contenu (sans le gap).
 */
size_t gap_buffer_length(const GapBuffer *gb) {
    if (!gb) return 0;
    return gb->gap_start + (gb->size - gb->gap_end);
}

/**
 * Retourne la position actuelle du curseur.
 */
size_t gap_buffer_get_cursor(const GapBuffer *gb) {
    return gb ? gb->gap_start : 0;
}

/**
 * Déplace le gap à une position donnée.
 * Essentiel pour l'insertion à n'importe quel endroit du texte.
 */
bool gap_buffer_move_cursor(GapBuffer *gb, size_t pos) {
    if (!gb) return false;
    
    size_t len = gap_buffer_length(gb);
    if (pos > len) pos = len;

    // Déplacement vers la gauche : on déplace les caractères après le gap vers la fin
    while (gb->gap_start > pos) {
        gb->gap_start--;
        gb->gap_end--;
        gb->buffer[gb->gap_end] = gb->buffer[gb->gap_start];
    }
    
    // Déplacement vers la droite : on déplace les caractères avant le gap vers le début
    while (gb->gap_start < pos) {
        gb->buffer[gb->gap_start] = gb->buffer[gb->gap_end];
        gb->gap_start++;
        gb->gap_end++;
    }
    
    gb->cursor = pos;
    return true;
}

/**
 * Insère un caractère à la position actuelle du gap.
 * Gère l'agrandissement automatique (realloc) si le gap est plein.
 */
bool gap_buffer_insert_char(GapBuffer *gb, char c) {
    if (!gb) return false;
    
    if (gb->gap_start == gb->gap_end) {
        // Le gap est vide, il faut agrandir le buffer
        size_t old_size = gb->size;
        size_t new_size = old_size * 2;
        char *new_buf = (char *)realloc(gb->buffer, new_size);
        if (!new_buf) return false;
        
        // Déplacer la "queue" du texte à la fin du nouveau buffer agrandi
        size_t tail_len = old_size - gb->gap_end;
        size_t new_gap_end = new_size - tail_len;
        if (tail_len > 0) {
            memmove(new_buf + new_gap_end, new_buf + gb->gap_end, tail_len);
        }
        
        gb->buffer = new_buf;
        gb->gap_end = new_gap_end;
        gb->size = new_size;
    }
    
    gb->buffer[gb->gap_start++] = c;
    gb->cursor++;
    return true;
}

/**
 * Insère une chaîne UTF-8 à la position actuelle du gap.
 */
bool gap_buffer_insert_string(GapBuffer *gb, const char *str) {
    if (!gb || !str) return false;
    
    while (*str) {
        if (!gap_buffer_insert_char(gb, *str++)) {
            return false;
        }
    }
    return true;
}

/**
 * Supprime le caractère avant le curseur (Backspace).
 */
bool gap_buffer_delete_backward(GapBuffer *gb) {
    if (!gb) return false;
    
    if (gb->gap_start > 0) {
        gb->gap_start--;
        gb->cursor--;
        return true;
    }
    return false;
}

/**
 * Supprime le caractère après le curseur (Delete).
 */
bool gap_buffer_delete_forward(GapBuffer *gb) {
    if (!gb) return false;
    
    if (gb->gap_end < gb->size) {
        gb->gap_end++;
        return true;
    }
    return false;
}

/**
 * Retourne le caractère à une position donnée.
 */
char gap_buffer_char_at(const GapBuffer *gb, size_t pos) {
    if (!gb) return '\0';
    
    size_t len = gap_buffer_length(gb);
    if (pos >= len) return '\0';
    
    if (pos < gb->gap_start) {
        return gb->buffer[pos];
    } else {
        return gb->buffer[pos + (gb->gap_end - gb->gap_start)];
    }
}

/**
 * Extrait tout le contenu du buffer dans une chaîne C nul-terminée.
 * L'appelant doit libérer la chaîne retournée avec free().
 */
char* gap_buffer_to_string(const GapBuffer *gb) {
    if (!gb) return NULL;
    
    size_t len = gap_buffer_length(gb);
    char *text = (char *)malloc(len + 1);
    if (!text) return NULL;
    
    // Copie de la partie avant le gap
    memcpy(text, gb->buffer, gb->gap_start);
    
    // Copie de la partie après le gap
    if (gb->gap_end < gb->size) {
        size_t tail_len = gb->size - gb->gap_end;
        memcpy(text + gb->gap_start, gb->buffer + gb->gap_end, tail_len);
    }
    
    text[len] = '\0';
    return text;
}