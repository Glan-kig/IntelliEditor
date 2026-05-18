/**
 * @file memory.h
 * @brief Allocateur debug compatible Valgrind.
 *
 * Wrappe malloc/free avec traces et comptage pour détecter les fuites.
 * @author DEV-A
 */
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdbool.h>

/** @brief Active le mode debug (traces sur stderr). */
void memory_set_debug(bool enabled);

/** @brief Wrapper malloc avec tracking. */
void *mem_alloc(size_t size, const char *file, int line);

/** @brief Wrapper calloc avec tracking. */
void *mem_calloc(size_t n, size_t size, const char *file, int line);

/** @brief Wrapper realloc avec tracking. */
void *mem_realloc(void *ptr, size_t size, const char *file, int line);

/** @brief Wrapper free avec tracking. */
void mem_free(void *ptr, const char *file, int line);

/** @brief Retourne nombre total d'allocations actives. */
size_t memory_active_count(void);

/** @brief Retourne total d'octets alloués actuellement. */
size_t memory_active_bytes(void);

/** @brief Affiche un rapport des fuites sur stderr. */
void memory_report_leaks(void);

/* Macros pratiques (à utiliser dans tout le code) */
#define MEM_ALLOC(s)        mem_alloc((s), __FILE__, __LINE__)
#define MEM_CALLOC(n, s)    mem_calloc((n), (s), __FILE__, __LINE__)
#define MEM_REALLOC(p, s)   mem_realloc((p), (s), __FILE__, __LINE__)
#define MEM_FREE(p)         mem_free((p), __FILE__, __LINE__)

#endif /* MEMORY_H */
