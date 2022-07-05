#include <glib.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lock-file.h"

struct lock_file_s {
    FILE *file;
    GMutex lock;
    atomic_size_t line_count;
};

lock_file_t *lock_file_open(const char *filename, const char *mode) {
    lock_file_t *lf = NULL;
    FILE *file = NULL;

    if ((file = fopen(filename, mode)) == NULL) {
        return NULL;
    }

    if ((lf = calloc(1, sizeof(struct lock_file_s))) == NULL) {
        fclose(file);
        return NULL;
    }

    lf->file = file;
    g_mutex_init(&lf->lock);

    return lf;
}

size_t lock_file_get_count(const lock_file_t *lf) {
    return (size_t)__c11_atomic_load(&lf->line_count, memory_order_relaxed);
}

static ssize_t _lock_file_readline(lock_file_t *lf, char **buffer, size_t *buflen) {
    g_mutex_lock(&lf->lock);
    ssize_t n = getline(buffer, buflen, lf->file);
    __c11_atomic_fetch_add(&lf->line_count, 1, memory_order_relaxed);
    lf->line_count++;
    g_mutex_unlock(&lf->lock);
    return n;
}

ssize_t lock_file_readline(lock_file_t *lf, char **buffer, size_t *buflen) {
    ssize_t n;

    do {
        if ((n = _lock_file_readline(lf, buffer, buflen)) < 0) {
            return n;
        }
    } while (n <= 1);

    return n;
}

void lock_file_close(lock_file_t *lf) {
    fclose(lf->file);
    memset(lf, 0, sizeof(struct lock_file_s));
    free(lf);
}
