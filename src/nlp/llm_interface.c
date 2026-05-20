#include "../../include/nlp.h"
#include "../../include/llm_thread.h"
#include "../../include/hunspell_wrap.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t g_llm_resp_lock = PTHREAD_MUTEX_INITIALIZER;
static char *g_last_llm_response = NULL;

void nlp_set_last_llm_response(const char* text) {
    pthread_mutex_lock(&g_llm_resp_lock);

    free(g_last_llm_response);
    g_last_llm_response = NULL;

    if (text) {
        g_last_llm_response = strdup(text);
    }

    pthread_mutex_unlock(&g_llm_resp_lock);
}

const char* nlp_get_last_llm_response(void) {
    static __thread char *thread_copy = NULL;

    pthread_mutex_lock(&g_llm_resp_lock);
    free(thread_copy);
    thread_copy = g_last_llm_response ? strdup(g_last_llm_response) : NULL;
    pthread_mutex_unlock(&g_llm_resp_lock);

    return thread_copy;
}

void nlp_system_init() {
    init_spell_checker(); // Vient de hunspell_wrap.c
    start_llm_thread();   // Vient de llm_thread.c
}

void nlp_system_cleanup() {
    stop_llm_thread();
    cleanup_spell_checker();

    pthread_mutex_lock(&g_llm_resp_lock);
    free(g_last_llm_response);
    g_last_llm_response = NULL;
    pthread_mutex_unlock(&g_llm_resp_lock);
}
