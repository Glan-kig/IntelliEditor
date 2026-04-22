# IntelliEditor - Traitement de Texte Intelligent (Version Linux)

**Projet C Avancé - L3 GL / UDBL (2025-2026)**

## 1. Présentation

IntelliEditor est un éditeur de texte performant écrit en C, conçu pour aider les étudiants dans la rédaction de documents académiques. Il intègre une analyse en temps réel de la conformité du document via un moteur de règles et une assistance par IA (LLM).

## 2. Équipe (Responsabilités)

- **Kalunga Kalwa Eddy** : DEV-A - Infrastructure & Éditeur (Gap Buffer).
- **Caleb** : DEV-B - Interface Utilisateur (GTK 3).
- **Imelda** : DEV-C - LLM & Moteur NLP (llama.cpp).
- **Glan Ilunga Kayembe** : DEV-D - Moteur de règles & Intégration générale.

## 3. Stack Technique (Linux)

- **Langage :** C11
- **Interface :** GTK 3
- **Regex :** PCRE2
- **Parser JSON :** cJSON
- **IA :** llama.cpp (Modèles GGUF)

## 4. Compilation et Installation
Le projet utilise un `Makefile` pour automatiser la gestion des dépendances sur Linux.

## 5. Structure du projet

```Arboressence
IntelliEditor/
├── src/
│   ├── main.c                          # Point d'entrée (Initialisation GTK)
│   ├── editor/                         # [DEV-A] Cœur de l'éditeur
│   │   ├── gap_buffer.c                # Gestion du texte en O(1)
│   │   └── undo_redo.c                 # Pattern Command
│   ├── ui/                             # [DEV-B] Interface GTK
│   │   ├── main_window.c               # Fenêtre et menus
│   │   └── rules_panel.c               # Panneau latéral des règles
│   ├── nlp/                            # [DEV-C] Intelligence Linguistique
│   │   ├── hunspell_wrap.c             # Correction orthographique
│   │   └── llm_interface.c             # Liaison avec llama.cpp
│   ├── rules/                          # [DEV-D] Moteur de règles
│   │   ├── rule_engine.c               # Logique de vérification
|   |   ├── rules.c                     # Lecture des fichiers en memoire
│   │   └── checkers/                   # Vérificateurs (Regex, WordCount)
|   |           └──regex_checker.c      # Vérificateurs d'expression regulieres
│   └── utils/                          # Outils partagés
│       ├── config.c                    # Lecture du .ini (dans ~/.config/intellieditor)
│       └── encoding.c                  # Gestion UTF-8 
├── include/                            # Fichiers .h
|       ├──rules.h                      # prototype du moteur de reglès
├── data/                               # Règles JSON et dictionnaires 
├── models/                             # Modèles GGUF (ex: Mistral 7B)
├── Mackfile                            # La compilation via make
└── CMakeLists.txt                      # Configuration du build
```

### Prérequis

Assurez-vous d'avoir installé les bibliothèques suivantes sur votre système :

- Distributions ayant pour base Debian :
```bash
sudo apt update
sudo apt install build-essential libcjson-dev libpcre2-dev libgtk-3-dev
```
- Distributions ayant pour base Redhat(ou fedora) :
```bash
sudo dnf install cjson cjson-devel pcre2-devel
```