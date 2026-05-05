#ifndef SCREEN_MAIN_H
#define SCREEN_MAIN_H

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <gtk/gtk.h>

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

GtkWidget *screen_main_new(void);

#endif // SCREEN_MAIN_H
