/**
 * @file exporter.h
 * @brief Export du document vers .txt, .rtf, .ie.
 * @author DEV-A
 */
#ifndef EXPORTER_H
#define EXPORTER_H

#include <stdbool.h>
#include "gap_buffer.h"
#include "formatter.h"

/** Formats d'export supportés. */
typedef enum {
    EXPORT_TXT,   /**< Texte brut UTF-8 */
    EXPORT_RTF,   /**< Rich Text Format */
    EXPORT_IE     /**< Format natif IntelliEditor (binaire) */
} ExportFormat;

/**
 * @brief Exporte le contenu d'un Gap Buffer vers un fichier.
 * @param gb Buffer source.
 * @param fmt Formatter (peut être NULL pour .txt).
 * @param filepath Chemin de sortie.
 * @param format Format cible.
 * @return true si succès.
 */
bool exporter_save(const GapBuffer *gb, const Formatter *fmt,
                   const char *filepath, ExportFormat format);

#endif /* EXPORTER_H */
