#include <gtk/gtk.h>
#include <string.h>
#include "rules.h"

/* Helper: clear list children */
static void rules_panel_clear(GtkWidget *panel) {
    GtkWidget *list = g_object_get_data(G_OBJECT(panel), "rules_list");
    if (!GTK_IS_LIST_BOX(list)) return;

    GList *children = gtk_container_get_children(GTK_CONTAINER(list));
    for (GList *l = children; l; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);
}

/* Small icon helper (text icons for simplicity) */
static const gchar* icon_for_issue_type(const char *type) {
    if (!type) return "⚪";
    if (g_strcmp0(type, "error") == 0) return "❌";
    if (g_strcmp0(type, "stub") == 0) return "❌";
    if (g_str_has_prefix(type, "warn") || g_str_has_prefix(type, "warning")) return "⚠️";
    return "✅";
}

/* Update UI from RuleReport */
void rules_panel_update_from_report(GtkWidget *panel, const RuleReport *report) {
    if (!GTK_IS_BOX(panel)) return;

    rules_panel_clear(panel);

    /* Séparateur visuel / aération */
    GtkWidget *spacer = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(panel), spacer, FALSE, TRUE, 6);


    GtkWidget *list = g_object_get_data(G_OBJECT(panel), "rules_list");
    GtkWidget *summary = g_object_get_data(G_OBJECT(panel), "rules_summary");
    if (!GTK_IS_LIST_BOX(list)) return;

    if (!report) {
        if (GTK_IS_LABEL(summary)) gtk_label_set_text(GTK_LABEL(summary), "Conformité : aucun rapport");
        return;
    }

    const int ok = report->rules_ok;
    const int total = report->rule_count;
    const int issues = report->issue_count;

    if (GTK_IS_LABEL(summary)) {
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

    for (int i = 0; i < report->issue_count; ++i) {
        const RuleIssue *iss = &report->issues[i];

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_widget_set_margin_bottom(row, 6);
        gtk_widget_set_hexpand(row, TRUE);

        /* Classe CSS (badge couleur) */
        GtkStyleContext *rctx = gtk_widget_get_style_context(row);
        gtk_style_context_add_class(rctx, "oo-issue-row");

        const gchar *type_class = "oo-issue-ok";
        if (g_strcmp0(iss->type, "error") == 0 || g_strcmp0(iss->type, "stub") == 0) {
            type_class = "oo-issue-error";
        } else if (g_str_has_prefix(iss->type, "warn") || g_str_has_prefix(iss->type, "warning")) {
            type_class = "oo-issue-warn";
        }
        gtk_style_context_add_class(rctx, type_class);

        const gchar *icon = icon_for_issue_type(iss->type);
        GtkWidget *icon_w = gtk_label_new(icon);
        gtk_widget_set_halign(icon_w, GTK_ALIGN_START);
        GtkStyleContext *bctx = gtk_widget_get_style_context(icon_w);
        gtk_style_context_add_class(bctx, "oo-badge");

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        GtkWidget *title = gtk_label_new(NULL);
        gchar *title_txt = g_strdup_printf("Ligne %d — %s", iss->line, iss->type);
        gtk_label_set_text(GTK_LABEL(title), title_txt);
        gtk_widget_set_halign(title, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "oo-issue-title");
        g_free(title_txt);

        GtkWidget *msg = gtk_label_new(iss->message);
        gtk_label_set_xalign(GTK_LABEL(msg), 0.0);
        gtk_widget_set_halign(msg, GTK_ALIGN_START);
        gtk_style_context_add_class(gtk_widget_get_style_context(msg), "oo-issue-msg");

        gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), msg, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(row), icon_w, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), vbox, TRUE, TRUE, 0);


        /* Optional: attach click handler to highlight line in editor (if implemented) */
        gtk_list_box_insert(GTK_LIST_BOX(list), row, -1);
    }

    gtk_widget_show_all(list);
}
 
/* Build the panel UI */
GtkWidget* create_rules_panel(void) {
    GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    /* Par défaut, le panneau ne s'affiche pas. */
    gtk_widget_hide(panel);
    gtk_container_set_border_width(GTK_CONTAINER(panel), 8);

    /* OnlyOffice-like sidebar width */
    gtk_widget_set_size_request(panel, 280, -1);

    /* ID for CSS */
    gtk_widget_set_name(panel, "rules-panel");

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(panel), header, FALSE, FALSE, 0);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span class='oo-title'>Panneau de conformité</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);

    GtkWidget *subtitle = gtk_label_new("Résultats de vérification");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(subtitle, 6);
    gtk_box_pack_start(GTK_BOX(header), subtitle, FALSE, FALSE, 0);

    GtkStyleContext *hctx = gtk_widget_get_style_context(header);
    gtk_style_context_add_class(hctx, "oo-panel-header");

    GtkWidget *list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_NONE);
    gtk_widget_set_vexpand(list, TRUE);
    gtk_box_pack_start(GTK_BOX(panel), list, TRUE, TRUE, 0);

    GtkWidget *summary = gtk_label_new("Conformité : ...");
    gtk_widget_set_halign(summary, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(panel), summary, FALSE, FALSE, 0);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *apply_btn = gtk_button_new_with_label("Appliquer");
    GtkWidget *export_btn = gtk_button_new_with_label("Exporter");
    gtk_box_pack_end(GTK_BOX(btn_box), export_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(btn_box), apply_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel), btn_box, FALSE, FALSE, 0);

    /* Store references */
    g_object_set_data(G_OBJECT(panel), "rules_list", list);
    g_object_set_data(G_OBJECT(panel), "rules_summary", summary);
    g_object_set_data(G_OBJECT(panel), "apply_button", apply_btn);
    g_object_set_data(G_OBJECT(panel), "export_button", export_btn);

    GtkStyleContext *lctx = gtk_widget_get_style_context(list);
    gtk_style_context_add_class(lctx, "oo-panel-section");

    return panel;
}

