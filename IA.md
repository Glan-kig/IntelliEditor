hunspell_wrap.c / pour la correction mot a mot (pas d'accent)
tokezer.c pour parcourir le texte
llm_thread.c / veille en arriere plan a ce llama soit fluide
llm_thread.h /pour l asynchronisme
llm_prompts.c / pour adapter la question selon le regle du JSON(R009)
llm_json_parser.c/ pour extraire la reponse de l IA
nlp_engine.c/ pour la liaison avec Dec-D pour rcevoir de lui
/home/stone/llama.cpp/build/bin/llama-server -m ~/Documents/c/IntelliEditor/models/Llama-3.2-1B-Instruct-Q4_K_M.gguf --port 8000 / chemin pour lancer le serveur 
Llama-3.2-1B-Instruct-Q4_K_M.gguf / nom du modele