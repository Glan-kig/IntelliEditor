#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>
#include "rules.h"

/* Nettoyer le panneau avant mise à jour */
static void rules_panel_clear(GtkWidget *panel) {
    GtkWidget *list = g_object_get_data(G_OBJECT(panel), "rules_list");
    GtkWidget *bad_words_list = g_object_get_data(G_OBJECT(panel), "bad_words_list");

    if (list) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(list));
        for (GList *l = children; l; l = l->next) {
            gtk_widget_destroy(GTK_WIDGET(l->data));
        }
        g_list_free(children);
    }

    if (bad_words_list) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(bad_words_list));
        for (GList *l = children; l; l = l->next) {
            gtk_widget_destroy(GTK_WIDGET(l->data));
        }
        g_list_free(children);
    }
}

static gboolean str_contains_ci(const char *haystack, const char *needle) {
    if (!haystack || !needle) return FALSE;
    size_t nlen = strlen(needle);
    if (nlen == 0) return TRUE;

    for (const char *p = haystack; *p; p++) {
        size_t i = 0;
        while (i < nlen) {
            char c1 = (char)tolower((unsigned char)p[i]);
            char c2 = (char)tolower((unsigned char)needle[i]);
            if (c1 != c2) break;
            i++;
        }
        if (i == nlen) return TRUE;
    }
    return FALSE;
}

/* Option 2: extraire uniquement le mot (premier token) depuis message */
static char *extract_first_word(const char *message) {
    if (!message) return NULL;

    /* stop chars: whitespace, ':' , '|' */
    const char *p = message;

    /* skip leading spaces */
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) return NULL;

    const char *start = p;
    while (*p && !isspace((unsigned char)*p) && *p != ':' && *p != '|') p++;

    size_t len = (size_t)(p - start);
    if (len == 0) return NULL;

    char *out = g_strndup(start, len);
    return out;
}

/* Choisir une icône selon le type d’issue */
static const gchar* icon_for_issue_type(const char *type) {
    if (!type) return "⚪";

    /* Icône dédiée pour orthographe / mots incorrects */
    if (g_strrstr(type, "spell") || g_strrstr(type, "ortho") || g_strrstr(type, "incorrect")) {
        return "📝❌";
    }

    if (g_strcmp0(type, "error") == 0) return "❌";
    if (g_strcmp0(type, "stub") == 0) return "❌";
    if (g_str_has_prefix(type, "warn") || g_str_has_prefix(type, "warning")) return "⚠️";
    return "✅";
}

static gboolean issue_is_spelling_like(const RuleIssue *iss) {
    if (!iss) return FALSE;

    /* Détection stricte pour éviter de classer toutes les règles en orthographe */
    if (str_contains_ci(iss->type, "spell") || str_contains_ci(iss->type, "ortho")) return TRUE;

    /* fallback minimal: message explicite d'orthographe */
    if (str_contains_ci(iss->message, "mot incorrect")) return TRUE;
    if (str_contains_ci(iss->message, "faute d'orthographe")) return TRUE;

    return FALSE;
}

/* Mettre à jour le panneau avec un rapport */
void rules_panel_update_from_report(GtkWidget *panel, const RuleReport *report) {
    if (!panel) return;

    rules_panel_clear(panel);

    GtkWidget *list = g_object_get_data(G_OBJECT(panel), "rules_list");
    GtkWidget *summary = g_object_get_data(G_OBJECT(panel), "rules_summary");

    GtkWidget *bad_words_list = g_object_get_data(G_OBJECT(panel), "bad_words_list");
    GtkWidget *bad_words_summary = g_object_get_data(G_OBJECT(panel), "bad_words_summary_label");

    if (!list) return;

    if (!report) {
        if (summary) gtk_label_set_text(GTK_LABEL(summary), "Conformité : aucun rapport");
        if (bad_words_summary) gtk_label_set_text(GTK_LABEL(bad_words_summary), "Mots incorrects : ...");
        return;
    }

    const int ok = report->rules_ok;
    const int total = report->rule_count;
    const int issues = report->issue_count;

    int bad_words_count = 0;

    if (summary) {
        gchar *txt = g_strdup_printf("Conformité : %d/%d règles OK (issues: %d)", ok, total, issues);
        gtk_label_set_text(GTK_LABEL(summary), txt);
        g_free(txt);
    }

    /* Chemin “détaillé” : affichage report->issues */
    for (int i = 0; i < report->issue_count; i++) {
        const RuleIssue *iss = &report->issues[i];

        /* panneau principal */
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_margin_bottom(row, 4);

        const gchar *icon = icon_for_issue_type(iss->type);
        GtkWidget *icon_w = gtk_label_new(icon);

        GtkWidget *label = gtk_label_new("");
        gchar *text = g_strdup_printf("Ligne %d | %s | %s", iss->line, iss->type, iss->message);
        gtk_label_set_text(GTK_LABEL(label), text);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        g_free(text);

        gtk_box_pack_start(GTK_BOX(row), icon_w, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), label, TRUE, TRUE, 0);

        gtk_list_box_insert(GTK_LIST_BOX(list), row, -1);
        gtk_widget_show_all(row);

        /* panneau "mots incorrects" */
        if (bad_words_list && issue_is_spelling_like(iss)) {
            char *w = extract_first_word(iss->message);
            if (w && *w) {
                GtkWidget *w_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
                gtk_widget_set_margin_bottom(w_row, 4);

                GtkWidget *w_icon = gtk_label_new("✗");
                GtkWidget *w_label = gtk_label_new(w);
                gtk_label_set_xalign(GTK_LABEL(w_label), 0.0);

                gtk_box_pack_start(GTK_BOX(w_row), w_icon, FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(w_row), w_label, TRUE, TRUE, 0);

                gtk_list_box_insert(GTK_LIST_BOX(bad_words_list), w_row, -1);
                gtk_widget_show_all(w_row);

                bad_words_count++;
            }
            g_free(w);
        }
    }

    if (bad_words_summary) {
        if (bad_words_count > 0) {
            gchar *sum = g_strdup_printf("Mots incorrects : %d", bad_words_count);
            gtk_label_set_text(GTK_LABEL(bad_words_summary), sum);
            g_free(sum);
        } else {
            gtk_label_set_text(GTK_LABEL(bad_words_summary),
                                "Mots incorrects : aucun (règles orthographe non détectées)");
        }
    }

    /* Toujours compléter avec les règles non conformes du report->rules[]
     * pour afficher aussi regex/section/llm même si report->issues contient
     * surtout des fautes orthographiques. */
    for (int i = 0; i < report->rule_count; i++) {
        const Rule *r = &report->rules[i];

        if (r->status == STATUS_CONFORME) continue;

        /* Éviter le doublon de la règle d'orthographe déjà représentée
         * par les issues détaillées/mots incorrects. */
        if (str_contains_ci(r->check_type, "spell") || str_contains_ci(r->check_type, "ortho")) {
            continue;
        }

        const gchar *mapped_type = "info";
        if (str_contains_ci(r->check_type, "regex")) {
            mapped_type = "warning";
        } else if (str_contains_ci(r->check_type, "section")) {
            mapped_type = "warning";
        } else if (str_contains_ci(r->check_type, "llm")) {
            mapped_type = "error";
        } else {
            mapped_type = (r->severity == SEVERITY_ERROR) ? "error" : "warning";
        }

        const gchar *icon = icon_for_issue_type(mapped_type);

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_margin_bottom(row, 4);

        GtkWidget *icon_w = gtk_label_new(icon);
        GtkWidget *label = gtk_label_new("");

        const char *status_txt =
            (r->status == STATUS_EN_COURS) ? "EN_COURS" :
            (r->status == STATUS_AVERTISSEMENT) ? "AVERTISSEMENT" :
            (r->status == STATUS_NON_CONFORME) ? "NON_CONFORME" : "CONFORME";

        gchar *text = g_strdup_printf("%s | %s | %s (%s)",
                                      r->id, r->check_type, r->description, status_txt);
        gtk_label_set_text(GTK_LABEL(label), text);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        g_free(text);

        gtk_box_pack_start(GTK_BOX(row), icon_w, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), label, TRUE, TRUE, 0);

        gtk_list_box_insert(GTK_LIST_BOX(list), row, -1);
        gtk_widget_show_all(row);
    }
}

/* Créer le panneau de conformité */
GtkWidget* create_rules_panel(void) {
    GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(panel), 10);

    GtkWidget *title = gtk_label_new("Panneau de conformité");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(title, 8);
    gtk_box_pack_start(GTK_BOX(panel), title, FALSE, FALSE, 0);

    GtkWidget *list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_NONE);
    gtk_box_pack_start(GTK_BOX(panel), list, TRUE, TRUE, 0);

    /* Section "mots incorrects" */
    GtkWidget *separator1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(panel), separator1, FALSE, TRUE, 8);

    GtkWidget *bad_words_summary = gtk_label_new("Mots incorrects : ...");
    gtk_widget_set_halign(bad_words_summary, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(panel), bad_words_summary, FALSE, FALSE, 0);

    GtkWidget *bad_words_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(bad_words_list), GTK_SELECTION_NONE);
    gtk_box_pack_start(GTK_BOX(panel), bad_words_list, TRUE, TRUE, 0);

    GtkWidget *separator2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(panel), separator2, FALSE, TRUE, 8);

    GtkWidget *summary = gtk_label_new("Conformité : ...");
    gtk_widget_set_halign(summary, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(panel), summary, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(panel), "rules_list", list);
    g_object_set_data(G_OBJECT(panel), "rules_summary", summary);

    g_object_set_data(G_OBJECT(panel), "bad_words_list", bad_words_list);
    g_object_set_data(G_OBJECT(panel), "bad_words_summary_label", bad_words_summary);

    return panel;
}
