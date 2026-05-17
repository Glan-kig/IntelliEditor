# DEV-B : Interface Utilisateur GTK

## Description de ma partie
Je suis en charge de l'interface utilisateur du projet IntelliEditor.
Ma partie couvre principalement :

- `src/ui/main_window.c`
  - création de la fenêtre principale
  - menus et barre d'outils
  - gestion des actions utilisateur (nouveau, ouvrir, sauvegarder, exporter RTF, corrections, chargement des règles)
- `src/ui/rules_panel.c`
  - construction du panneau latéral des règles
  - affichage de l'état de conformité
- `include/ui.h`
  - déclaration des fonctions UI partagées

## Ce que j'ai fait

1. J'ai nettoyé `src/ui/main_window.c` pour supprimer les définitions en double qui causaient des erreurs de compilation.
2. J'ai ajouté un header `include/ui.h` pour déclarer `create_main_window()` et `create_rules_panel()`.
3. J'ai corrigé l'intégration du panneau de règles avec `create_rules_panel()`.
4. J'ai vérifié que l'UI compile correctement.

## Commande de test

Pour tester uniquement la partie interface utilisateur, utiliser :

```bash
cd /home/calebkindji/IntelliEditor
gcc -Iinclude src/main.c src/ui/main_window.c src/ui/rules_panel.c src/utils/rules_stub.c -o text $(pkg-config --cflags --libs gtk+-3.0)
```

### Notes pour l'équipe
- Cette commande teste la partie UI sans dépendre de `cJSON` ou des fichiers de règles complets.
- Si GTK 3 n'est pas installé sur le système, installez-le d'abord (`gtk+-3.0` / `gtk3-devel`).
- Le binaire généré est `text`.

## Résultat attendu

- la compilation doit se terminer sans erreur
- un exécutable `text` doit apparaître à la racine du projet
- l'application doit pouvoir démarrer et afficher la fenêtre principale
