#include <gtk/gtk.h>
#include "ui.h"
#include "rules.h"

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = create_main_window();
    gtk_widget_show(window);

    gtk_main();
    return 0;
}
