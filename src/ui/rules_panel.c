#include <gtk/gtk.h>
#include "rules.h"

// Fonction qui construit le panneau des règles
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

    struct {
        const gchar *icon;
        const gchar *text;
    } rules[] = {
        {"✅", "R001 — Introduction présente"},
        {"✅", "R002 — Pas de 1ère personne"},
        {"⚠️", "R003 — Titre H1 non majuscule"},
        {"❌", "R004 — Conclusion manquante"}
    };

    for (size_t i = 0; i < G_N_ELEMENTS(rules); ++i) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_margin_bottom(row, 4);
        GtkWidget *icon = gtk_label_new(rules[i].icon);
        GtkWidget *label = gtk_label_new(rules[i].text);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_box_pack_start(GTK_BOX(row), icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), label, TRUE, TRUE, 0);
        gtk_list_box_insert(GTK_LIST_BOX(list), row, -1);
    }

    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(panel), separator, FALSE, TRUE, 8);

    GtkWidget *summary = gtk_label_new("Conformité : 2/4 règles OK");
    gtk_widget_set_halign(summary, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(panel), summary, FALSE, FALSE, 0);

    return panel;
}
