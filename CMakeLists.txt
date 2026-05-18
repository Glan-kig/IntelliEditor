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

# Pour PCRE2 et CMocka, on peut utiliser des méthodes simples
find_library(PCRE2_LIB pcre2-8 REQUIRED)
find_library(CMOCKA_LIB cmocka)

# 4. Définition des répertoires d'inclusion (headers)
include_directories(include ${GTK3_INCLUDE_DIRS} ${CJSON_INCLUDE_DIRS})

# 5. Liste des fichiers sources du moteur (DEV-D)
set(RULES_SRC
    src/rules/rules.c
    src/rules/rule_engine.c
    src/rules/checkers/regex_checker.c
)

# 6. Création de l'exécutable principal
add_executable(intelli_engine src/rules/test_main.c ${RULES_SRC})

# 7. Liaison des bibliothèques (Linker)
target_link_libraries(intelli_engine 
    ${GTK3_LIBRARIES} 
    ${CJSON_LIBRARIES} 
    ${PCRE2_LIB}
)

# 8. Section pour les tests unitaires (CMocka)
if(CMOCKA_LIB)
    add_executable(unit_tests tests/test_rules_cmocka.c ${RULES_SRC})
    target_link_libraries(unit_tests ${CMOCKA_LIB} ${PCRE2_LIB} ${CJSON_LIBRARIES})
endif()