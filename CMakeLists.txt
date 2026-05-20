# 1. Version minimale de CMake et nom du projet
cmake_minimum_required(VERSION 3.10)
project(IntelliEditor C)

# 2. Standard du langage C (C11 comme demandé dans le PDF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 3. Recherche des dépendances (Les "packages")
# CMake va chercher les fichiers .pc (pkg-config) sur la machine
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(CJSON REQUIRED libcjson)

# libcurl
pkg_check_modules(CURL REQUIRED libcurl)

# Pour PCRE2 et CMocka, on peut utiliser des méthodes simples
find_library(PCRE2_LIB pcre2-8 REQUIRED)
find_library(CMOCKA_LIB cmocka)

# Hunspell
find_library(HUNSPELL_LIB hunspell REQUIRED)
set(HUNSPELL_LIBRARIES ${HUNSPELL_LIB})

# Threads::Threads (corrige l'erreur "target was not found")
find_package(Threads REQUIRED)

# 4. Définition des répertoires d'inclusion (headers)
include_directories(include ${GTK3_INCLUDE_DIRS} ${CJSON_INCLUDE_DIRS})

# 5. Liste des fichiers sources du moteur (DEV-D)
set(RULES_SRC
    src/editor/gap_buffer.c
    src/editor/formatter.c
    src/editor/exporter.c
    src/editor/search_replace.c
    src/editor/undo_redo.c
    src/utils/memory.c
    src/utils/config.c
    src/utils/encoding.c

    src/ui/rules_panel.c
    src/ui/main_window.c

    src/nlp/nlp_engine.c
    src/nlp/tokenizer.c
    src/nlp/llm_interface.c
    src/nlp/llm_prompts.c
    src/nlp/llm_json_parser.c
    src/nlp/llm_client.c
    src/nlp/hunspell_wrap.c
    src/nlp/llm_thread.c

    src/rules/rules.c
    src/rules/rule_engine.c
    src/rules/checkers/regex_checker.c

)

# 6. Création de l'exécutable principal
add_executable(intelli_engine src/main.c ${RULES_SRC})

# 7. Liaison des bibliothèques (Linker)
target_link_libraries(intelli_engine 
    ${GTK3_LIBRARIES} 
    ${CJSON_LIBRARIES} 
    ${PCRE2_LIB}
    ${HUNSPELL_LIBRARIES}
    ${CURL_LIBRARIES}
    Threads::Threads
    m
)