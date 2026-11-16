/* Timeline history file parsing (parseTimeLineHistory) from src/bin/common/pgsql.c. */
#include <stdbool.h>

#include "harness_common.h"
#include "pgsql.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	mayhem_quiet();

	char *content = mayhem_cstring(data, size);

	if (content == NULL)
	{
		return -1;
	}

	IdentifySystem system = { 0 };

	system.timeline = 1;
	(void) parseTimeLineHistory("00000002.history", content, &system);
	free(system.timelines.history);

	free(content);
	return 0;
}
