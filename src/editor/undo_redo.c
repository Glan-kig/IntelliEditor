/**
 * @file undo_redo.c
 * @brief Implémentation du système Undo/Redo (pattern Command).
 * @author DEV-A
 */
#include "undo_redo.h"
#include "memory.h"
#include <string.h>
#include <stdlib.h>

/** Une commande encapsule une opération réversible. */
struct Command {
    CommandType type;
    size_t      pos;
    char       *text;     /**< Texte concerné (copie) */
    size_t      text_len;
    Command    *next;     /**< Chaînage pour pile */
};

/** Pile Undo/Redo : deux listes chaînées LIFO. */
struct UndoRedoStack {
    Command *undo_top;
    Command *redo_top;
    size_t   undo_count;
    size_t   redo_count;
    size_t   max_history; /**< 0 = illimité */
};

/* --------------------------------------------------------------------------
 * Helpers internes
 * -------------------------------------------------------------------------- */

static Command *command_create(CommandType type, size_t pos, const char *text) {
    Command *cmd = MEM_ALLOC(sizeof(Command));
    if (!cmd) return NULL;
    cmd->type = type;
    cmd->pos = pos;
    cmd->text_len = text ? strlen(text) : 0;
    cmd->text = MEM_ALLOC(cmd->text_len + 1);
    if (!cmd->text) { MEM_FREE(cmd); return NULL; }
    if (text) memcpy(cmd->text, text, cmd->text_len);
    cmd->text[cmd->text_len] = '\0';
    cmd->next = NULL;
    return cmd;
}

static void command_destroy(Command *cmd) {
    if (!cmd) return;
    MEM_FREE(cmd->text);
    MEM_FREE(cmd);
}

/** Vide une pile de commandes. */
static void stack_clear(Command **top, size_t *count) {
    while (*top) {
        Command *next = (*top)->next;
        command_destroy(*top);
        *top = next;
    }
    *count = 0;
}

/** Tronque la pile undo si elle dépasse max_history. */
static void trim_history(UndoRedoStack *s) {
    if (s->max_history == 0) return;
    while (s->undo_count > s->max_history) {
        /* Supprimer le plus ancien (fond de pile) */
        Command **cur = &s->undo_top;
        while ((*cur)->next) cur = &(*cur)->next;
        command_destroy(*cur);
        *cur = NULL;
        s->undo_count--;
    }
}

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

UndoRedoStack *undo_redo_create(size_t max_history) {
    UndoRedoStack *s = MEM_CALLOC(1, sizeof(UndoRedoStack));
    if (!s) return NULL;
    s->max_history = max_history;
    return s;
}

void undo_redo_destroy(UndoRedoStack *s) {
    if (!s) return;
    stack_clear(&s->undo_top, &s->undo_count);
    stack_clear(&s->redo_top, &s->redo_count);
    MEM_FREE(s);
}

bool undo_redo_push(UndoRedoStack *s, CommandType type,
                    size_t pos, const char *text) {
    if (!s) return false;
    Command *cmd = command_create(type, pos, text);
    if (!cmd) return false;

    cmd->next = s->undo_top;
    s->undo_top = cmd;
    s->undo_count++;

    /* Toute nouvelle action invalide la pile redo */
    stack_clear(&s->redo_top, &s->redo_count);

    trim_history(s);
    return true;
}

bool undo_redo_undo(UndoRedoStack *s, GapBuffer *gb) {
    if (!s || !gb || !s->undo_top) return false;

    Command *cmd = s->undo_top;
    s->undo_top = cmd->next;
    s->undo_count--;

    /* Appliquer l'opération INVERSE sur le buffer */
    gap_buffer_move_cursor(gb, cmd->pos);
    if (cmd->type == CMD_INSERT) {
        /* Inverse de INSERT = supprimer text_len caractères en avant */
        for (size_t i = 0; i < cmd->text_len; i++)
            gap_buffer_delete_forward(gb);
    } else { /* CMD_DELETE → réinsérer */
        gap_buffer_insert_string(gb, cmd->text);
        gap_buffer_move_cursor(gb, cmd->pos);
    }

    /* Empiler sur redo */
    cmd->next = s->redo_top;
    s->redo_top = cmd;
    s->redo_count++;
    return true;
}

bool undo_redo_redo(UndoRedoStack *s, GapBuffer *gb) {
    if (!s || !gb || !s->redo_top) return false;

    Command *cmd = s->redo_top;
    s->redo_top = cmd->next;
    s->redo_count--;

    /* Réappliquer l'opération ORIGINALE */
    gap_buffer_move_cursor(gb, cmd->pos);
    if (cmd->type == CMD_INSERT) {
        gap_buffer_insert_string(gb, cmd->text);
    } else {
        for (size_t i = 0; i < cmd->text_len; i++)
            gap_buffer_delete_forward(gb);
    }

    /* Remettre sur undo */
    cmd->next = s->undo_top;
    s->undo_top = cmd;
    s->undo_count++;
    return true;
}

bool undo_redo_can_undo(const UndoRedoStack *s) {
    return s && s->undo_top != NULL;
}

bool undo_redo_can_redo(const UndoRedoStack *s) {
    return s && s->redo_top != NULL;
}
