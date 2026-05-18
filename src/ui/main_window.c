
#include <gtk/gtk.h>
#include <math.h>
#include "rules.h"


/* Panel rules */
void rules_panel_update_from_report(GtkWidget *panel, const RuleReport *report);


/* ---------------------------
   Configuration zoom / marges
   --------------------------- */
static int current_zoom = 25;   /* valeur par défaut (taille police) */
static const int MIN_ZOOM = 10;
static const int MAX_ZOOM = 100;
static const int ZOOM_STEP = 5;

// Polices : nombre de pixels (approx) basé sur le pourcentage de zoom
static int zoom_to_font_size(int zoom_percent) {
    // 100% => 12px, MIN 50% => 8px, MAX 200% => 24px
    int size = (zoom_percent * 12) / 100;
    if (size < 6) size = 6;
    if (size > 40) size = 40;
    return size;
}


/* ---------------------------
   Helpers: theme, zoom, status
   --------------------------- */

static void apply_theme(GtkWidget *window, gboolean dark_mode) {
    GtkCssProvider *provider = gtk_css_provider_new();
    if (dark_mode) {
        gtk_css_provider_load_from_data(provider,
            "textview { background-color: #ffffff; color: #000000; }"
            "window { background-color: #f0f0f0; }", -1, NULL);
    } else {
        gtk_css_provider_load_from_data(provider,
            "textview { background-color: #ffffff; color: #000000; }"
            "window { background-color: #f0f0f0; }", -1, NULL);
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
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
        if (**p != '\0') word_count++;
    }
    g_strfreev(words);

    int line = gtk_text_iter_get_line(&cursor) + 1;
    int column = gtk_text_iter_get_line_offset(&cursor) + 1;
    g_free(text);

    gchar *status_text = g_strdup_printf("Mots : %d | Ligne %d, Col %d | UTF-8 | FR | Zoom %d%%",
                                         word_count, line, column, current_zoom);
    GtkWidget *statusbar = g_object_get_data(G_OBJECT(window), "statusbar");
    guint context = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "editor-status");
    gtk_statusbar_pop(GTK_STATUSBAR(statusbar), context);
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), context, status_text);
    g_free(status_text);
}

/* ---------------------------
   Fichiers: sauvegarde / lecture
   --------------------------- */

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

/* ---------------------------
   Actions de base
   --------------------------- */

static void on_quit(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

static void on_new(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    gtk_text_buffer_set_text(buffer, "", -1);
    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Nouveau document créé");
    update_statusbar(window);
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

/* Export RTF (simple) */
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

/* ---------------------------
   Outils: correction / reformulation
   --------------------------- */

static void on_load_rules(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);

    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkWidget *rules_panel = g_object_get_data(G_OBJECT(window), "rules_panel");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Charger un fichier de règles",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Charger", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        RuleReport *loaded = load_rules(filename);
        if (loaded) {
            gchar *message = g_strdup_printf("Règles chargées : %d règles", loaded->rule_count);
            gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, message);
            g_free(message);
        } else {
            gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Aucune règle chargée");
        }

        /* Wiring UI : remplir le panneau au chargement (stub produit des issues factices) */
        if (rules_panel) {
            RuleReport *ui_report = apply_rules_to_buffer(buffer);
            if (ui_report) {
                rules_panel_update_from_report(rules_panel, ui_report);
                free_rule_report(ui_report);
            } else {
                rules_panel_update_from_report(rules_panel, NULL);
            }
        }

        if (loaded) free_rule_report(loaded);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void on_correct(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);

    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkWidget *rules_panel = g_object_get_data(G_OBJECT(window), "rules_panel");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));

    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Analyse orthographique en cours...");
    update_statusbar(window);

    RuleReport *report = apply_rules_to_buffer(buffer);
    if (report) {
        rules_panel_update_from_report(rules_panel, report);
        free_rule_report(report);
    }

    update_statusbar(window);
}


static void on_reformulate(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);

    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkWidget *rules_panel = g_object_get_data(G_OBJECT(window), "rules_panel");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));

    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Reformulation en cours...");
    update_statusbar(window);

    /* Stub : on réutilise le diagnostic pour valider le wiring orthographe/reformulation */
    RuleReport *report = apply_rules_to_buffer(buffer);
    if (report) {
        rules_panel_update_from_report(rules_panel, report);
        free_rule_report(report);
    }

    update_statusbar(window);
}


/* Callbacks pour copier/coller/couper */
static void on_copy(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    gtk_text_buffer_copy_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
}

static void on_paste(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    gtk_text_buffer_paste_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), NULL, TRUE);
}

static void on_cut(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    gtk_text_buffer_cut_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), TRUE);
}

/* ---------------------------
   Règles: appliquer / exporter
   --------------------------- */

/* Appliquer les règles chargées au texte (implémentation minimale) */
static void on_apply_rules(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));

    /* Exemple minimal : ici tu peux appeler ta fonction d'application de règles si elle existe.
       Pour l'instant on se contente d'un message et d'une mise à jour du status. */
    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Application des règles...");
    update_statusbar(window);
}

/* Exporter un rapport simple des règles (fichier texte) */
static void on_export_rules(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Exporter rapport de règles",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Exporter", GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "rules_report.txt");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        /* Contenu minimal du rapport ; adapte selon ton format réel */
        const gchar *report = "Rapport de règles\nAucune donnée détaillée fournie.\n";
        g_file_set_contents(filename, report, -1, NULL);
        gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Rapport de règles exporté");
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

/* ---------------------------
   Affichage: zoom, marges, saut de page
   --------------------------- */

static void update_zoom(GtkWidget *text_area) {
    int px = zoom_to_font_size(current_zoom);

    // GTK/Pango attend des tailles en points. 1 point ~ 96dpi/72. On prend un facteur simple.
    double points = (double)px * 0.75;
    gchar *font_str = g_strdup_printf("Sans %d", (int)round(points));
    PangoFontDescription *font = pango_font_description_from_string(font_str);

    g_object_set(text_area, "font-desc", font, NULL);

    // Forcer un redraw/recalc du layout pour que le changement de police
    // soit bien appliqué visuellement.
    gtk_widget_queue_draw(text_area);
    gtk_widget_queue_resize(text_area);

    pango_font_description_free(font);
    g_free(font_str);
}




/* Zoom avant */
static void on_zoom_in(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    if (current_zoom < MAX_ZOOM) {
        current_zoom += ZOOM_STEP;
        if (current_zoom > MAX_ZOOM) current_zoom = MAX_ZOOM;
        update_zoom(text_area);
        update_statusbar(window);
    }
}

/* Zoom arrière */
static void on_zoom_out(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    if (current_zoom > MIN_ZOOM) {
        current_zoom -= ZOOM_STEP;
        if (current_zoom < MIN_ZOOM) current_zoom = MIN_ZOOM;
        update_zoom(text_area);
        update_statusbar(window);
    }
}

/* Réinitialiser zoom */
static void on_zoom_reset(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    current_zoom = 50;
    update_zoom(text_area);
    update_statusbar(window);
}

/* Insérer un saut de page visuel */
static void on_insert_page_break(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, "\n\n---------------- PAGE BREAK ----------------\n\n", -1);
    update_statusbar(window);
}

/* Afficher/masquer marges (toggle) */
static void on_toggle_margins(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    /* On lit la marge gauche actuelle pour décider */
    gint left = gtk_text_view_get_left_margin(GTK_TEXT_VIEW(text_area));
    if (left == 0) {
        gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_area), 50);
        gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_area), 50);
        gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text_area), 50);
        gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text_area), 50);
        gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Marges affichées");
    } else {
        gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_area), 0);
        gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_area), 0);
        gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text_area), 0);
        gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text_area), 0);
        gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Marges masquées");
    }
    update_statusbar(window);
}

/* ---------------------------
   Aide: ouvrir doc / raccourcis
   --------------------------- */

static void on_open_doc(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    /* Exemple : ouvrir une URL de documentation dans le navigateur par défaut */
    const gchar *doc_url = "https://example.com/intellieditor-doc";
    GError *error = NULL;
    gtk_show_uri_on_window(GTK_WINDOW(window), doc_url, GDK_CURRENT_TIME, &error);
    if (error) {
        g_printerr("Impossible d'ouvrir la documentation: %s\n", error->message);
        g_error_free(error);
    } else {
        gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Ouverture de la documentation...");
    }
}

static void on_show_shortcuts(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "Raccourcis clavier disponibles:\n\n"
                                               "Ctrl + : Zoom avant\n"
                                               "Ctrl - : Zoom arrière\n"
                                               "Ctrl 0 : Zoom par défaut\n"
                                               "Ctrl S : Enregistrer\n"
                                               "Ctrl O : Ouvrir\n"
                                               "Ctrl N : Nouveau\n");
    gtk_window_set_title(GTK_WINDOW(dialog), "Raccourcis clavier");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* ---------------------------
   Callbacks divers
   --------------------------- */

static void on_text_changed(GtkTextBuffer *buffer, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    update_statusbar(window);
}

static void on_toggle_theme(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);

    gboolean dark_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window), "dark_mode"));
    dark_mode = !dark_mode;
    g_object_set_data(G_OBJECT(window), "dark_mode", GINT_TO_POINTER(dark_mode));

    apply_theme(window, dark_mode);
    update_statusbar(window);
}

static void on_about(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);

    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "IntelliEditor");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), "Editeur de texte (GTK) avec panneau de r2gles.");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), (const gchar *[]) { "CalebKindji", NULL });
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}


/* ---------------------------
   Fenêtre principale
   --------------------------- */

GtkWidget* create_main_window(void) {
    GtkWidget *window, *vbox, *menubar;
    GtkWidget *toolbar, *statusbar;
    GtkWidget *hbox, *text_area, *rules_panel;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "IntelliEditor");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 700);
    g_signal_connect(window, "destroy", G_CALLBACK(on_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    /* Menus */
    menubar = gtk_menu_bar_new();
    GtkWidget *fileMi = gtk_menu_item_new_with_label("Fichier");
    GtkWidget *editMi = gtk_menu_item_new_with_label("Édition");
    GtkWidget *viewMi = gtk_menu_item_new_with_label("Affichage");
    GtkWidget *toolsMi = gtk_menu_item_new_with_label("Outils");
    GtkWidget *rulesMi = gtk_menu_item_new_with_label("Règles");
    GtkWidget *helpMi = gtk_menu_item_new_with_label("Aide");

    GtkWidget *fileMenu = gtk_menu_new();
    GtkWidget *editMenu = gtk_menu_new();
    GtkWidget *viewMenu = gtk_menu_new();
    GtkWidget *toolsMenu = gtk_menu_new();
    GtkWidget *rulesMenu = gtk_menu_new();
    GtkWidget *helpMenu = gtk_menu_new();

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(editMi), editMenu);
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

    /* --- Fichier --- */
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

    /* --- Édition --- */
    GtkWidget *copyMi = gtk_menu_item_new_with_label("Copier");
    GtkWidget *pasteMi = gtk_menu_item_new_with_label("Coller");
    GtkWidget *cutMi = gtk_menu_item_new_with_label("Couper");
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), copyMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), pasteMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), cutMi);

    /* --- Affichage --- */
    GtkWidget *zoomInMi = gtk_menu_item_new_with_label("Zoom avant");
    GtkWidget *zoomOutMi = gtk_menu_item_new_with_label("Zoom arrière");
    GtkWidget *zoomResetMi = gtk_menu_item_new_with_label("Zoom par défaut");
    GtkWidget *toggleMarginsMi = gtk_menu_item_new_with_label("Afficher/Masquer marges");
    GtkWidget *insertPageBreakMi = gtk_menu_item_new_with_label("Insérer saut de page");
    GtkWidget *toggleThemeMi = gtk_menu_item_new_with_label("Basculer thème clair/sombre");

    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), zoomInMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), zoomOutMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), zoomResetMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), toggleMarginsMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), insertPageBreakMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), toggleThemeMi);

    /* --- Outils --- */
    GtkWidget *correctMi = gtk_menu_item_new_with_label("Orthographe");
    GtkWidget *reformulateMi = gtk_menu_item_new_with_label("Reformuler");
    GtkWidget *statsMi = gtk_menu_item_new_with_label("Statistiques du document");
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), correctMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), reformulateMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), statsMi);

    /* --- Règles --- */
    GtkWidget *loadRulesMi = gtk_menu_item_new_with_label("Charger règles...");
    GtkWidget *applyRulesMi = gtk_menu_item_new_with_label("Appliquer règles");
    GtkWidget *exportRulesMi = gtk_menu_item_new_with_label("Exporter rapport");
    gtk_menu_shell_append(GTK_MENU_SHELL(rulesMenu), loadRulesMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(rulesMenu), applyRulesMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(rulesMenu), exportRulesMi);

    /* --- Aide --- */
    GtkWidget *aboutMi = gtk_menu_item_new_with_label("À propos");
    GtkWidget *docMi = gtk_menu_item_new_with_label("Documentation");
    GtkWidget *shortcutsMi = gtk_menu_item_new_with_label("Raccourcis clavier");
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), aboutMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), docMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), shortcutsMi);

    /* Pack menubar */
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    /* Toolbar (simple) */
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

    /* Zone principale: texte + panneau règles */
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

    /* Marges par défaut type "Word" */
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_area), 50);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_area), 50);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text_area), 50);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text_area), 50);

    gtk_container_add(GTK_CONTAINER(text_scrolled), text_area);

    /* Panneau règles (fonction create_rules_panel fournie ailleurs) */
    rules_panel = create_rules_panel();
    gtk_widget_set_size_request(rules_panel, 300, 0);

    /* Stocker pour les callbacks */
    g_object_set_data(G_OBJECT(window), "rules_panel", rules_panel);


    gtk_box_pack_start(GTK_BOX(hbox), text_scrolled, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), rules_panel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    /* Statusbar */
    statusbar = gtk_statusbar_new();
    g_object_set_data(G_OBJECT(window), "statusbar", statusbar);
    gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    /* Connexions signaux menu -> callbacks */
    g_signal_connect(newMi, "activate", G_CALLBACK(on_new), window);
    g_signal_connect(openMi, "activate", G_CALLBACK(on_open), window);
    g_signal_connect(saveMi, "activate", G_CALLBACK(on_save), window);
    g_signal_connect(saveRtfMi, "activate", G_CALLBACK(on_save_rtf), window);
    g_signal_connect(quitMi, "activate", G_CALLBACK(on_quit), NULL);

    g_signal_connect(copyMi, "activate", G_CALLBACK(on_copy), window);
    g_signal_connect(pasteMi, "activate", G_CALLBACK(on_paste), window);
    g_signal_connect(cutMi, "activate", G_CALLBACK(on_cut), window);

    g_signal_connect(zoomInMi, "activate", G_CALLBACK(on_zoom_in), window);
    g_signal_connect(zoomOutMi, "activate", G_CALLBACK(on_zoom_out), window);
    g_signal_connect(zoomResetMi, "activate", G_CALLBACK(on_zoom_reset), window);
    g_signal_connect(toggleMarginsMi, "activate", G_CALLBACK(on_toggle_margins), window);
    g_signal_connect(insertPageBreakMi, "activate", G_CALLBACK(on_insert_page_break), window);
    g_signal_connect(toggleThemeMi, "activate", G_CALLBACK(on_toggle_theme), window);


    g_signal_connect(correctMi, "activate", G_CALLBACK(on_correct), window);
    g_signal_connect(reformulateMi, "activate", G_CALLBACK(on_reformulate), window);
    g_signal_connect(statsMi, "activate", G_CALLBACK(update_statusbar), window);

    g_signal_connect(loadRulesMi, "activate", G_CALLBACK(on_load_rules), window);
    g_signal_connect(applyRulesMi, "activate", G_CALLBACK(on_apply_rules), window);
    g_signal_connect(exportRulesMi, "activate", G_CALLBACK(on_export_rules), window);

    g_signal_connect(aboutMi, "activate", G_CALLBACK(on_about), window);
    g_signal_connect(docMi, "activate", G_CALLBACK(on_open_doc), window);
    g_signal_connect(shortcutsMi, "activate", G_CALLBACK(on_show_shortcuts), window);

    /* Toolbar buttons */
    g_signal_connect(newTb, "clicked", G_CALLBACK(on_new), window);
    g_signal_connect(openTb, "clicked", G_CALLBACK(on_open), window);
    g_signal_connect(saveTb, "clicked", G_CALLBACK(on_save), window);
    g_signal_connect(correctTb, "clicked", G_CALLBACK(on_correct), window);
    g_signal_connect(reformTb, "clicked", G_CALLBACK(on_reformulate), window);
    g_signal_connect(loadRulesTb, "clicked", G_CALLBACK(on_load_rules), window);

    /* Signaux texte */
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    g_signal_connect(buffer, "changed", G_CALLBACK(on_text_changed), window);

    /* Stocker références utiles sur la fenêtre */
    g_object_set_data(G_OBJECT(window), "text_area", text_area);
    g_object_set_data(G_OBJECT(window), "statusbar", statusbar);
    g_object_set_data(G_OBJECT(window), "dark_mode", GINT_TO_POINTER(FALSE));

    /* Appliquer zoom initial */
    update_zoom(text_area);

    /* Accélérateurs clavier */
    GtkAccelGroup *accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel);
    /* Ctrl + (Zoom avant) */
    gtk_widget_add_accelerator(zoomInMi, "activate", accel, GDK_KEY_plus, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(zoomOutMi, "activate", accel, GDK_KEY_minus, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(zoomResetMi, "activate", accel, GDK_KEY_0, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(saveMi, "activate", accel, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(openMi, "activate", accel, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(newMi, "activate", accel, GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    /* Afficher tout */
    gtk_widget_show_all(window);
    update_statusbar(window);
    return window;
}
