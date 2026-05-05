#include "config.h"
#include "router.h"
#include "client_handler.h"
#include "globals.h"
#include "user_store.h"
#include "../common/net_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <process.h>
    #define THREAD_CREATE(fn, arg) _beginthread(fn, 0, arg)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <pthread.h>
    #define THREAD_CREATE(fn, arg) pthread_create(&tid, NULL, fn, arg)
#endif

static volatile int g_running = 1;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

int main(void)
{
    signal(SIGINT, signal_handler);

    if (sock_init() != 0) {
        fprintf(stderr, "sock_init failed\n");
        return EXIT_FAILURE;
    }

    globals_init();
    user_store_init();  /* Initialize test users */

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        fprintf(stderr, "socket failed\n");
        sock_cleanup();
        return EXIT_FAILURE;
    }

    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "bind failed\n");
        sock_close(listen_fd);
        sock_cleanup();
        return EXIT_FAILURE;
    }

    if (listen(listen_fd, LISTEN_BACKLOG) < 0) {
        fprintf(stderr, "listen failed\n");
        sock_close(listen_fd);
        sock_cleanup();
        return EXIT_FAILURE;
    }

    printf("Chat Server v2.0.0 listening on port %d\n", DEFAULT_PORT);
    router_init();

    while (g_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (g_running) {
                fprintf(stderr, "accept failed\n");
            }
            continue;
        }

        ClientThreadArg *arg = malloc(sizeof(ClientThreadArg));
        if (!arg) {
            sock_close(client_fd);
            continue;
        }

        arg->socket_fd = client_fd;

#ifdef _WIN32
        if (_beginthread(client_thread, 0, arg) == -1) {
            fprintf(stderr, "_beginthread failed\n");
            free(arg);
            sock_close(client_fd);
            continue;
        }
#else
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, arg) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            free(arg);
            sock_close(client_fd);
            continue;
        }
        pthread_detach(tid);
#endif
    }

    sock_close(listen_fd);
    sock_cleanup();

    printf("Chat Server stopped\n");
    return EXIT_SUCCESS;
}
