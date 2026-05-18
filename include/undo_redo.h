/**
 * @file undo_redo.h
 * @brief Système Undo/Redo via le pattern Command.
 *
 * Chaque opération d'édition est encapsulée dans une commande possédant
 * une méthode execute() et undo(). Les commandes sont empilées sur deux
 * piles : undo_stack et redo_stack.
 *
 * @author DEV-A
 */
#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include <stddef.h>
#include <stdbool.h>
#include "gap_buffer.h"

/** Type de commande supporté. */
typedef enum {
    CMD_INSERT,   /**< Insertion de texte */
    CMD_DELETE    /**< Suppression de texte */
} CommandType;

/** Structure opaque d'une commande. */
typedef struct Command Command;

/** Structure opaque de la pile Undo/Redo. */
typedef struct UndoRedoStack UndoRedoStack;

/**
 * @brief Crée une pile Undo/Redo.
 * @param max_history Profondeur max (0 = illimité).
 */
UndoRedoStack *undo_redo_create(size_t max_history);

/** @brief Libère la pile et toutes les commandes. */
void undo_redo_destroy(UndoRedoStack *stack);

/**
 * @brief Enregistre une nouvelle commande (vide la pile redo).
 * @param type Type de commande.
 * @param pos Position dans le buffer.
 * @param text Texte concerné (copié en interne).
 */
bool undo_redo_push(UndoRedoStack *stack, CommandType type,
                    size_t pos, const char *text);

/** @brief Annule la dernière commande (applique l'inverse sur gb). */
bool undo_redo_undo(UndoRedoStack *stack, GapBuffer *gb);

/** @brief Réapplique la dernière commande annulée. */
bool undo_redo_redo(UndoRedoStack *stack, GapBuffer *gb);

/** @brief Vrai si undo est possible. */
bool undo_redo_can_undo(const UndoRedoStack *stack);

/** @brief Vrai si redo est possible. */
bool undo_redo_can_redo(const UndoRedoStack *stack);

/* Alias pour compatibilité avec ancienne API */
typedef UndoRedoStack UndoStack;
#define undo_stack_create(max) undo_redo_create(max)
#define undo_stack_destroy(s) undo_redo_destroy(s)
#define undo_record_insert(s, pos, text, len) undo_redo_push((s), CMD_INSERT, (pos), (text))
#define undo_record_delete(s, pos, text, len) undo_redo_push((s), CMD_DELETE, (pos), (text))
#define undo_perform(s, gb) undo_redo_undo((s), (gb))
#define redo_perform(s, gb) undo_redo_redo((s), (gb))

#endif /* UNDO_REDO_H */
