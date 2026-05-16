# Moteur de règles & Intégration générale

## Objectif du dossier `rules/`

Le dossier `src/rules/` contient le moteur de règles principal de l'application. Il permet de charger des règles définies en JSON, de les exécuter sur un texte, puis de produire un rapport de conformité.

## Structure générale

- `include/rules.h` : définition des structures `Rule` et `RuleReport`, des états de conformité et des prototypes de fonctions.
- `src/rules/rules.c` : lecture du fichier JSON, création du rapport de règles et gestion robuste de la mémoire.
- `src/rules/rule_engine.c` : moteur principal d'exécution des règles, mise à jour du score, affichage du rapport et utilitaires.
- `src/rules/checkers/regex_checker.c` : vérificateur regex utilisant PCRE2 pour détecter des mots interdits.
- `src/rules/test_main.c` : programme de test standalone pour valider le chargement et l'exécution des règles.

## Principaux concepts

### `Rule` et `RuleReport`

- `Rule` représente une règle individuelle.
- `RuleReport` contient un tableau de règles (`rules`), le nombre total de règles (`rule_count`) et le nombre de règles conformes (`rules_ok`).
- Chaque règle a un champ `parameter` pour des données dynamiques (chaîne ou structure). Ce champ est utilisé par les vérificateurs.

### Chargement des règles

- `load_rules(const char* filename)` lit un fichier JSON, parse le tableau `rules`, et remplit un `RuleReport`.
- Le JSON attendu contient au minimum : `id`, `description`, `check_type` et `severity`.
- Les règles peuvent aussi inclure un champ optionnel `parameter`, utilisé par le moteur pour la recherche de section ou des patterns regex.

### Moteur d’exécution

- `run_rule_engine(RuleReport* report, const char* current_text)` applique les règles au texte fourni en utilisant les paramètres de chaque règle.
- `run_full_diagnostic(RuleReport* report, const char* text)` exécute également toutes les règles et met à jour le score, en suivant une logique paramétrée.
- `update_report_score(RuleReport* report)` calcule `rules_ok` en comptant les règles ayant `STATUS_CONFORME`.
- `print_compliance_report(RuleReport* report)` affiche un rapport formaté et liste les problèmes détectés.

## Vérificateurs

### `check_section_exists()`

- Vérifie si le document contient une section donnée.
- Dans la version actuelle, la recherche est implémentée avec PCRE2 pour gérer correctement l'UTF-8 et la casse Unicode.
- Le `section_name` est échappé avant compilation du pattern, ce qui garantit la recherche littérale.

### `check_regex_forbidden()`

- Utilise PCRE2 pour détecter des patterns interdits dans le texte.
- Renvoie `STATUS_NON_CONFORME` si une correspondance est trouvée.
- L'implémentation est optimisée en utilisant une version avec regex précompilée.

## Exemple de format JSON

```json
{
  "rules": [
    {
      "id": "R001",
      "description": "Vérifier la présence de l'introduction",
      "check_type": "section_exists",
      "severity": "error",
      "parameter": "Introduction"
    },
    {
      "id": "R002",
      "description": "Interdire les pronoms personnels",
      "check_type": "regex_forbidden",
      "severity": "warning",
      "parameter": "\\b(je|moi|mon|ma|mes)\\b"
    }
  ]
}
```

## Points importants

- `parameter` est dupliqué avec `strdup()` lorsque le JSON le contient.
- Il faut absolument appeler `free_rule_report(report)` après utilisation pour libérer `parameter`, `rules` et `report`.
- `rule_report->rules` est alloué avec `calloc()` pour garantir l'initialisation des champs à zéro et éviter des frees invalides en cas d'erreur partielle.
- Le support UTF-8 dans `check_section_exists()` est assuré par PCRE2 avec `PCRE2_UTF | PCRE2_UCP | PCRE2_CASELESS`.

## Extension du moteur

Pour ajouter un nouveau type de vérification :

1. Ajouter `check_type` dans les règles JSON.
2. Implémenter le vérificateur dans `src/rules/checkers/` ou directement dans `rule_engine.c`.
3. Ajouter un cas dans `run_rule_engine()` et `run_full_diagnostic()`.
4. Mettre à jour la documentation et les tests.

## Conseils de maintenance

- Préférer des tests unitaires pour chaque nouveau type de règle.
- Si le moteur devient plus complexe, séparer davantage les checkers dans des fichiers dédiés.
- Garder la logique de parsing JSON et la logique d'exécution séparées.
- Vérifier les performances si le texte analysé devient volumineux.

## Commande de test

Pour compiler et tester le moteur de règles :

```bash
gcc -I./include \
    src/rules/rule_engine.c \
    src/rules/rules.c \
    src/rules/checkers/regex_checker.c \
    src/rules/test_main.c \
    -lcjson -lpcre2-8 -o intelli_editor
./intelli_editor
```

## Résumé

Le dossier `rules/` contient le cœur du moteur de validation. Il est conçu pour être extensible, paramétrable via JSON, et offre une base solide pour des analyses de texte plus avancées (avec PCRE2, support UTF-8, et gestion mémoire robuste).

