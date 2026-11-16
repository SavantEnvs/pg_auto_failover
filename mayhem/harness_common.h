#ifndef MAYHEM_HARNESS_COMMON_H
#define MAYHEM_HARNESS_COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

/* copy the fuzz input into a NUL-terminated heap string (exact-size, so ASan sees overruns) */
static char *
mayhem_cstring(const uint8_t *data, size_t size)
{
	char *copy = (char *) malloc(size + 1);

	if (copy == NULL)
	{
		return NULL;
	}
	if (size > 0)
	{
		memcpy(copy, data, size);
	}
	copy[size] = '\0';
	return copy;
}

static void
mayhem_quiet(void)
{
	log_set_quiet(true);
}

#endif
