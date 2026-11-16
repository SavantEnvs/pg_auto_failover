/* JSON parsing of monitor state notifications and of the nodes array. */
#include <stdbool.h>

#include "harness_common.h"
#include "nodestate_utils.h"
#include "parsing.h"
#include "pgsql.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 1)
	{
		return -1;
	}

	mayhem_quiet();

	char *json = mayhem_cstring(data + 1, size - 1);

	if (json == NULL)
	{
		return -1;
	}

	if (data[0] & 1)
	{
		CurrentNodeState *nodeState = (CurrentNodeState *) calloc(1, sizeof(*nodeState));

		if (nodeState != NULL)
		{
			(void) parse_state_notification_message(nodeState, json);
		}
		free(nodeState);
	}
	else
	{
		NodeAddressArray *nodesArray = (NodeAddressArray *) calloc(1, sizeof(*nodesArray));

		if (nodesArray != NULL)
		{
			(void) parseNodesArray(json, nodesArray, (int64_t) data[0]);
		}
		free(nodesArray);
	}

	free(json);
	return 0;
}
