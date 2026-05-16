#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char server_url[256];   // URL du serveur llama.cpp (ex: http://localhost:8080)
    char rules_path[256];   // Chemin vers le fichier rules.json
    int timeout_seconds;    // Temps d'attente max pour l'IA
    float temperature;      // Créativité de l'IA (0.0 à 1.0)
} AppConfig;

// Charge la configuration depuis un fichier JSON
int load_config(const char *filename, AppConfig *config);

#endif