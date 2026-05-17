#include "../../include/undo_redo.h"
#include <stdlib.h>
#include <string.h>

UndoRedoManager* ur_init() {
    return (UndoRedoManager*)calloc(1, sizeof(UndoRedoManager));
}

static void free_stack(EditorCommand **stack) {
    while (*stack) {
        EditorCommand *tmp = *stack;
        *stack = tmp->next;
        free(tmp->data);
        free(tmp);
    }
}

void ur_record_action(UndoRedoManager *mgr, CommandType type, size_t pos, const char *content) {
    EditorCommand *cmd = malloc(sizeof(EditorCommand));
    cmd->type = type;
    cmd->position = pos;
    cmd->data = strdup(content);
    cmd->next = mgr->undo_stack;
    mgr->undo_stack = cmd;
    free_stack(&mgr->redo_stack); // Invalidation du redo lors d'une nouvelle action
}

void ur_undo(UndoRedoManager *mgr, GapBuffer *gb) {
    if (!mgr->undo_stack) return;
    EditorCommand *cmd = mgr->undo_stack;
    mgr->undo_stack = cmd->next;

    gb_move_cursor(gb, cmd->position);
    if (cmd->type == CMD_INSERT) {
        // Annuler insertion = Supprimer
        size_t len = strlen(cmd->data);
        gb_move_cursor(gb, cmd->position + len);
        for(size_t i = 0; i < len; i++) gb_delete_backspace(gb);
    } else {
        // Annuler suppression = Réinsérer
        for(size_t i = 0; cmd->data[i]; i++) gb_insert_char(gb, cmd->data[i]);
    }
    
    cmd->next = mgr->redo_stack;
    mgr->redo_stack = cmd;
}

void ur_redo(UndoRedoManager *mgr, GapBuffer *gb) {
    if (!mgr->redo_stack) return;
    EditorCommand *cmd = mgr->redo_stack;
    mgr->redo_stack = cmd->next;

    gb_move_cursor(gb, cmd->position);
    if (cmd->type == CMD_INSERT) {
        for(size_t i = 0; cmd->data[i]; i++) gb_insert_char(gb, cmd->data[i]);
    } else {
        size_t len = strlen(cmd->data);
        gb_move_cursor(gb, cmd->position + len);
        for(size_t i = 0; i < len; i++) gb_delete_backspace(gb);
    }

    cmd->next = mgr->undo_stack;
    mgr->undo_stack = cmd;
}

void ur_cleanup(UndoRedoManager *mgr) {
    if (mgr) {
        free_stack(&mgr->undo_stack);
        free_stack(&mgr->redo_stack);
        free(mgr);
    }
}