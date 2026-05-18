/**
 * @file memory.c
 * @brief Allocateur debug avec tracking des fuites mémoire.
 *
 * Wrappe malloc/calloc/realloc/free avec traces optionnelles et comptage
 * des allocations actives. Compatible avec Valgrind.
 * @author DEV-A
 */

#include "../../include/memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Structure de tracking pour une allocation
 * -------------------------------------------------------------------------- */
typedef struct MemBlock {
    void         *ptr;           // Pointeur alloué
    size_t        size;           // Taille demandée
    const char   *file;           // Fichier source
    int           line;           // Numéro de ligne
    struct MemBlock *next;        // Chaînage simple
} MemBlock;

/* --------------------------------------------------------------------------
 * Globales pour tracking
 * -------------------------------------------------------------------------- */
static MemBlock *mem_list = NULL;      // Tête de la liste d'allocations
static size_t   total_bytes = 0;       // Total d'octets alloués
static size_t   total_count = 0;       // Nombre total d'allocations
static bool     debug_mode = false;    // Mode debug activé?

/* --------------------------------------------------------------------------
 * Helpers internes
 * -------------------------------------------------------------------------- */

/**
 * Ajoute une allocation au tracking.
 */
static void register_alloc(void *ptr, size_t size, const char *file, int line) {
    if (!ptr) return;

    MemBlock *block = (MemBlock *)malloc(sizeof(MemBlock));
    if (!block) {
        fprintf(stderr, "[ERROR] register_alloc: impossible d'allouer MemBlock\n");
        return;
    }

    block->ptr = ptr;
    block->size = size;
    block->file = file;
    block->line = line;
    block->next = mem_list;
    mem_list = block;

    total_bytes += size;
    total_count++;

    if (debug_mode) {
        fprintf(stderr, "[ALLOC] %p (%zu bytes) @ %s:%d\n", ptr, size, file, line);
    }
}

/**
 * Retire une allocation du tracking et retourne sa taille.
 * Retourne 0 si le pointeur n'est pas trouvé.
 */
static size_t unregister_alloc(void *ptr) {
    if (!ptr) return 0;

    MemBlock **pp = &mem_list;
    while (*pp) {
        if ((*pp)->ptr == ptr) {
            MemBlock *block = *pp;
            size_t size = block->size;
            const char *file = block->file;
            int line = block->line;

            *pp = block->next;
            total_bytes -= size;
            total_count--;

            if (debug_mode) {
                fprintf(stderr, "[FREE] %p (%zu bytes) @ %s:%d\n", ptr, size, file, line);
            }

            free(block);
            return size;
        }
        pp = &(*pp)->next;
    }

    if (debug_mode) {
        fprintf(stderr, "[WARN] unregister_alloc: pointeur %p non trouvé\n", ptr);
    }
    return 0;
}

/**
 * Cherche une allocation et retourne sa taille.
 */
static size_t find_alloc_size(void *ptr) {
    if (!ptr) return 0;

    for (MemBlock *block = mem_list; block; block = block->next) {
        if (block->ptr == ptr) {
            return block->size;
        }
    }
    return 0;
}

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

void memory_set_debug(bool enabled) {
    debug_mode = enabled;
}

void *mem_alloc(size_t size, const char *file, int line) {
    if (size == 0) {
        if (debug_mode) {
            fprintf(stderr, "[WARN] mem_alloc: taille 0 @ %s:%d\n", file, line);
        }
        return NULL;
    }

    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "[ERROR] mem_alloc: malloc(%zu) failed @ %s:%d\n", size, file, line);
        return NULL;
    }

    register_alloc(ptr, size, file, line);
    return ptr;
}

void *mem_calloc(size_t n, size_t size, const char *file, int line) {
    if (n == 0 || size == 0) {
        if (debug_mode) {
            fprintf(stderr, "[WARN] mem_calloc: n=%zu size=%zu @ %s:%d\n", n, size, file, line);
        }
        return NULL;
    }

    // Vérifier débordement
    if (n > (size_t)-1 / size) {
        fprintf(stderr, "[ERROR] mem_calloc: débordement (%zu * %zu) @ %s:%d\n", n, size, file, line);
        return NULL;
    }

    void *ptr = calloc(n, size);
    if (!ptr) {
        fprintf(stderr, "[ERROR] mem_calloc: calloc(%zu, %zu) failed @ %s:%d\n", n, size, file, line);
        return NULL;
    }

    register_alloc(ptr, n * size, file, line);
    return ptr;
}

void *mem_realloc(void *ptr, size_t size, const char *file, int line) {
    if (size == 0) {
        // realloc(ptr, 0) équivaut à free(ptr) et retourne NULL
        if (ptr) {
            unregister_alloc(ptr);
            free(ptr);
        }
        return NULL;
    }

    if (!ptr) {
        // realloc(NULL, size) équivaut à malloc(size)
        return mem_alloc(size, file, line);
    }

    // Chercher la taille ancienne
    size_t old_size = find_alloc_size(ptr);
    if (old_size == 0) {
        fprintf(stderr, "[WARN] mem_realloc: pointeur %p non trouvé @ %s:%d\n", ptr, file, line);
        // Continuer quand même, realloc le fera lui-même
    }

    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        fprintf(stderr, "[ERROR] mem_realloc: realloc(%p, %zu) failed @ %s:%d\n", ptr, size, file, line);
        return NULL;
    }

    // Mettre à jour le tracking
    if (old_size > 0) {
        unregister_alloc(ptr);
    }
    register_alloc(new_ptr, size, file, line);

    return new_ptr;
}

void mem_free(void *ptr, const char *file, int line) {
    if (!ptr) {
        if (debug_mode) {
            fprintf(stderr, "[WARN] mem_free: NULL pointer @ %s:%d\n", file, line);
        }
        return;
    }

    unregister_alloc(ptr);
    free(ptr);
}

size_t memory_active_count(void) {
    return total_count;
}

size_t memory_active_bytes(void) {
    return total_bytes;
}

void memory_report_leaks(void) {
    fprintf(stderr, "\n=== Memory Leak Report ===\n");
    fprintf(stderr, "Total allocations actives : %zu\n", total_count);
    fprintf(stderr, "Total octets alloués     : %zu bytes\n", total_bytes);

    if (total_count == 0) {
        fprintf(stderr, "[OK] Aucune fuite détectée.\n\n");
        return;
    }

    fprintf(stderr, "\nBloques non libérés:\n");
    for (MemBlock *block = mem_list; block; block = block->next) {
        fprintf(stderr, "  %p (%zu bytes) @ %s:%d\n", block->ptr, block->size, block->file, block->line);
    }
    fprintf(stderr, "\n");
}
