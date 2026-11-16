/* Dotted "major.minor" version parsing (parse_dotted_version_string and its two wrappers) from src/bin/common/parsing.c. */
#include <stdbool.h>

#include "harness_common.h"
#include "parsing.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 1)
	{
		return -1;
	}

	mayhem_quiet();

	char *str = mayhem_cstring(data + 1, size - 1);

	if (str == NULL)
	{
		return -1;
	}

	int version = 0;

	switch (data[0] % 3)
	{
		case 0:
		{
			(void) parse_pg_version_string(str, &version);
			break;
		}

		case 1:
		{
			(void) parse_dotted_version_string(str, &version);
			break;
		}

		default:
		{
			(void) parse_pgaf_extension_version_string(str, &version);
			break;
		}
	}

	free(str);
	return 0;
}
