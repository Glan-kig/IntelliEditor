#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// Structure d'une tâche pour l'IA
typedef struct Task {
    char *text;
    char *instruction;
    struct Task *next;
} LLMTask;

LLMTask *queue = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // Verrou pour la sécurité
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;   // Signal de réveil

// Ligne par ligne : Ajoute une demande d'analyse dans la file
void push_llm_task(const char *t, const char *i) {
    pthread_mutex_lock(&lock); // On bloque l'accès aux autres threads
    LLMTask *new_t = malloc(sizeof(LLMTask));
    new_t->text = strdup(t);
    new_t->instruction = strdup(i);
    new_t->next = queue;
    queue = new_t;
    pthread_cond_signal(&cond); // On réveille l'ouvrier (Worker)
    pthread_mutex_unlock(&lock); // On libère l'accès
}