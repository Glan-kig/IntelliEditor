/**
 * @file exporter.c
 * @brief Export multi-format : .txt, .rtf, .ie.
 *
 * - .txt : texte brut UTF-8 (formatage ignoré).
 * - .rtf : Rich Text Format minimal (gras, italique, souligné).
 * - .ie  : format natif IntelliEditor (texte + ranges sérialisés).
 *
 * @author DEV-A
 */
#include "exporter.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Export .txt
 * -------------------------------------------------------------------------- */

static bool export_txt(const GapBuffer *gb, FILE *f) {
    char *content = gap_buffer_to_string(gb);
    if (!content) return false;
    size_t len = strlen(content);
    bool ok = (fwrite(content, 1, len, f) == len);
    MEM_FREE(content);
    return ok;
}

/* --------------------------------------------------------------------------
 * Export .rtf
 * -------------------------------------------------------------------------- */

/** Écrit un caractère en échappant les méta-caractères RTF. */
static void rtf_write_char(FILE *f, unsigned char c) {
    if (c == '\\' || c == '{' || c == '}') {
        fprintf(f, "\\%c", c);
    } else if (c == '\n') {
        fputs("\\par\n", f);
    } else if (c < 0x80) {
        fputc(c, f);
    } else {
        /* UTF-8 → \uN avec valeur signée 16 bits */
        fprintf(f, "\\u%d?", (int)c);
    }
}

/** Émet les balises RTF d'ouverture pour un style. */
static void rtf_open_style(FILE *f, unsigned int s) {
    if (s & STYLE_BOLD)      fputs("\\b ",  f);
    if (s & STYLE_ITALIC)    fputs("\\i ",  f);
    if (s & STYLE_UNDERLINE) fputs("\\ul ", f);
}

/** Émet les balises de fermeture. */
static void rtf_close_style(FILE *f, unsigned int s) {
    if (s & STYLE_UNDERLINE) fputs("\\ul0 ", f);
    if (s & STYLE_ITALIC)    fputs("\\i0 ",  f);
    if (s & STYLE_BOLD)      fputs("\\b0 ",  f);
}

static bool export_rtf(const GapBuffer *gb, const Formatter *fmt, FILE *f) {
    char *content = gap_buffer_to_string(gb);
    if (!content) return false;
    size_t len = strlen(content);

    /* En-tête RTF minimal */
    fputs("{\\rtf1\\ansi\\deff0\n", f);

    unsigned int current = STYLE_NONE;
    for (size_t i = 0; i < len; i++) {
        unsigned int s = fmt ? formatter_get_style_at(fmt, i) : STYLE_NONE;
        if (s != current) {
            rtf_close_style(f, current);
            rtf_open_style(f, s);
            current = s;
        }
        rtf_write_char(f, (unsigned char)content[i]);
    }
    rtf_close_style(f, current);
    fputs("}\n", f);

    MEM_FREE(content);
    return true;
}

/* --------------------------------------------------------------------------
 * Export .ie (format natif)
 * -------------------------------------------------------------------------- */

/** Callback pour sérialiser un range. */
static void ie_write_range(size_t start, size_t end,
                           unsigned int style, void *user_data) {
    FILE *f = (FILE *)user_data;
    fprintf(f, "RANGE %zu %zu %u\n", start, end, (unsigned)style);
}

static bool export_ie(const GapBuffer *gb, const Formatter *fmt, FILE *f) {
    char *content = gap_buffer_to_string(gb);
    if (!content) return false;
    size_t len = strlen(content);

    /* En-tête : magic + version + taille du texte */
    fprintf(f, "IE_FORMAT_V1\nTEXT_LEN %zu\n", len);
    fwrite(content, 1, len, f);
    fputc('\n', f);

    /* Sérialisation des ranges */
    fprintf(f, "RANGES %zu\n", fmt ? formatter_range_count(fmt) : 0);
    if (fmt) formatter_iterate(fmt, ie_write_range, f);

    MEM_FREE(content);
    return true;
}

/* --------------------------------------------------------------------------
 * Dispatcher
 * -------------------------------------------------------------------------- */

bool exporter_save(const GapBuffer *gb, const Formatter *fmt,
                   const char *filepath, ExportFormat format) {
    if (!gb || !filepath) return false;
    FILE *f = fopen(filepath, "wb");
    if (!f) return false;

    bool ok = false;
    switch (format) {
        case EXPORT_TXT: ok = export_txt(gb, f);       break;
        case EXPORT_RTF: ok = export_rtf(gb, fmt, f);  break;
        case EXPORT_IE:  ok = export_ie(gb, fmt, f);   break;
    }
    fclose(f);
    return ok;
}

ExportFormat exporter_detect_format(const char *filepath) {
    if (!filepath) return EXPORT_TXT;
    const char *dot = strrchr(filepath, '.');
    if (!dot) return EXPORT_TXT;
    if (strcmp(dot, ".rtf") == 0) return EXPORT_RTF;
    if (strcmp(dot, ".ie")  == 0) return EXPORT_IE;
    return EXPORT_TXT;
}
