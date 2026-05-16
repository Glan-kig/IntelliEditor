# DEV-B : Interface utilisateur (UI) / Expérience utilisateur

## Contexte
Ce module correspond au rôle **DEV-B** dans le projet IntelliEditor.  
Il est dédié à la conception et au développement de l’interface graphique (GTK) et à l’expérience utilisateur sur Fedora/Linux.

## Explication
Le rôle DEV-B consiste à réaliser une interface utilisateur robuste pour l’éditeur de texte. Le travail inclut la création d’une fenêtre principale, d’une barre de menus, d’une barre d’outils, d’une zone d’édition textuelle et d’un panneau de règles. L’objectif est de proposer une expérience fluide pour la rédaction et la correction, tout en garantissant la compatibilité avec l’environnement Fedora/Linux et GTK.

---

## Objectifs atteints jusqu’ici
- Création de la **fenêtre principale** avec GTK.
- Ajout d’une **barre de menus** :
  - Fichier (Nouveau, Ouvrir, Sauvegarder, Quitter).
  - Affichage (Thème clair/sombre).
  - Aide (À propos).
- Mise en place d’une **barre d’outils** avec boutons :
  - Nouveau, Ouvrir, Sauvegarder, Corriger, Reformuler.
- Intégration d’une **zone d’édition** (`GtkTextView`).
- Ajout d’un **panneau des règles** séparé (`rules_panel.c`).
- Création d’une **barre de statut dynamique** :
  - Affiche ligne, colonne, nombre de mots, encodage, thème.
- Gestion des **événements de texte** :
  - Mise à jour automatique du statut quand le contenu change.
- Ajout de **boîtes de dialogue** pour :
  - Ouvrir un fichier texte.
  - Sauvegarder un fichier texte.
  - Afficher les informations “À propos”.

---

## Organisation des fichiers
- `main_window.h` → Prototypes et organisation du module UI.
- `main_window.c` → Fenêtre principale, menus, toolbar, zone d’édition, statut.
- `rules_panel.c` → Panneau des règles (labels et icônes).
- `DEV-B.md` → Documentation du rôle DEV-B.

---

## Prochaines étapes
- Charger les règles dynamiquement depuis un fichier JSON.
- Ajouter des icônes colorées (✓ vert, ⚠ orange, ✗ rouge).
- Améliorer la zone d’édition (polices, couleurs, thèmes).
- Connecter les boutons **Corriger/Reformuler** au module NLP (DEV-C).
- Préparer un système de préférences utilisateur (fichiers de configuration).

---

## Commit type
Exemple de commit pour ce module :

```bash
git add src/ui/*
git commit -m "DEV-B : Interface GTK complète avec menus, toolbar, panneau des règles et statut dynamique"
git push origin dev-b-ui
