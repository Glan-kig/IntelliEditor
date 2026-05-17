/**
 * @file formatter.h
 * @brief Moteur de formatage de styles inline (gras, italique, etc.).
 * @author DEV-A
 */
#ifndef FORMATTER_H
#define FORMATTER_H

#include <stddef.h>
#include <stdbool.h>

/** Styles supportés (combinables en bitmask). */
typedef enum {
    STYLE_NONE       = 0,
    STYLE_BOLD       = 1 << 0,
    STYLE_ITALIC     = 1 << 1,
    STYLE_UNDERLINE  = 1 << 2,
    STYLE_STRIKE     = 1 << 3
} TextStyle;

/* Alias pour la compatibilité */
typedef unsigned int FormatStyle;
#define FORMAT_BOLD       STYLE_BOLD
#define FORMAT_ITALIC     STYLE_ITALIC
#define FORMAT_UNDERLINE  STYLE_UNDERLINE
#define FORMAT_STRIKE     STYLE_STRIKE
#define STYLE_NONE        0

/** Plage de texte avec un style appliqué. */
typedef struct {
    size_t start;       /**< Offset de début (octets) */
    size_t end;         /**< Offset de fin (exclusif) */
    unsigned int style; /**< Bitmask TextStyle */
} StyleRange;

/** Structure opaque du formatter. */
typedef struct Formatter Formatter;

/** @brief Crée un formatter vide. */
Formatter *formatter_create(void);

/** @brief Libère le formatter. */
void formatter_destroy(Formatter *fmt);

/** @brief Applique un style à une plage. */
bool formatter_apply(Formatter *fmt, size_t start, size_t end, unsigned int style);

/** @brief Retire un style d'une plage. */
bool formatter_remove(Formatter *fmt, size_t start, size_t end, unsigned int style);

/** @brief Retourne le style à une position donnée. */
unsigned int formatter_get_style_at(const Formatter *fmt, size_t pos);

/** @brief Nombre de plages stylées. */
size_t formatter_range_count(const Formatter *fmt);

/** @brief Accès à une plage par index. */
const StyleRange *formatter_get_range(const Formatter *fmt, size_t index);

/** @brief Vérifie si un style est appliqué à une position. */
bool formatter_has_style(const Formatter *fmt, size_t pos, unsigned int style);

/** @brief Callback pour itérer sur les ranges stylés. */
typedef void (*FormatterIterFn)(size_t start, size_t end, unsigned int style, void *user_data);

/** @brief Itère sur tous les ranges stylés. */
void formatter_iterate(const Formatter *fmt, FormatterIterFn fn, void *user_data);

#endif /* FORMATTER_H */
