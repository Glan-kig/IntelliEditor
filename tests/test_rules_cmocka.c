#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "../include/rules.h"

// Rappel des prototypes
RuleStatus check_section_exists(const char* text, const char* section);
RuleStatus check_regex_forbidden(const char* text, const char* pattern);

// --- Test de la détection de section ---
static void test_section_detection(void **state) {
    (void) state; // Inutilisé ici
    
    const char* text = "Introduction\nMon super rapport.";
    
    // assert_int_equal vérifie que le retour est bien STATUS_CONFORME (0)
    assert_int_equal(check_section_exists(text, "Introduction"), STATUS_CONFORME);
    assert_int_equal(check_section_exists(text, "Conclusion"), STATUS_NON_CONFORME);
}

// --- Test des Regex (PCRE2) ---
static void test_regex_forbidden_words(void **state) {
    (void) state;
    
    const char* pattern = "\\b(je|moi|mon)\\b";
    
    // Cas invalide (usage de "je")
    assert_int_equal(check_regex_forbidden("Je pense que...", pattern), STATUS_NON_CONFORME);
    
    // Cas valide (neutre)
    assert_int_equal(check_regex_forbidden("L'analyse montre que...", pattern), STATUS_CONFORME);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_section_detection),
        cmocka_unit_test(test_regex_forbidden_words),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}