#ifndef SCREEN_CHAT_H
#define SCREEN_CHAT_H

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <gtk/gtk.h>

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

GtkWidget *screen_chat_new(void);

#endif // SCREEN_CHAT_H
