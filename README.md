# IntelliEditor
IntelliEditor est un éditeur de texte intelligent conçu pour fonctionner hors ligne sur Fedora/Linux avec une interface GTK. Il combine édition de texte et assistance à la rédaction en français, avec chargement de règles, correction orthographique et panneau de conformité.

# Fonctionnalités

- Édition de texte avec interface GTK
- Chargement de règles JSON personnalisables
- Panneau de conformité avec statuts
- Correction orthographique en français
- Export texte et RTF
- Interface entièrement en français

# Structure du projet

```Arborescence
IntelliEditor/
├── src/
│   ├── main.c
│   ├── editor/
│   │   ├── gap_buffer.c
│   │   └── undo_redo.c
│   ├── ui/
│   │   ├── main_window.c
│   │   └── rules_panel.c
│   ├── nlp/
│   │   ├── hunspell_wrap.c
│   │   └── llm_interface.c
│   ├── rules/
│   │   ├── rule_engine.c
│   │   └── rules.c
│   └── utils/
│       ├── config.c
│       └── encoding.c
└── include/
    └── rules.h
```

# Environnement Fedora

Interface utilisateur GTK 3 sur Fedora/Linux. Pas de Win32.

# Installation

- GTK 3 : sudo dnf install gtk3 gtk3-devel
- cJSON : sudo dnf install cjson cjson-devel
- PCRE2 : sudo dnf install pcre2-devel
- Hunspell : sudo dnf install hunspell hunspell-devel
- gcc : sudo dnf install gcc

# Compilation

gcc src/ui/main_window.c src/ui/rules_panel.c src/rules/rules.c src/rules/rule_engine.c src/nlp/hunspell_wrap.c src/nlp/llm_interface.c src/editor/gap_buffer.c src/editor/undo_redo.c src/utils/config.c src/utils/encoding.c $(pkg-config --cflags --libs gtk+-3.0) -o intellieditor

# Notes

- UI avec barre de menus, barre d'outils, éditeur et panneau de règles.
- Barre de statut : mots, position, encodage.
- Chargement de règles JSON et export RTF.
