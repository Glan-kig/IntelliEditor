#include <gtk/gtk.h>
#include "rules.h"

static void on_quit(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

static gchar* rtf_escape(const gchar *text) {
    GString *out = g_string_new(NULL);
    for (const gchar *p = text; *p; ++p) {
        switch (*p) {
            case '\\': g_string_append(out, "\\\\"); break;
            case '{': g_string_append(out, "\\{"); break;
            case '}': g_string_append(out, "\\}"); break;
            case '\n': g_string_append(out, "\\par\n"); break;
            default: g_string_append_c(out, *p); break;
        }
    }
    return g_string_free(out, FALSE);
}

static void update_statusbar(GtkWidget *window) {
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    GtkTextIter start, end, cursor;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));

    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    gchar **words = g_strsplit_set(text, " \t\n\r", -1);
    int word_count = 0;
    for (gchar **p = words; *p; ++p) {
        if (**p != '\0') {
            word_count += 1;
        }
    }
    g_strfreev(words);

    int line = gtk_text_iter_get_line(&cursor) + 1;
    int column = gtk_text_iter_get_line_offset(&cursor) + 1;
    g_free(text);

    gchar *status_text = g_strdup_printf("Mots : %d | Ligne %d, Col %d | UTF-8 | FR", word_count, line, column);
    GtkWidget *statusbar = g_object_get_data(G_OBJECT(window), "statusbar");
    guint context = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "editor-status");
    gtk_statusbar_pop(GTK_STATUSBAR(statusbar), context);
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), context, status_text);
    g_free(status_text);
}

static gboolean save_text_to_file(GtkTextBuffer *buffer, const gchar *filename) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *content = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    gboolean result = g_file_set_contents(filename, content, -1, NULL);
    g_free(content);
    return result;
}

static gboolean load_file_to_buffer(GtkTextBuffer *buffer, const gchar *filename) {
    gchar *content = NULL;
    GError *error = NULL;
    if (!g_file_get_contents(filename, &content, NULL, &error)) {
        g_printerr("Erreur de lecture : %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }
    gtk_text_buffer_set_text(buffer, content, -1);
    g_free(content);
    return TRUE;
}

static void on_new(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    gtk_text_buffer_set_text(buffer, "", -1);
    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Nouveau document créé");
}

static void on_open(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Ouvrir un fichier",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Ouvrir", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (load_file_to_buffer(buffer, filename)) {
            gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, filename);
            update_statusbar(window);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void on_save(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Enregistrer le fichier",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Enregistrer", GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "document.txt");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (save_text_to_file(buffer, filename)) {
            gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Document sauvegardé");
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void on_save_rtf(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *content = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    gchar *rtf = rtf_escape(content);
    g_free(content);

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Exporter en RTF",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Exporter", GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "document.rtf");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GString *file_content = g_string_new("{\\rtf1\\ansi\\deff0\n");
        g_string_append_printf(file_content, "%s", rtf);
        g_string_append(file_content, "}\n");
        g_file_set_contents(filename, file_content->str, -1, NULL);
        g_string_free(file_content, TRUE);
        gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Export RTF généré");
        g_free(filename);
    }

    g_free(rtf);
    gtk_widget_destroy(dialog);
}

static void on_load_rules(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Charger un fichier de règles",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Charger", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        RuleReport *report = load_rules(filename);
        if (report) {
            gchar *message = g_strdup_printf("Règles chargées : %d règles", report->rule_count);
            gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, message);
            g_free(message);
        }
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void on_correct(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Analyse orthographique en cours...");
    update_statusbar(window);
}

static void on_reformulate(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Reformulation en cours...");
}

static void on_about(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    const gchar *authors[] = {"IntelliEditor Team", NULL};
    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "IntelliEditor");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), "0.1");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), "Éditeur de texte intelligent avec correction et panneau de règles.");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_text_changed(GtkTextBuffer *buffer, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    update_statusbar(window);
}

GtkWidget* create_main_window(void) {
    GtkWidget *window, *vbox, *menubar;
    GtkWidget *toolbar;
    GtkWidget *statusbar;
    GtkWidget *hbox, *text_area, *rules_panel;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "IntelliEditor");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 700);
    g_signal_connect(window, "destroy", G_CALLBACK(on_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    menubar = gtk_menu_bar_new();
    GtkWidget *fileMi = gtk_menu_item_new_with_label("Fichier");
    GtkWidget *editMi = gtk_menu_item_new_with_label("Édition");
    GtkWidget *viewMi = gtk_menu_item_new_with_label("Affichage");
    GtkWidget *toolsMi = gtk_menu_item_new_with_label("Outils");
    GtkWidget *rulesMi = gtk_menu_item_new_with_label("Règles");
    GtkWidget *helpMi = gtk_menu_item_new_with_label("Aide");

    GtkWidget *fileMenu = gtk_menu_new();
    GtkWidget *viewMenu = gtk_menu_new();
    GtkWidget *toolsMenu = gtk_menu_new();
    GtkWidget *rulesMenu = gtk_menu_new();
    GtkWidget *helpMenu = gtk_menu_new();

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(viewMi), viewMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(toolsMi), toolsMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(rulesMi), rulesMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpMi), helpMenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), editMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), viewMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), toolsMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), rulesMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), helpMi);

    GtkWidget *newMi = gtk_menu_item_new_with_label("Nouveau");
    GtkWidget *openMi = gtk_menu_item_new_with_label("Ouvrir...");
    GtkWidget *saveMi = gtk_menu_item_new_with_label("Enregistrer...");
    GtkWidget *saveRtfMi = gtk_menu_item_new_with_label("Exporter en RTF...");
    GtkWidget *quitMi = gtk_menu_item_new_with_label("Quitter");

    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), newMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveRtfMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);

    GtkWidget *correctMi = gtk_menu_item_new_with_label("Corriger");
    GtkWidget *reformulateMi = gtk_menu_item_new_with_label("Reformuler");
    GtkWidget *loadRulesMi = gtk_menu_item_new_with_label("Charger règles...");
    GtkWidget *aboutMi = gtk_menu_item_new_with_label("À propos");

    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), correctMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), reformulateMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(rulesMenu), loadRulesMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), aboutMi);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    g_signal_connect(newMi, "activate", G_CALLBACK(on_new), window);
    g_signal_connect(openMi, "activate", G_CALLBACK(on_open), window);
    g_signal_connect(saveMi, "activate", G_CALLBACK(on_save), window);
    g_signal_connect(saveRtfMi, "activate", G_CALLBACK(on_save_rtf), window);
    g_signal_connect(quitMi, "activate", G_CALLBACK(on_quit), NULL);
    g_signal_connect(correctMi, "activate", G_CALLBACK(on_correct), window);
    g_signal_connect(reformulateMi, "activate", G_CALLBACK(on_reformulate), window);
    g_signal_connect(loadRulesMi, "activate", G_CALLBACK(on_load_rules), window);
    g_signal_connect(aboutMi, "activate", G_CALLBACK(on_about), window);

    toolbar = gtk_toolbar_new();
    GtkToolItem *newTb = gtk_tool_button_new(NULL, "Nouveau");
    GtkToolItem *openTb = gtk_tool_button_new(NULL, "Ouvrir");
    GtkToolItem *saveTb = gtk_tool_button_new(NULL, "Enregistrer");
    GtkToolItem *correctTb = gtk_tool_button_new(NULL, "Corriger");
    GtkToolItem *reformTb = gtk_tool_button_new(NULL, "Reformuler");
    GtkToolItem *loadRulesTb = gtk_tool_button_new(NULL, "Règles");

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), newTb, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), openTb, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), saveTb, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), correctTb, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), reformTb, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), loadRulesTb, -1);

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    g_signal_connect(newTb, "clicked", G_CALLBACK(on_new), window);
    g_signal_connect(openTb, "clicked", G_CALLBACK(on_open), window);
    g_signal_connect(saveTb, "clicked", G_CALLBACK(on_save), window);
    g_signal_connect(correctTb, "clicked", G_CALLBACK(on_correct), window);
    g_signal_connect(reformTb, "clicked", G_CALLBACK(on_reformulate), window);
    g_signal_connect(loadRulesTb, "clicked", G_CALLBACK(on_load_rules), window);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
    gtk_widget_set_hexpand(hbox, TRUE);
    gtk_widget_set_vexpand(hbox, TRUE);

    GtkWidget *text_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_hexpand(text_scrolled, TRUE);
    gtk_widget_set_vexpand(text_scrolled, TRUE);
    text_area = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_area), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(text_area), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_area), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_area), 8);
    gtk_container_add(GTK_CONTAINER(text_scrolled), text_area);

    rules_panel = create_rules_panel();
    gtk_widget_set_size_request(rules_panel, 300, 0);

    gtk_box_pack_start(GTK_BOX(hbox), text_scrolled, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), rules_panel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    statusbar = gtk_statusbar_new();
    g_object_set_data(G_OBJECT(window), "statusbar", statusbar);
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "editor-status");
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "Prêt - UTF-8 - FR");
    gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    g_signal_connect(buffer, "changed", G_CALLBACK(on_text_changed), window);

    g_object_set_data(G_OBJECT(window), "text_area", text_area);
    g_object_set_data(G_OBJECT(window), "statusbar", statusbar);

    gtk_widget_show_all(window);
    update_statusbar(window);
    return window;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GtkWidget *window = create_main_window();
    gtk_main();
    return 0;
}
