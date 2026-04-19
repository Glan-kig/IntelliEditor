# IntelliEditor
Notre projet est un Éditeur de texte intelligent, Le but est d'aider les étudiants  a réaliser leur travail de fin de cycle sans trop se prendre la tete sur la norme, car l'IA intégrer va  s'assurer de faire respecter la norme Ecriture selon  celle qui sera imposer

# Structure du projet

```Arboressence
IntelliEditor/
├── src/
│   ├── main.c                # Point d'entrée (Initialisation GTK)
│   ├── editor/               # [DEV-A] Cœur de l'éditeur
│   │   ├── gap_buffer.c      # Gestion du texte en O(1)
│   │   └── undo_redo.c       # Pattern Command
│   ├── ui/                   # [DEV-B] Interface GTK
│   │   ├── main_window.c     # Fenêtre et menus
│   │   └── rules_panel.c     # Panneau latéral des règles
│   ├── nlp/                  # [DEV-C] Intelligence Linguistique
│   │   ├── hunspell_wrap.c   # Correction orthographique
│   │   └── llm_interface.c   # Liaison avec llama.cpp
│   ├── rules/                # [DEV-D] Moteur de règles
│   │   ├── rule_engine.c     # Logique de vérification
│   │   └── checkers/         # Vérificateurs (Regex, WordCount)
│   └── utils/                # Outils partagés
│       ├── config.c          # Lecture du .ini (dans ~/.config/intellieditor)
│       └── encoding.c        # Gestion UTF-8 [cite: 90]
├── include/                  # Fichiers .h
├── data/                     # Règles JSON et dictionnaires [cite: 197, 310]
├── models/                   # Modèles GGUF (ex: Mistral 7B) [cite: 294]
└── CMakeLists.txt            # Configuration du build
```