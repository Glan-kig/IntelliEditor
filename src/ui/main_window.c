#include <stdlib.h>
#include <gtk/gtk.h>

// Callback pour quitter l'application
static void on_quit(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

// Callback pour "Nouveau"
static void on_new(GtkWidget *widget, gpointer data) {
    g_print("Nouveau document créé\n");
}

// Callback pour "Ouvrir"
static void on_open(GtkWidget *widget, gpointer data) {
    g_print("Ouverture d'un fichier...\n");
}

// Callback pour "Sauvegarder"
static void on_save(GtkWidget *widget, gpointer data) {
    g_print("Sauvegarde du fichier...\n");
}

// Mise à jour de la barre de statut
void update_status(GtkStatusbar *statusbar, int line, int col, int words) {
    gchar *msg = g_strdup_printf("Ln %d, Col %d | %d mots | UTF-8 | Thème : Clair", line, col, words);
    gtk_statusbar_pop(statusbar, 0);
    gtk_statusbar_push(statusbar, 0, msg);
    g_free(msg);
}

// Thème clair
static void set_light_theme(GtkWidget *widget, gpointer data) {
    GtkSettings *settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-application-prefer-dark-theme", FALSE, NULL);
    g_print("Thème clair activé\n");
}

// Thème sombre
static void set_dark_theme(GtkWidget *widget, gpointer data) {
    GtkSettings *settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
    g_print("Thème sombre activé\n");
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *menubar, *fileMenu, *fileMi;
    GtkWidget *newMi, *openMi, *saveMi, *quitMi;
    GtkWidget *viewMenu, *viewMi, *themeLight, *themeDark;
    GtkWidget *toolbar;
    GtkToolItem *newTb, *openTb, *saveTb;
    GtkWidget *statusbar;
    GtkWidget *hbox, *rules_panel, *label, *text_area;

    gtk_init(&argc, &argv);

    // Fenêtre principale
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "IntelliEditor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(on_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Menu principal
    menubar = gtk_menu_bar_new();
    fileMenu = gtk_menu_new();
    viewMenu = gtk_menu_new();

    fileMi = gtk_menu_item_new_with_label("Fichier");
    viewMi = gtk_menu_item_new_with_label("Affichage");

    newMi = gtk_menu_item_new_with_label("Nouveau");
    openMi = gtk_menu_item_new_with_label("Ouvrir");
    saveMi = gtk_menu_item_new_with_label("Sauvegarder");
    quitMi = gtk_menu_item_new_with_label("Quitter");

    themeLight = gtk_menu_item_new_with_label("Thème clair");
    themeDark = gtk_menu_item_new_with_label("Thème sombre");

    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), newMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), themeLight);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), themeDark);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(viewMi), viewMenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), viewMi);

    g_signal_connect(newMi, "activate", G_CALLBACK(on_new), NULL);
    g_signal_connect(openMi, "activate", G_CALLBACK(on_open), NULL);
    g_signal_connect(saveMi, "activate", G_CALLBACK(on_save), NULL);
    g_signal_connect(quitMi, "activate", G_CALLBACK(on_quit), NULL);

    g_signal_connect(themeLight, "activate", G_CALLBACK(set_light_theme), NULL);
    g_signal_connect(themeDark, "activate", G_CALLBACK(set_dark_theme), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // Barre d’outils
    toolbar = gtk_toolbar_new();
    newTb = gtk_tool_button_new(NULL, "Nouveau");
    openTb = gtk_tool_button_new(NULL, "Ouvrir");
    saveTb = gtk_tool_button_new(NULL, "Sauvegarder");

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), newTb, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), openTb, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), saveTb, -1);

    g_signal_connect(newTb, "clicked", G_CALLBACK(on_new), NULL);
    g_signal_connect(openTb, "clicked", G_CALLBACK(on_open), NULL);
    g_signal_connect(saveTb, "clicked", G_CALLBACK(on_save), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    // Zone centrale avec panneau des règles
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    // Zone d’édition
    text_area = gtk_text_view_new();
    gtk_box_pack_start(GTK_BOX(hbox), text_area, TRUE, TRUE, 0);

    // Panneau des règles
    rules_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    label = gtk_label_new("Panneau des Règles");
    gtk_box_pack_start(GTK_BOX(rules_panel), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), rules_panel, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    // Barre de statut
    statusbar = gtk_statusbar_new();
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, "Prêt - UTF-8 - FR");
    gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
