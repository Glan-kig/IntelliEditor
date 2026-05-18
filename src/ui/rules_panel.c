#include <gtk/gtk.h>
#include "rules.h"

/*
  Panneau de règles data-driven.
  - create_rules_panel() construit l’UI de base (titre + list + summary)
  - rules_panel_update_from_report() remplit la liste à partir de RuleReport
*/

static void rules_panel_clear(GtkWidget *panel) {
    GtkWidget *list = g_object_get_data(G_OBJECT(panel), "rules_list");
    if (!list) return;

    GList *children = gtk_container_get_children(GTK_CONTAINER(list));
    for (GList *l = children; l; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);
}

static const gchar* icon_for_issue_type(const char *type) {
    if (!type) return "⚪";
    if (g_strcmp0(type, "error") == 0) return "❌";
    if (g_strcmp0(type, "stub") == 0) return "❌";
    if (g_str_has_prefix(type, "warn") || g_str_has_prefix(type, "warning")) return "⚠️";
    return "✅";
}

void rules_panel_update_from_report(GtkWidget *panel, const RuleReport *report) {
    if (!panel) return;

    rules_panel_clear(panel);

    GtkWidget *list = g_object_get_data(G_OBJECT(panel), "rules_list");
    GtkWidget *summary = g_object_get_data(G_OBJECT(panel), "rules_summary");

    if (!list) return;


    if (!report) {
        if (summary) gtk_label_set_text(GTK_LABEL(summary), "Conformité : aucun rapport");
        return;
    }

    const int ok = report->rules_ok;
    const int total = report->rule_count;
    const int issues = report->issue_count;

    if (summary) {
        if (total > 0) {
            gchar *txt = g_strdup_printf("Conformité : %d/%d règles OK (issues: %d)", ok, total, issues);
            gtk_label_set_text(GTK_LABEL(summary), txt);
            g_free(txt);
        } else {
            gchar *txt = g_strdup_printf("Conformité : issues: %d", issues);
            gtk_label_set_text(GTK_LABEL(summary), txt);
            g_free(txt);
        }
    }

    for (int i = 0; i < report->issue_count; i++) {
        const RuleIssue *iss = &report->issues[i];

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
    }
}

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

    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(panel), separator, FALSE, TRUE, 8);

    GtkWidget *summary = gtk_label_new("Conformité : ...");
    gtk_widget_set_halign(summary, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(panel), summary, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(panel), "rules_list", list);
    g_object_set_data(G_OBJECT(panel), "rules_summary", summary);

    return panel;
}

