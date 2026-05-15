#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// Vérifie si une chaîne est en UTF-8 valide
int is_valid_utf8(const char *str);

// Compte le nombre réel de caractères (et non d'octets)
// Exemple : "été" = 5 octets en UTF-8 mais 3 caractères.
size_t count_utf8_characters(const char *str);

// Nettoie le texte des caractères non-imprimables
char* sanitize_text(const char *input);

#endif