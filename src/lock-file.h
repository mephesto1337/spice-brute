#ifndef __LOCK_FILE_H__DEFINED__
#define __LOCK_FILE_H__DEFINED__

#include <pthread.h>
#include <stdio.h>

typedef struct lock_file_s lock_file_t;

lock_file_t *lock_file_open(const char *filename, const char *mode);
ssize_t lock_file_readline(lock_file_t *f, char **buffer, size_t *buflen);
size_t lock_file_get_count(const lock_file_t *lf);
void lock_file_close(lock_file_t *f);

#endif // __LOCK_FILE_H__DEFINED__
