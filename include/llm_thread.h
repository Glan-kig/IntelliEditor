#ifndef LLM_THREAD_H
#define LLM_THREAD_H

// Ajoute une tâche dans la file d'attente
void push_llm_task(const char *text, const char *instruction);

// Démarre le thread ouvrier
void start_llm_thread();

void stop_llm_thread();

#endif