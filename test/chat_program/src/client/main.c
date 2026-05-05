#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <gtk/gtk.h>

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

#include "screen_login.h"
#include "screen_main.h"
#include "screen_chat.h"
#include "screen_mypage.h"
#include <stdio.h>
#include <stdlib.h>

static void on_app_activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Client v2.0.0");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *stack = gtk_stack_new();
    gtk_window_set_child(GTK_WINDOW(window), stack);

    GtkWidget *login_screen = screen_login_new();
    GtkWidget *main_screen = screen_main_new();
    GtkWidget *chat_screen = screen_chat_new();
    GtkWidget *mypage_screen = screen_mypage_new();

    gtk_stack_add_titled(GTK_STACK(stack), login_screen, "login", "Login");
    gtk_stack_add_titled(GTK_STACK(stack), main_screen, "main", "Main");
    gtk_stack_add_titled(GTK_STACK(stack), chat_screen, "chat", "Chat");
    gtk_stack_add_titled(GTK_STACK(stack), mypage_screen, "mypage", "MyPage");

    gtk_stack_set_visible_child_name(GTK_STACK(stack), "login");

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[])
{
    GtkApplication *app = gtk_application_new("org.example.chat", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
