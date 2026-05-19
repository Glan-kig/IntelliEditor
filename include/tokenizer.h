#ifndef TOKENIZER_H
#define TOKENIZER_H

// Découpe le texte par ponctuation pour l'IA
void analyze_sentences(const char* full_text);

// Extrait spécifiquement une section (ex: "Introduction") pour le JSON R009
char* extract_section(const char* full_text, const char* section_name);

#endif