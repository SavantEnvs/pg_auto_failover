/* pg_controldata output parsing (regex-driven field extraction) from src/bin/common/parsing.c. */
#include <stdbool.h>

#include "harness_common.h"
#include "parsing.h"
#include "pgctl.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	mayhem_quiet();

	char *text = mayhem_cstring(data, size);

	if (text == NULL)
	{
		return -1;
	}

	PostgresControlData controlData = { 0 };

	(void) parse_controldata(&controlData, text);

	free(text);
	return 0;
}
