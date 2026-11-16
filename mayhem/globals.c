/* Definitions of the globals that src/bin/common expects pg_autoctl's main.c to provide. */
#include <stddef.h>

#include "postgres_fe.h"

#include "lock_utils.h"

char pg_autoctl_argv0[MAXPGPATH];
char pg_autoctl_program[MAXPGPATH];
int pgconnect_timeout = 2;

char *ps_buffer;
size_t ps_buffer_size;
size_t last_status_len;

Semaphore log_semaphore = { 0 };
