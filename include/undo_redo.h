#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include <stddef.h>
#include "gap_buffer.h"

typedef enum { CMD_INSERT, CMD_DELETE } CommandType;

typedef struct EditorCommand {
    CommandType type;
    size_t position;
    char *data;
    struct EditorCommand *next;
} EditorCommand;

typedef struct {
    EditorCommand *undo_stack;
    EditorCommand *redo_stack;
} UndoRedoManager;

UndoRedoManager* ur_init();
void ur_record_action(UndoRedoManager *mgr, CommandType type, size_t pos, const char *content);
void ur_undo(UndoRedoManager *mgr, GapBuffer *gb);
void ur_redo(UndoRedoManager *mgr, GapBuffer *gb);
void ur_cleanup(UndoRedoManager *mgr);

#endif