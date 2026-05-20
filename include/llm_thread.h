#ifndef LLM_THREAD_H
#define LLM_THREAD_H

#include "rules.h"

// Ajoute une tâche dans la file d'attente
void push_llm_task(const char *text, const char *instruction);

// Callback optionnel pour récupérer le résultat côté UI (thread-safe via g_idle_add)
typedef void (*LLMResultCallback)(const char *text,
                                   const char *instruction,
                                   RuleStatus status,
                                   void *user_data);

// Enregistre un callback à appeler quand une tâche est terminée.
// user_data est renvoyé tel quel au callback.
void set_llm_result_callback(LLMResultCallback cb, void *user_data);

// Démarre le thread ouvrier
void start_llm_thread();

void stop_llm_thread();

#endif
