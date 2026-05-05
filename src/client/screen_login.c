#include "screen_login.h"

GtkWidget *screen_login_new(void)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget *label = gtk_label_new("Login Screen");
    gtk_box_append(GTK_BOX(box), label);

    return box;
}

void screen_login_show_error(GtkWidget *screen, const char *error)
{
    (void)screen;
    (void)error;
}
