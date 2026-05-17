#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../../include/utils.h"

// Compte le nombre de caractères réels
// En UTF-8, les caractères accentués font plusieurs octets.
size_t count_utf8_characters(const char *str) {
    size_t count = 0;
    if (!str) return 0;
    const unsigned char *p = (const unsigned char *)str;
    while (*p) {
        if ((*p & 0xC0) != 0x80) count++;
        p++;
    }
    return count;
}

// Vérifie la validité UTF-8 (supporte 1..4 octets)
int is_valid_utf8(const char *str) {
    if (!str) return 0;
    const unsigned char *p = (const unsigned char *)str;
    while (*p) {
        if (*p <= 0x7F) {
            p++;
        } else if ((*p & 0xE0) == 0xC0) {
            // 2-octets
            if (p[1] == 0 || (p[1] & 0xC0) != 0x80) return 0;
            p += 2;
        } else if ((*p & 0xF0) == 0xE0) {
            // 3-octets
            if (p[1] == 0 || p[2] == 0) return 0;
            if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) return 0;
            p += 3;
        } else if ((*p & 0xF8) == 0xF0) {
            // 4-octets
            if (p[1] == 0 || p[2] == 0 || p[3] == 0) return 0;
            if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 || (p[3] & 0xC0) != 0x80) return 0;
            p += 4;
        } else {
            return 0; // octet invalide
        }
    }
    return 1;
}

// Nettoie le texte avant analyse en préservant les séquences UTF-8 valides
char* sanitize_text(const char *input) {
    if (!input) return NULL;
    size_t len = strlen(input);
    char *output = malloc(len + 1);
    if (!output) return NULL;
    size_t in = 0, out = 0;

    while (input[in]) {
        unsigned char c = (unsigned char)input[in];
        if (c <= 0x7F) {
            // ASCII
            if (isprint(c) || input[in] == '\n' || input[in] == '\r' || input[in] == '\t') {
                output[out++] = input[in];
            }
            in++;
        } else {
            // Détecter la longueur de la séquence UTF-8
            int seq = 0;
            if ((c & 0xE0) == 0xC0) seq = 2;
            else if ((c & 0xF0) == 0xE0) seq = 3;
            else if ((c & 0xF8) == 0xF0) seq = 4;
            else { in++; continue; }

            int ok = 1;
            for (int k = 1; k < seq; k++) {
                if (input[in + k] == '\0' || ((input[in + k] & 0xC0) != 0x80)) { ok = 0; break; }
            }
            if (ok) {
                for (int k = 0; k < seq; k++) output[out++] = input[in++];
            } else {
                // séquence invalide -> ignorer l'octet leader
                in++;
            }
        }
    }
    output[out] = '\0';
    return output;
}