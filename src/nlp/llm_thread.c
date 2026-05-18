#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../include/nlp.h"
#include "../../include/llm_thread.h"

// Structure d'une tâche pour l'IA
typedef struct Task {
    char *text;
    char *instruction;
    struct Task *next;
} LLMTask;

LLMTask *queue = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // Verrou pour la sécurité
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;   // Signal de réveil

static int stop_thread = 0;
static pthread_t thread_id;
static int thread_started = 0;

// Ligne par ligne : Ajoute une demande d'analyse dans la file
void push_llm_task(const char *t, const char *i) {
    if (!t || !i) return;

    pthread_mutex_lock(&lock);

    LLMTask *new_t = malloc(sizeof(LLMTask));
    if (!new_t) {
        pthread_mutex_unlock(&lock);
        fprintf(stderr, "[ERROR] push_llm_task: échec malloc\n");
        return;
    }

    new_t->text = strdup(t);
    new_t->instruction = strdup(i);
    if (!new_t->text || !new_t->instruction) {
        free(new_t->text);
        free(new_t->instruction);
        free(new_t);
        pthread_mutex_unlock(&lock);
        fprintf(stderr, "[ERROR] push_llm_task: échec strdup\n");
        return;
    }

    new_t->next = queue;
    queue = new_t;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);
}
// Dans ce fichier, on va créer un thread qui tourne en permanence pour traiter les demandes d'analyse sémantique. Les autres parties du programme peuvent ajouter des tâches à la file, et le thread les traitera une par une.

// Cette fonction est le "cerveau" qui tourne en arrière-plan./qui tourne en boucle pour traiter les demandes d'analyse sémantique. Elle attend que des tâches soient ajoutées à la file, puis les traite en appelant la fonction d'analyse de l'IA.
void* llm_worker_func(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        while (queue == NULL && !stop_thread) {
            pthread_cond_wait(&cond, &lock);
        }

        if (queue == NULL && stop_thread) {
            pthread_mutex_unlock(&lock);
            break;
        }

        LLMTask* task = queue;
        queue = queue->next;
        pthread_mutex_unlock(&lock);
        
    
        // On appelle ta fonction de llm_client.c
        RuleStatus result = ask_llm_semantic_check(task->text, task->instruction);
        
        // Ici, on devrait envoyer le résultat à l'UI (DEV-B) ou au moteur de règles (DEV-D)
        printf("Analyse terminée pour : %s -> Statut : %d\n", task->text, result);
        
        //  Libération de la mémoire de la tâche
        free(task->text);
        free(task->instruction);
        free(task);
    }
    return NULL;
}

// Fonction pour démarrer l'ouvrier au lancement du programme
void start_llm_thread() {
    pthread_t thread_id;
    int result = pthread_create(&thread_id, NULL, llm_worker_func, NULL);
    if (result != 0) {
        fprintf(stderr, "[ERROR] start_llm_thread: pthread_create failed\n");
        return;
    }
    thread_started = 1;
    pthread_detach(thread_id); // Le thread vit sa vie ou fonctionne de manière autonome.
}

// Fonction pour arrêter proprement le thread au moment de la fermeture du programme
void stop_llm_thread() {
    pthread_mutex_lock(&lock);
    stop_thread = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);

    if (thread_started) {
        pthread_join(thread_id, NULL);
        thread_started = 0;
    }

    // Optionnel : libérer les tâches restantes si on veut un arrêt immédiat
    while (queue) {
        LLMTask *task = queue;
        queue = queue->next;
        free(task->text);
        free(task->instruction);
        free(task);
    }
}