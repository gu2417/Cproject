#ifndef SCREEN_LOGIN_H
#define SCREEN_LOGIN_H

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <gtk/gtk.h>

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

GtkWidget *screen_login_new(void);
void screen_login_show_error(GtkWidget *screen, const char *error);

#endif // SCREEN_LOGIN_H
