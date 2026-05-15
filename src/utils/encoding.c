#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../../include/utils.h"

// Compte le nombre de caractères réels
// En UTF-8, les caractères accentués font plusieurs octets.
size_t count_utf8_characters(const char *str) {
    size_t count = 0;
    while (*str) {
        // En UTF-8, un nouveau caractère commence si l'octet 
        // ne commence pas par les bits 10xxxxxx
        if ((*str & 0xc0) != 0x80) {
            count++;
        }
        str++;
    }
    return count;
}

// Vérifie la validité UTF-8 (simple version)
int is_valid_utf8(const char *str) {
    if (!str) return 0;
    const unsigned char *bytes = (const unsigned char *)str;
    while (*bytes) {
        if (bytes[0] <= 0x7F) {
            bytes += 1;
        } else if ((bytes[0] & 0xE0) == 0xC0) {
            if ((bytes[1] & 0xC0) != 0x80) return 0;
            bytes += 2;
        } else if ((bytes[0] & 0xF0) == 0xE0) {
            if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80) return 0;
            bytes += 3;
        } else {
            return 0; // Encodage non supporté ou invalide
        }
    }
    return 1;
}

// Nettoie le texte avant analyse
char* sanitize_text(const char *input) {
    if (!input) return NULL;
    size_t len = strlen(input);
    char *output = malloc(len + 1);
    size_t j = 0;

    for (size_t i = 0; i < len; i++) {
        // On garde les caractères imprimables, les tabulations et sauts de ligne
        if (isprint((unsigned char)input[i]) || input[i] == '\n' || input[i] == '\r' || input[i] == '\t') {
            output[j++] = input[i];
        }
    }
    output[j] = '\0';
    return output;
}