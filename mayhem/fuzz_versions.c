/*
 * pg_ctl version output, LSN, boolean, integer/float and enum-string parsers from src/bin/common
 * (parsing.c, string_utils.c, ipaddr.c, pgsetup.c).
 *
 * Case 0 (parse_version_number) still reaches parse_dotted_version_string through
 * parse_pg_version_string, so this target can hit its signed overflow at parsing.c:217
 * (known-findings/version-number-int-overflow). It cannot reach that function's out-of-bounds
 * write: parse_version_number first truncates the version to PG_VERSION_STRING_MAX (12) bytes,
 * shorter than the char[21] component buffers. fuzz_dotted_version.c feeds the dotted parser
 * directly so both of its bugs are explored without stopping this target's other parsers early.
 */
#include <stdbool.h>

#include "harness_common.h"
#include "ipaddr.h"
#include "parsing.h"
#include "pgsetup.h"
#include "string_utils.h"

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
	uint64_t lsn = 0;
	bool flag = false;
	char versionString[PG_VERSION_STRING_MAX] = { 0 };

	switch (data[0] % 14)
	{
		case 0:
		{
			(void) parse_version_number(str, versionString,
										sizeof(versionString), &version);
			break;
		}

		case 1:
		{
			(void) parse_bool(str, &flag);
			break;
		}

		case 2:
		{
			(void) parseLSN(str, &lsn);
			break;
		}

		case 3:
		{
			int number = 0;
			(void) stringToInt(str, &number);
			break;
		}

		case 4:
		{
			unsigned int number = 0;
			(void) stringToUInt(str, &number);
			break;
		}

		case 5:
		{
			int64_t number = 0;
			(void) stringToInt64(str, &number);
			break;
		}

		case 6:
		{
			uint64_t number = 0;
			(void) stringToUInt64(str, &number);
			break;
		}

		case 7:
		{
			short number = 0;
			(void) stringToShort(str, &number);
			break;
		}

		case 8:
		{
			unsigned short number = 0;
			(void) stringToUShort(str, &number);
			break;
		}

		case 9:
		{
			int32_t number = 0;
			uint32_t unumber = 0;
			(void) stringToInt32(str, &number);
			(void) stringToUInt32(str, &unumber);
			break;
		}

		case 10:
		{
			double number = 0;
			(void) stringToDouble(str, &number);
			break;
		}

		case 11:
		{
			(void) ip_address_type(str);
			break;
		}

		case 12:
		{
			(void) pgsetup_parse_sslmode(str);
			(void) pgsetup_parse_hba_level(str);
			break;
		}

		default:
		{
			(void) nodeKindFromString(str);
			break;
		}
	}

	free(str);
	return 0;
}
