# Moteur de règles & intégration générale

## Objectif du dossier `src/rules/`

Le dossier `src/rules/` contient le moteur de règles principal de l'application. Il est responsable de :

- charger un jeu de règles JSON, 
- exécuter ces règles sur un texte, 
- produire un rapport de conformité calculé automatiquement.

## Structure du dossier

- `include/rules.h` : définitions des structures `Rule` et `RuleReport`, états de conformité, et prototypes des fonctions publiques.
- `src/rules/rules.c` : parsing JSON, construction de `RuleReport`, validation des champs et gestion robuste de la mémoire.
- `src/rules/rule_engine.c` : exécution du moteur de règles, mise à jour des scores, affichage du rapport et intégration des helpers.
- `src/rules/checkers/regex_checker.c` : vérificateur regex pour les patterns interdits.
- `src/rules/test_main.c` : programme de test simple pour valider le chargement et l’exécution du moteur.

## Définitions clés

### `Rule`

La structure `Rule` représente une règle unique :

- `id[10]` : identifiant de la règle (`R001`, `R002`, ...).
- `category[32]` : catégorie de la règle (structure, style, etc.).
- `description[256]` : description lisible de la règle.
- `check_type[32]` : type de vérification (`section_exists`, `regex_forbidden`, ...).
- `severity` : gravité (`SEVERITY_INFO`, `SEVERITY_WARNING`, `SEVERITY_ERROR`).
- `status` : état d’exécution (`STATUS_CONFORME`, `STATUS_AVERTISSEMENT`, `STATUS_NON_CONFORME`, `STATUS_EN_COURS`).
- `parameter` : paramètre flexible optionnel (alloué dynamiquement avec `strdup()` si présent).

### `RuleReport`

`RuleReport` contient :

- `rules` : tableau dynamique de `Rule`.
- `rule_count` : nombre total de règles chargées.
- `rules_ok` : nombre de règles validées (`STATUS_CONFORME`).

## Chargement des règles JSON

La fonction `load_rules(const char* filename)` de `src/rules/rules.c` :

- lit le fichier JSON avec une fonction `read_file()` sûre, 
- parse l’arborescence JSON avec `cJSON`,
- vérifie que `rules` existe et qu’il s’agit bien d’un tableau,
- alloue `rule_report->rules` avec `calloc()` pour éviter les valeurs indéterminées,
- valide chaque règle et copie les champs `id`, `description`, `check_type`, `severity`, et `parameter`.

### Champs obligatoires

Chaque règle JSON doit fournir :

- `id`
- `description`
- `check_type`

Le champ `severity` est optionnel ; s’il est absent ou invalide, la valeur par défaut est `SEVERITY_INFO`.

### Champ optionnel `parameter`

- `parameter` est lu seulement si c’est une chaîne JSON.
- Il est dupliqué avec `strdup()` et stocké dans `current_rule->parameter`.
- Si `parameter` est absent, `current_rule->parameter` reste `NULL`.

### Gestion mémoire

- En cas d’erreur lors du parsing, `load_rules()` libère proprement :
  - les paramètres `strdup()` déjà alloués,
  - le tableau de règles,
  - la structure `RuleReport`,
  - le contenu du fichier et l’arbre JSON.
- `free_rule_report(report)` libère aussi bien le tableau `rules` que chaque `parameter` attaché.

## Moteur d’exécution

### `run_rule_engine(RuleReport* report, const char* current_text)`

- valide `report` et `current_text`,
- nettoie le texte si possible via `sanitize_text()` (helpers utilitaires),
- vérifie la validité UTF-8 avec `is_valid_utf8()`,
- exécute les règles une par une en fonction de `check_type`.

### `run_full_diagnostic(RuleReport* report, const char* text)`

- valide également les paramètres d’entrée,
- utilise `analysis_text` pour remplacer le texte brut lorsqu’il est nettoyé ou validé,
- exécute `check_section_exists()` et `check_regex_forbidden()` sur le texte préparé,
- met à jour le score final avec `update_report_score(report)`.

### `update_report_score(RuleReport* report)`

- calcule `rules_ok` en comptant les règles avec `STATUS_CONFORME`.
- permet de garder le rapport à jour après chaque exécution.

### `print_compliance_report(RuleReport* report)`

- affiche un rapport lisible en console,
- liste les règles non conformes avec un statut visuel,
- utilise `report->rules_ok` et `report->rule_count`.

## Vérificateurs disponibles

### `check_section_exists(const char* document_text, const char* section_name)`

- cherche la présence d’une section donnée dans le document,
- utilise PCRE2 avec les options `PCRE2_UTF | PCRE2_UCP | PCRE2_CASELESS`,
- échappe `section_name` en littéral pour éviter l’interprétation regex,
- renvoie `STATUS_CONFORME` si trouvé, `STATUS_NON_CONFORME` sinon,
- renvoie `STATUS_AVERTISSEMENT` si la compilation du pattern échoue.

### `check_regex_forbidden(const char* document_text, const char* pattern)`

- compile une regex PCRE2 sur `pattern`,
- utilise une version optimisée interne `check_regex_forbidden_optimized()` avec `pcre2_match_data`,
- renvoie `STATUS_NON_CONFORME` si un match est trouvé,
- renvoie `STATUS_CONFORME` quand rien n’est trouvé.

## Exemple de format JSON

```json
{
  "rules": [
    {
      "id": "R001",
      "category": "structure",
      "description": "Vérifier la présence de l'introduction",
      "check_type": "section_exists",
      "severity": "error",
      "parameter": "Introduction"
    },
    {
      "id": "R002",
      "category": "style",
      "description": "Interdire les pronoms personnels",
      "check_type": "regex_forbidden",
      "severity": "warning",
      "parameter": "\\b(je|moi|mon|ma|mes)\\b"
    }
  ]
}
```

## Notes importantes

- `parameter` est alloué dynamiquement et doit être libéré avec `free()` dans `free_rule_report()`.
- `rules.c` utilise `calloc()` pour le tableau de règles afin de garantir une initialisation à zéro.
- `severity` non reconnu retombe sur `SEVERITY_INFO`.
- `run_rule_engine()` et `run_full_diagnostic()` effectuent des vérifications de texte plus strictes quand le module `utils` est disponible.
- `check_regex_forbidden()` ne force pas le support UTF-8 dans son appel actuel; la regex est compilée avec `PCRE2_CASELESS`.

## Ajouter une nouvelle règle

1. Ajouter une entrée JSON avec `check_type` correspondant.
2. Implémenter le vérificateur dans `src/rules/checkers/` ou directement dans `rule_engine.c`.
3. Ajouter le dispatch `if/else` dans `run_rule_engine()` et `run_full_diagnostic()`.
4. Ajouter des tests ou un exemple dans `test_main.c`.

## Compilation et test

Pour compiler et exécuter le programme de test :

```zsh
gcc -I./include \
    src/rules/rule_engine.c \
    src/rules/rules.c \
    src/rules/checkers/regex_checker.c \
    src/utils/encoding.c \
    src/rules/test_main.c \
    -lcjson -lpcre2-8 -lcurl \
  -o intelli_engine
```

## Résumé

Le dossier `src/rules/` implémente un moteur de validation textuelle extensible, avec parsing JSON, exécution paramétrée, et reporting de conformité. Il est conçu pour être facile à étendre et maintenir, tout en restant centré sur la séparation entre parsing, exécution et vérification.

