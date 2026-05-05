#include "screen_chat.h"

GtkWidget *screen_chat_new(void)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget *label = gtk_label_new("Chat Screen");
    gtk_box_append(GTK_BOX(box), label);

    return box;
}
