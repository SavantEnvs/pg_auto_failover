/* Postgres connection-string (URI and key=value) parsing, SSL option extraction and password scrubbing. */
#include <stdbool.h>

#include "harness_common.h"
#include "parsing.h"
#include "pgsetup.h"
#include "pgsql.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	/* parsing.h: URIs are bounded by MAXCONNINFO; callers store them in MAXCONNINFO buffers */
	if (size < 1 || size >= MAXCONNINFO)
	{
		return -1;
	}

	mayhem_quiet();

	char *pguri = mayhem_cstring(data + 1, size - 1);

	if (pguri == NULL)
	{
		return -1;
	}

	switch (data[0] % 4)
	{
		case 0:
		{
			URIParams *uriParams = (URIParams *) calloc(1, sizeof(URIParams));
			KeyVal *overrides = (KeyVal *) calloc(1, sizeof(KeyVal));
			char *rebuilt = (char *) calloc(1, MAXCONNINFO);

			if (uriParams != NULL && overrides != NULL && rebuilt != NULL)
			{
				if (parse_pguri_info_key_vals(pguri, overrides, uriParams, true))
				{
					(void) buildPostgresURIfromPieces(uriParams, rebuilt);
				}
			}
			free(uriParams);
			free(overrides);
			free(rebuilt);
			break;
		}

		case 1:
		{
			URIParams *uriParams = (URIParams *) calloc(1, sizeof(URIParams));
			KeyVal *overrides = (KeyVal *) calloc(1, sizeof(KeyVal));

			if (uriParams != NULL && overrides != NULL)
			{
				(void) parse_pguri_info_key_vals(pguri, overrides, uriParams, false);
			}
			free(uriParams);
			free(overrides);
			break;
		}

		case 2:
		{
			SSLOptions ssl = { 0 };

			(void) parse_pguri_ssl_settings(pguri, &ssl);
			break;
		}

		default:
		{
			char *scrubbed = (char *) calloc(1, MAXCONNINFO);

			if (scrubbed != NULL)
			{
				(void) parse_and_scrub_connection_string(pguri, scrubbed);
			}
			free(scrubbed);
			break;
		}
	}

	free(pguri);
	return 0;
}
