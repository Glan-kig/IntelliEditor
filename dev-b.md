# DEV-B : Interface Utilisateur GTK

## Description de ma partie
Je suis en charge de l’interface utilisateur du projet **IntelliEditor**.  
Ma partie couvre principalement :

- `src/ui/main_window.c`  
  - création de la fenêtre principale  
  - menus et barre d’outils  
  - gestion des actions utilisateur (nouveau, ouvrir, sauvegarder, exporter RTF, corrections, chargement des règles)  
  - fonctionnalités avancées : zoom (50–100), marges type Word, sauts de page, bascule thème clair/sombre, gestion police/couleur/styles  
- `src/ui/rules_panel.c`  
  - construction du panneau latéral des règles  
  - affichage de l’état de conformité et des issues  
  - mise à jour dynamique via `rules_panel_update_from_report()`  
- `src/utils/rules_stub.c`  
  - stub minimal pour charger et appliquer des règles factices  
  - génération d’issues fictives pour tester l’UI  
- `include/ui.h`  
  - déclaration des fonctions UI partagées  
- `include/rules.h`  
  - définition des structures `Rule`, `RuleIssue`, `RuleReport` et API de règles

## Ce que j’ai fait
1. Nettoyage de `src/ui/main_window.c` pour supprimer les doublons et corriger la compilation.  
2. Ajout du header `include/ui.h` pour déclarer `create_main_window()` et `create_rules_panel()`.  
3. Intégration du panneau de règles avec `create_rules_panel()` et mise à jour via `rules_panel_update_from_report()`.  
4. Ajout des fonctionnalités **zoom limité (50–100)**, **marges type Word**, **sauts de page**, **basculer thème clair/sombre**.  
5. Création d’un **stub de règles** (`rules_stub.c`) qui génère des issues factices pour tester l’UI.  
6. Vérification que l’UI compile et que le panneau affiche correctement les issues.  

## Commande de test

Pour tester uniquement la partie interface utilisateur :

```bash
cd /home/calebkindji/IntelliEditor
cd /home/calebkindji/IntelliEditor
gcc -Iinclude src/main.c src/ui/main_window.c src/ui/rules_panel.c src/utils/rules_stub.c -o text $(pkg-config --cflags --libs gtk+-3.0) -lm
