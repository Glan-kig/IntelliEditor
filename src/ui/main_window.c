/* main_window.c
 *
 * Fenêtre principale d'IntelliEditor avec barre de menus, toolbar enrichie
 * (police, taille, gras/italique/souligné, alignement, styles), zone de texte,
 * panneau règles et statusbar.
 *
 * Dépendances externes (fichiers fournis ailleurs) :
 *  - rules.h / rules.c (load_rules, apply_rules_to_buffer, free_rule_report, create_rules_panel, rules_panel_update_from_report)
 *
 * Compile : `gcc -o intellieditor main_window.c rules.c `pkg-config --cflags --libs gtk+-3.0` -lm`
 */

#include <gtk/gtk.h>
#include <gtk/gtkaboutdialog.h>
#include <math.h>
#include "rules.h"


/* Prototypes externes fournis ailleurs */
GtkWidget* create_rules_panel(void);
void rules_panel_update_from_report(GtkWidget *panel, const RuleReport *report);

/* ---------------------------
   Configuration zoom / marges
   --------------------------- */
static int current_zoom = 50;
static const int MIN_ZOOM = 10;
static const int MAX_ZOOM = 200;
static const int ZOOM_STEP = 5;

/* Convertit un pourcentage de zoom en taille approximative en pixels */
static int zoom_to_font_size(int zoom_percent) {
    /* 100% => 12px (approx), on scale linéairement */
    int size = (zoom_percent * 12) / 100;
    if (size < 6) size = 6;
    if (size > 72) size = 72;
    return size;
}

/* ---------------------------
   Helpers: theme, zoom, status
   --------------------------- */

static void apply_theme(GtkWidget *window, gboolean dark_mode) {
    (void)window;

    GtkCssProvider *provider = gtk_css_provider_new();

    /* Style proche OnlyOffice (light). On ignore dark_mode pour l’instant : charte claire uniquement. */
    if (dark_mode) {
        /* volontairement light-only */
        dark_mode = FALSE;
    }

    gtk_css_provider_load_from_data(provider,
        /* Base */
        "window { background-color: #f3f5f7; }"
        "textview { background-color: #ffffff; color: #1f2328; border-radius: 8px; }"
        "label { color: #1f2328; }"

        /* Title / headers */
        ".oo-title { font-weight: 700; color: #1f2328; font-size: 14px; }"
        ".oo-subtitle { color: #6b7280; font-size: 12px; }"

        /* Ribbon / toolbar */
        "toolbar { background-color: #ffffff; border-bottom: 1px solid #e5e7eb; padding: 6px; }"
        "toolbar toolbutton { padding: 4px 6px; border-radius: 6px; }"

        /* Buttons */
        "button, toolbutton { background-image: none; }"
        "button { background-color: #f9fafb; border: 1px solid #d1d5db; border-radius: 8px; padding: 6px 10px; }"
        "button:hover { background-color: #eef2ff; border-color: #c7d2fe; }"

        /* Sidebar / panel */
        "#rules-panel { background-color: #ffffff; border: 1px solid #e5e7eb; border-radius: 12px; }"
        "#rules-panel .oo-panel-header { background: #f8fafc; border-bottom: 1px solid #e5e7eb; padding: 10px 12px; border-top-left-radius: 12px; border-top-right-radius: 12px; }"
        "#rules-panel .oo-panel-section { padding: 10px 12px; }"

        /* Issue rows */
        "#rules-panel .oo-issue-row { padding: 8px 10px; border-radius: 10px; margin: 0px; border: 1px solid transparent; }"
        "#rules-panel .oo-issue-row:hover { background-color: #f3f4f6; border-color: #e5e7eb; }"
        "#rules-panel .oo-badge { min-width: 22px; font-weight: 700; }"
        "#rules-panel .oo-issue-title { font-weight: 600; font-size: 12px; color: #111827; }"
        "#rules-panel .oo-issue-msg { font-size: 12px; color: #374151; }"

        /* Badges by type */
        "#rules-panel .oo-issue-error .oo-badge { background-color: #fee2e2; color: #991b1b; border: 1px solid #fecaca; border-radius: 999px; padding: 2px 6px; }"
        "#rules-panel .oo-issue-warn  .oo-badge { background-color: #fef3c7; color: #92400e; border: 1px solid #fde68a; border-radius: 999px; padding: 2px 6px; }"
        "#rules-panel .oo-issue-ok    .oo-badge { background-color: #dcfce7; color: #166534; border: 1px solid #bbf7d0; border-radius: 999px; padding: 2px 6px; }",
        -1, NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    g_object_unref(provider);
}



static void update_statusbar(GtkWidget *window) {
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    if (!GTK_IS_TEXT_VIEW(text_area)) return;

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

    gchar *status_text = g_strdup_printf("Mots : %d | Ligne %d, Col %d | FR | Zoom %d%%",
                                         word_count, line, column, current_zoom);
    GtkWidget *statusbar = g_object_get_data(G_OBJECT(window), "statusbar");
    if (GTK_IS_STATUSBAR(statusbar)) {
        guint context = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "editor-status");
        gtk_statusbar_pop(GTK_STATUSBAR(statusbar), context);
        gtk_statusbar_push(GTK_STATUSBAR(statusbar), context, status_text);
    }
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
    (void)widget;
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

    RuleReport *report = apply_rules_to_buffer(buffer);
    if (report) {
        rules_panel_update_from_report(rules_panel, report);
        free_rule_report(report);
    }

    update_statusbar(window);
}

/* Callbacks pour copier/coller/couper */
static void on_copy(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    gtk_text_buffer_copy_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
}

static void on_paste(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    gtk_text_buffer_paste_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), NULL, TRUE);
}

static void on_cut(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    gtk_text_buffer_cut_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), TRUE);
}

/* ---------------------------
   Règles: appliquer / exporter
   --------------------------- */

static void on_apply_rules(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));

    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Application des règles...");
    update_statusbar(window);
}

/* Exporter un rapport simple des règles (fichier texte) */
static void on_export_rules(GtkWidget *widget, gpointer data) {
    (void)widget;
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

static void set_default_paragraph_format(GtkTextView *text_view) {
    /* Interligne : 1.15 (lisibilité) */
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);

    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(table, "normal_base");
    if (!tag) {
        tag = gtk_text_tag_new("normal_base");
        gtk_text_tag_table_add(table, tag);
    }

    /* Pango: pixels/taille via size, interligne via "pixels-above-lines"/"pixels-below-lines" ou "line-height" selon version.
       Ici on utilise line-height si supporté ; sinon, on ne casse pas la compilation. */
#ifdef PANGO_VERSION
    /* tentative : line-height (Pango>=1.50) */
    g_object_set(tag, "size", -1, NULL);
#endif
    /* En pratique GTK/Pango expose "pixels-above-lines" et "pixels-below-lines".
       Pour obtenir ~1.15, on ajoute un surplus de 15% sur la hauteur courante (approximatif). */
    int surplus = 2; /* valeur conservatrice */
    g_object_set(tag, "pixels_above_lines", 0, "pixels_below_lines", surplus, NULL);

    gtk_text_buffer_remove_tag(buffer, tag, &start, &end);
    gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
}

static void update_zoom(GtkWidget *text_area) {

    if (!GTK_IS_TEXT_VIEW(text_area)) return;
    int px = zoom_to_font_size(current_zoom);
    double points = (double)px * 0.75;
    gchar *font_str = g_strdup_printf("Sans %d", (int)round(points));
    PangoFontDescription *font = pango_font_description_from_string(font_str);

    /* gtk_widget_override_font() est obsolète sur GTK3.
       On applique donc la police à tout le contenu via des tags. */
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);

    gchar *tag_name = g_strdup_printf("zoom_%d", current_zoom);
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);
    if (!tag) {
        tag = gtk_text_tag_new(tag_name);
        gtk_text_tag_table_add(table, tag);
    }

    /* Convertir la description en attributs de tag */
    /* family */
    const gchar *family = pango_font_description_get_family(font);
    if (family) g_object_set(tag, "family", family, NULL);

    /* size: Pango en unit(s) de PANGO_SCALE */
    int size = pango_font_description_get_size(font);
    if (size > 0) g_object_set(tag, "size", size, NULL);

    gtk_text_buffer_remove_tag(buffer, tag, &start, &end);
    gtk_text_buffer_apply_tag(buffer, tag, &start, &end);

    pango_font_description_free(font);
    g_free(font_str);
    g_free(tag_name);
}


static void on_zoom_in(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    if (current_zoom < MAX_ZOOM) {
        current_zoom += ZOOM_STEP;
        if (current_zoom > MAX_ZOOM) current_zoom = MAX_ZOOM;
        update_zoom(text_area);
        update_statusbar(window);
    }
}

static void on_zoom_out(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    if (current_zoom > MIN_ZOOM) {
        current_zoom -= ZOOM_STEP;
        if (current_zoom < MIN_ZOOM) current_zoom = MIN_ZOOM;
        update_zoom(text_area);
        update_statusbar(window);
    }
}

static void on_zoom_reset(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    current_zoom = 50;
    update_zoom(text_area);
    update_statusbar(window);
}

static void on_insert_page_break(GtkWidget *widget, gpointer data) {
    /* "Saut de page" : on insère un marqueur visuel séparé.
       (Dans cette version, c'est du texte séparateur ; l'idée est de créer une nouvelle page.) */
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, "\n\n---------------- PAGE BREAK ----------------\n\n", -1);
    update_statusbar(window);
}

static void show_rules_panel_if_hidden(GtkWidget *window) {
    GtkWidget *rules_panel = g_object_get_data(G_OBJECT(window), "rules_panel");
    if (rules_panel && !gtk_widget_get_visible(rules_panel)) {
        gtk_widget_set_visible(rules_panel, TRUE);
    }
}

static void on_toggle_margins(GtkWidget *widget, gpointer data) {
    /* "Angle": dès que l'utilisateur agit sur la mise en page (marges), on affiche le panneau de conformité. */
    show_rules_panel_if_hidden(GTK_WIDGET(data));
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *text_area = g_object_get_data(G_OBJECT(window), "text_area");
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
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
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
    (void)widget;
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

/* ---------------------------
   Mise en forme: police, taille, styles, alignement
   --------------------------- */

static void apply_font_to_selection(GtkTextView *text_view, const gchar *family, int size_pt) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) return;

    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    gchar tag_name[128];
    g_snprintf(tag_name, sizeof(tag_name), "font_%s_%d", family ? family : "Sans", size_pt);

    GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);
    if (!tag) {
        tag = gtk_text_tag_new(tag_name);
        if (family) g_object_set(tag, "family", family, NULL);
        if (size_pt > 0) g_object_set(tag, "size", (int)(size_pt * PANGO_SCALE), NULL);
        gtk_text_tag_table_add(table, tag);
    }
    gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
}

static void on_font_set(GtkFontButton *font_button, gpointer data) {
    GtkTextView *text_view = GTK_TEXT_VIEW(data);

    /* GTK builds may not provide gtk_font_button_get_font_desc()/set_font_desc().
       Use font name instead and parse it into a PangoFontDescription. */
    const gchar *font_name = gtk_font_button_get_font_name(font_button);
    /* gtk_font_button_get_font_name() peut être dépréciée selon la version GTK.
       On la conserve pour GTK3 afin de supporter l’API sans dépendre de gtk_font_button_get_font_desc(). */

    if (!font_name || !*font_name) return;

    PangoFontDescription *desc = pango_font_description_from_string(font_name);
    if (!desc) return;

    const gchar *family = pango_font_description_get_family(desc);
    int size = pango_font_description_get_size(desc);

    int size_pt = 0;
    if (size > 0) size_pt = size / PANGO_SCALE;

    apply_font_to_selection(text_view, family, size_pt);
    pango_font_description_free(desc);
}






static void on_size_changed(GtkSpinButton *spin, gpointer data) {
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    int size_pt = gtk_spin_button_get_value_as_int(spin);
    apply_font_to_selection(text_view, NULL, size_pt);
}

static void ensure_tag(GtkTextBuffer *buffer, const gchar *name, GParamSpec *unused, GObject *properties, ...) {
    (void)unused;
    (void)properties;
    (void)name;
    (void)buffer;
    (void)unused;
}

/* Gras / Italique / Souligné appliqués sur sélection (déjà définis plus haut) */
static void on_toggle_bold_cb(GtkToggleToolButton *button, gpointer data) {
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) return;

    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(table, "bold");
    if (!tag) {
        tag = gtk_text_tag_new("bold");
        g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);
        gtk_text_tag_table_add(table, tag);
    }
    gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
}

static void on_toggle_italic_cb(GtkToggleToolButton *button, gpointer data) {
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) return;

    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(table, "italic");
    if (!tag) {
        tag = gtk_text_tag_new("italic");
        g_object_set(tag, "style", PANGO_STYLE_ITALIC, NULL);
        gtk_text_tag_table_add(table, tag);
    }
    gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
}

static void on_toggle_underline_cb(GtkToggleToolButton *button, gpointer data) {
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) return;

    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(table, "underline");
    if (!tag) {
        tag = gtk_text_tag_new("underline");
        g_object_set(tag, "underline", PANGO_UNDERLINE_SINGLE, NULL);
        gtk_text_tag_table_add(table, tag);
    }
    gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
}

/* Alignement de paragraphe */
static void apply_alignment_to_paragraphs(GtkTextView *text_view, GtkJustification justification) {

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        /* Appliquer tag de justification sur chaque paragraphe couvert */
        GtkTextIter iter = start;
        do {
            GtkTextIter para_start = iter;
            gtk_text_iter_set_line_offset(&para_start, 0);
            GtkTextIter para_end = para_start;
            if (!gtk_text_iter_ends_line(&para_end)) {
                gtk_text_iter_forward_to_line_end(&para_end);
            }
            GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
            GtkTextTag *tag = gtk_text_tag_table_lookup(table, "just");
            if (!tag) {
                tag = gtk_text_tag_new("just");
                g_object_set(tag, "justification", justification, NULL);
                gtk_text_tag_table_add(table, tag);
            } else {
                /* mettre à jour justification si nécessaire */
                g_object_set(tag, "justification", justification, NULL);
            }
            gtk_text_buffer_apply_tag(buffer, tag, &para_start, &para_end);
            iter = para_end;
            if (!gtk_text_iter_forward_line(&iter)) break;
        } while (gtk_text_iter_compare(&iter, &end) <= 0);
    } else {
        /* Si pas de sélection, appliquer au paragraphe courant */
        GtkTextIter cursor;
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
        GtkTextIter para_start = cursor;
        gtk_text_iter_set_line_offset(&para_start, 0);
        GtkTextIter para_end = para_start;
        if (!gtk_text_iter_ends_line(&para_end)) gtk_text_iter_forward_to_line_end(&para_end);
        GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
        GtkTextTag *tag = gtk_text_tag_table_lookup(table, "just");
        if (!tag) {
            tag = gtk_text_tag_new("just");
            g_object_set(tag, "justification", justification, NULL);
            gtk_text_tag_table_add(table, tag);
        } else {
            g_object_set(tag, "justification", justification, NULL);
        }
        gtk_text_buffer_apply_tag(buffer, tag, &para_start, &para_end);
    }
}

static void on_align_left(GtkToolButton *button, gpointer data) {
    (void)button;
    apply_alignment_to_paragraphs(GTK_TEXT_VIEW(data), GTK_JUSTIFY_LEFT);
}

static void on_align_center(GtkToolButton *button, gpointer data) {
    (void)button;
    apply_alignment_to_paragraphs(GTK_TEXT_VIEW(data), GTK_JUSTIFY_CENTER);
}

static void on_align_right(GtkToolButton *button, gpointer data) {
    (void)button;
    apply_alignment_to_paragraphs(GTK_TEXT_VIEW(data), GTK_JUSTIFY_RIGHT);
}

    /* Interligne / styles prédéfinis (Normal, Titre 1, Titre 2)
       Note : GTK utilise des tags Pango pour appliquer interligne, polices, etc. */

static void apply_style(GtkTextView *text_view, const gchar *style_name) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) return;

    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    if (g_strcmp0(style_name, "Normal") == 0) {
        /* Retirer tags de titre si présents (simple approche) */
        GtkTextTag *t1 = gtk_text_tag_table_lookup(table, "titre1");
        GtkTextTag *t2 = gtk_text_tag_table_lookup(table, "titre2");
        if (t1) gtk_text_buffer_remove_tag(buffer, t1, &start, &end);
        if (t2) gtk_text_buffer_remove_tag(buffer, t2, &start, &end);
    } else if (g_strcmp0(style_name, "Titre 1") == 0) {
        GtkTextTag *t1 = gtk_text_tag_table_lookup(table, "titre1");
        if (!t1) {
            t1 = gtk_text_tag_new("titre1");
            g_object_set(t1, "weight", PANGO_WEIGHT_BOLD, "size", (int)(18 * PANGO_SCALE), NULL);
            gtk_text_tag_table_add(table, t1);
        }
        gtk_text_buffer_apply_tag(buffer, t1, &start, &end);
    } else if (g_strcmp0(style_name, "Titre 2") == 0) {
        GtkTextTag *t2 = gtk_text_tag_table_lookup(table, "titre2");
        if (!t2) {
            t2 = gtk_text_tag_new("titre2");
            g_object_set(t2, "weight", PANGO_WEIGHT_BOLD, "size", (int)(14 * PANGO_SCALE), NULL);
            gtk_text_tag_table_add(table, t2);
        }
        gtk_text_buffer_apply_tag(buffer, t2, &start, &end);
    }
}

/* ---------------------------
   Theme / About / Style combo callbacks
   --------------------------- */

static void on_toggle_theme(GtkWidget *widget, gpointer data);
static void on_about(GtkWidget *widget, gpointer data);
static void on_style_combo_changed(GtkComboBox *combo, gpointer data);

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

    const gchar *authors[] = { "Caleb Kindji", NULL };

    GtkWidget *about = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), "IntelliEditor");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), "0.1");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), "Editeur de texte avec panneau de règles.");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);

    /* GTK3: make it modal/transient relative to main window.
       Some GTK builds may not expose about-dialog modal/transient setters,
       so we use the generic dialog API instead. */
    gtk_window_set_modal(GTK_WINDOW(about), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(window));

    gtk_dialog_run(GTK_DIALOG(about));
    gtk_widget_destroy(about);
}

static void on_style_combo_changed(GtkComboBox *combo, gpointer data) {
    GtkTextView *tv = GTK_TEXT_VIEW(data);
    gchar *style = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (!style) return;

    apply_style(tv, style);
    g_free(style);
}

/* ---------------------------
   Fenêtre principale
   --------------------------- */

GtkWidget* create_main_window(void) {
    GtkWidget *window, *vbox, *menubar;
    GtkWidget *toolbar, *statusbar;
    GtkWidget *hbox, *text_area, *rules_panel;

    gtk_init(NULL, NULL);

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

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), editMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), viewMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), toolsMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), rulesMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), helpMi);

    /* Sous-menus */
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

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    /* Toolbar enrichie */
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);

    /* Boutons de fichier rapides */
    GtkToolItem *newTb = gtk_tool_button_new(NULL, "Nouveau");
    GtkToolItem *openTb = gtk_tool_button_new(NULL, "Ouvrir");
    GtkToolItem *saveTb = gtk_tool_button_new(NULL, "Enregistrer");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), newTb, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), openTb, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), saveTb, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    /* Police (GtkFontButton) */
    GtkWidget *fontBtn = gtk_font_button_new();
    /* Proposer explicitement "Calibre" (si disponible) */
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(fontBtn), "Calibre 11");



    GtkToolItem *fontItem = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(fontItem), fontBtn);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), fontItem, -1);

    /* Taille (spin) */
    GtkWidget *sizeSpin = gtk_spin_button_new_with_range(8, 72, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sizeSpin), 11);
    GtkToolItem *sizeItem = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(sizeItem), sizeSpin);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sizeItem, -1);

    /* Gras / Italique / Souligné */
    GtkToolItem *boldBtn = gtk_toggle_tool_button_new();
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(boldBtn), "B");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), boldBtn, -1);

    GtkToolItem *italicBtn = gtk_toggle_tool_button_new();
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(italicBtn), "I");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), italicBtn, -1);

    GtkToolItem *underlineBtn = gtk_toggle_tool_button_new();
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(underlineBtn), "U");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), underlineBtn, -1);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    /* Alignement */
    GtkToolItem *alignLeft = gtk_tool_button_new(NULL, "G");
    GtkToolItem *alignCenter = gtk_tool_button_new(NULL, "C");
    GtkToolItem *alignRight = gtk_tool_button_new(NULL, "D");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), alignLeft, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), alignCenter, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), alignRight, -1);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    /* Styles (combo) : le corps doit utiliser "Normal" (et non "Titre 1") */
    GtkWidget *styleCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(styleCombo), "Normal");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(styleCombo), "Titre 1");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(styleCombo), "Titre 2");
    gtk_combo_box_set_active(GTK_COMBO_BOX(styleCombo), 0);

    GtkToolItem *styleItem = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(styleItem), styleCombo);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), styleItem, -1);

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

    /* Marges par défaut (rèf. charte : ~2,5 cm) — valeur en pixels pour GTK */
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_area), 60);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_area), 60);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text_area), 60);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text_area), 60);


    gtk_container_add(GTK_CONTAINER(text_scrolled), text_area);

    /* Panneau règles (fonction create_rules_panel fournie ailleurs) */
    rules_panel = create_rules_panel();
    /* Panneau latéral : réduire la largeur pour laisser plus d'espace au texte */
    gtk_widget_set_size_request(rules_panel, 230, 0);


    /* Stocker pour les callbacks */
    g_object_set_data(G_OBJECT(window), "text_area", text_area);
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
    g_signal_connect(quitMi, "activate", G_CALLBACK(on_quit), window);

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
    g_signal_connect(statsMi, "activate", G_CALLBACK(on_show_shortcuts), window);

    g_signal_connect(loadRulesMi, "activate", G_CALLBACK(on_load_rules), window);
    g_signal_connect(applyRulesMi, "activate", G_CALLBACK(on_apply_rules), window);
    g_signal_connect(exportRulesMi, "activate", G_CALLBACK(on_export_rules), window);

    g_signal_connect(aboutMi, "activate", G_CALLBACK(on_about), window);
    g_signal_connect(docMi, "activate", G_CALLBACK(on_open_doc), window);
    g_signal_connect(shortcutsMi, "activate", G_CALLBACK(on_show_shortcuts), window);

    /* Toolbar signals */
    g_signal_connect(newTb, "clicked", G_CALLBACK(on_new), window);
    g_signal_connect(openTb, "clicked", G_CALLBACK(on_open), window);
    g_signal_connect(saveTb, "clicked", G_CALLBACK(on_save), window);

    /* Font and size */
    g_signal_connect(fontBtn, "font-set", G_CALLBACK(on_font_set), text_area);
    g_signal_connect(sizeSpin, "value-changed", G_CALLBACK(on_size_changed), text_area);

    /* Format buttons */
    g_signal_connect(boldBtn, "toggled", G_CALLBACK(on_toggle_bold_cb), text_area);
    g_signal_connect(italicBtn, "toggled", G_CALLBACK(on_toggle_italic_cb), text_area);
    g_signal_connect(underlineBtn, "toggled", G_CALLBACK(on_toggle_underline_cb), text_area);

    /* Align */
    g_signal_connect(alignLeft, "clicked", G_CALLBACK(on_align_left), text_area);
    g_signal_connect(alignCenter, "clicked", G_CALLBACK(on_align_center), text_area);
    g_signal_connect(alignRight, "clicked", G_CALLBACK(on_align_right), text_area);

    /* Styles combo */
    g_signal_connect(styleCombo, "changed", G_CALLBACK(on_style_combo_changed), text_area);


    /* Text buffer signals */
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_area));
    g_signal_connect(buffer, "changed", G_CALLBACK(on_text_changed), window);

    /* Initial zoom application */
    update_zoom(text_area);
    update_statusbar(window);

    gtk_widget_show_all(window);
    return window;
}
