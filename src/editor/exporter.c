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
#include <glib.h>
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

bool export_pdf(const GapBuffer *gb, const Formatter *fmt, FILE *f);

bool exporter_save(const GapBuffer *gb, const Formatter *fmt,
                   const char *filepath, ExportFormat format) {
    if (!gb || !filepath) return false;
    FILE *f = fopen(filepath, "wb");
    if (!f) return false;

    bool ok = false;
    switch (format) {
        case EXPORT_TXT: ok = export_txt(gb, f);       break;
        case EXPORT_RTF: ok = export_rtf(gb, fmt, f);  break;
        case EXPORT_PDF: ok = export_pdf(gb, fmt, f);  break;
        case EXPORT_IE:  ok = export_ie(gb, fmt, f);   break;
    }
    fclose(f);
    return ok;
}

static void pdf_escape_text(GString *out, const char *text, gsize len) {
    for (gsize i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c == '(' || c == ')' || c == '\\') {
            g_string_append_c(out, '\\');
            g_string_append_c(out, c);
        } else if (c == '\r') {
            continue;
        } else if (c == '\n') {
            /* Handled outside as a line break */
        } else if (c < 32 || c > 126) {
            g_string_append_c(out, '?');
        } else {
            g_string_append_c(out, c);
        }
    }
}

static bool export_pdf(const GapBuffer *gb, const Formatter *fmt, FILE *f) {
    (void)fmt;
    char *content = gap_buffer_to_string(gb);
    if (!content) return false;

    GString *stream = g_string_new(NULL);
    g_string_append(stream, "BT\n/F1 12 Tf\n50 800 Td\n");

    const char *line = content;
    while (line) {
        const char *next = strchr(line, '\n');
        g_string_append_c(stream, '(');
        if (next) {
            pdf_escape_text(stream, line, next - line);
        } else {
            pdf_escape_text(stream, line, strlen(line));
        }
        g_string_append(stream, ") Tj\n");

        if (!next) break;
        g_string_append(stream, "0 -14 Td\n");
        line = next + 1;
    }

    g_string_append(stream, "ET\n");

    long offsets[6];
    long xref_start;

    fprintf(f, "%%PDF-1.4\n%%\xFF\xFF\xFF\xFF\n");
    offsets[0] = ftell(f);
    fprintf(f, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");
    offsets[1] = ftell(f);
    fprintf(f, "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");
    offsets[2] = ftell(f);
    fprintf(f, "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>\nendobj\n");
    offsets[3] = ftell(f);
    fprintf(f, "4 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\n");
    offsets[4] = ftell(f);
    fprintf(f, "5 0 obj\n<< /Length %zu >>\nstream\n%sendstream\nendobj\n", stream->len, stream->str);
    xref_start = ftell(f);

    fprintf(f, "xref\n0 6\n");
    fprintf(f, "0000000000 65535 f \n");
    for (int i = 0; i < 5; ++i) {
        fprintf(f, "%010ld 00000 n \n", offsets[i]);
    }
    fprintf(f, "trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n%ld\n%%%%EOF\n", xref_start);

    g_string_free(stream, TRUE);
    MEM_FREE(content);
    return true;
}

ExportFormat exporter_detect_format(const char *filepath) {
    if (!filepath) return EXPORT_TXT;
    const char *dot = strrchr(filepath, '.');
    if (!dot) return EXPORT_TXT;
    if (strcmp(dot, ".rtf") == 0) return EXPORT_RTF;
    if (strcmp(dot, ".pdf") == 0) return EXPORT_PDF;
    if (strcmp(dot, ".ie")  == 0) return EXPORT_IE;
    return EXPORT_TXT;
}
