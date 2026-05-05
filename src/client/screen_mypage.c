#include "screen_mypage.h"

GtkWidget *screen_mypage_new(void)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget *label = gtk_label_new("MyPage Screen");
    gtk_box_append(GTK_BOX(box), label);

    return box;
}
