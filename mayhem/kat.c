/*
 * Known-answer probe for pg_auto_failover's parsers. Built with the project's normal flags by
 * mayhem/build.sh and run by mayhem/test.sh, which diffs this program's stdout against golden text.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ini_file.h"
#include "log.h"
#include "nodestate_utils.h"
#include "parsing.h"
#include "pgsetup.h"
#include "pgsql.h"
#include "state.h"

static const char *CONTROLDATA =
	"pg_control version number:            1700\n"
	"Catalog version number:               202406281\n"
	"Database system identifier:           7412345678901234567\n"
	"Database cluster state:               in production\n"
	"pg_control last modified:             Mon 16 Nov 2026 10:00:00 AM UTC\n"
	"Latest checkpoint location:           0/3000060\n"
	"Latest checkpoint's REDO location:    0/3000028\n"
	"Latest checkpoint's REDO WAL file:    000000020000000000000003\n"
	"Latest checkpoint's TimeLineID:       2\n"
	"Latest checkpoint's PrevTimeLineID:   1\n"
	"Minimum recovery ending location:     0/0\n";

static const char *NOTIFICATION =
	"{\"type\":\"state\",\"formation\":\"default\",\"groupId\":0,\"nodeId\":3,"
	"\"name\":\"node_3\",\"host\":\"db3.example.com\",\"port\":5433,"
	"\"reportedState\":\"secondary\",\"goalState\":\"primary\",\"health\":\"good\"}";

static const char *NODES =
	"[{\"node_id\":1,\"node_lsn\":\"0/3000060\",\"node_name\":\"a\","
	"\"node_host\":\"h1\",\"node_port\":5432,\"node_is_primary\":true},"
	"{\"node_id\":2,\"node_lsn\":\"1/A0\",\"node_name\":\"b\","
	"\"node_host\":\"h2\",\"node_port\":5433,\"node_is_primary\":false},"
	"{\"node_id\":9,\"node_lsn\":\"0/1\",\"node_name\":\"me\","
	"\"node_host\":\"h9\",\"node_port\":5439,\"node_is_primary\":false}]";

static const char *TIMELINE_HISTORY =
	"1\t0/3000060\tno recovery target specified\n"
	"\n"
	"2\t0/50000A8\tno recovery target specified\n";

int
main(void)
{
	log_set_quiet(true);

	int version = -1;
	char versionString[PG_VERSION_STRING_MAX] = { 0 };

	bool ok = parse_pg_version_string("17.4", &version);
	printf("pg_version 17.4 ok=%d value=%d\n", ok, version);

	ok = parse_pg_version_string("14", &version);
	printf("pg_version 14 ok=%d value=%d\n", ok, version);

	ok = parse_pg_version_string("1.2.3", &version);
	printf("pg_version 1.2.3 ok=%d\n", ok);

	ok = parse_version_number("pg_ctl (PostgreSQL) 10.3", versionString,
							  sizeof(versionString), &version);
	printf("pg_ctl_version ok=%d string=%s value=%d\n", ok, versionString, version);

	ok = parse_pgaf_extension_version_string("2.1", &version);
	printf("extension_version ok=%d value=%d\n", ok, version);

	bool flag = false;
	ok = parse_bool("on", &flag);
	printf("bool on ok=%d value=%d\n", ok, flag);
	ok = parse_bool("No", &flag);
	printf("bool No ok=%d value=%d\n", ok, flag);
	ok = parse_bool("maybe", &flag);
	printf("bool maybe ok=%d\n", ok);

	uint64_t lsn = 0;
	ok = parseLSN("16/B374D848", &lsn);
	printf("lsn 16/B374D848 ok=%d value=%llx\n", ok, (unsigned long long) lsn);
	ok = parseLSN("0/3000060", &lsn);
	printf("lsn 0/3000060 ok=%d value=%llx\n", ok, (unsigned long long) lsn);
	ok = parseLSN("123456789/0", &lsn);
	printf("lsn 123456789/0 ok=%d\n", ok);

	PostgresControlData control = { 0 };
	ok = parse_controldata(&control, CONTROLDATA);
	printf("controldata ok=%d control=%u catalog=%u sysid=%llu tli=%u "
		   "checkpoint=%s redo=%s minrecovery=%s\n",
		   ok, control.pg_control_version, control.catalog_version_no,
		   (unsigned long long) control.system_identifier, control.timeline_id,
		   control.latestCheckpointLSN, control.latestCheckpointRedoLSN,
		   control.minRecoveryEndLSN);

	char *scrubbed = calloc(1, MAXCONNINFO);
	ok = parse_and_scrub_connection_string(
		"postgres://autoctl:s3cret@monitor.example.com:5432/pg_auto_failover?sslmode=require",
		scrubbed);
	printf("scrub ok=%d uri=%s\n", ok, scrubbed);
	free(scrubbed);

	SSLOptions ssl = { 0 };
	ok = parse_pguri_ssl_settings(
		"postgres://u@h:5432/db?sslmode=verify-full&sslrootcert=/ca.crt",
		&ssl);
	printf("ssl ok=%d mode=%s ca=%s\n", ok, ssl.sslModeStr, ssl.caFile);

	CurrentNodeState *nodeState = calloc(1, sizeof(CurrentNodeState));
	ok = parse_state_notification_message(nodeState, NOTIFICATION);
	printf("notification ok=%d formation=%s node=%d name=%s host=%s port=%d "
		   "reported=%s goal=%s health=%d\n",
		   ok, nodeState->formation, nodeState->node.nodeId, nodeState->node.name,
		   nodeState->node.host, nodeState->node.port,
		   NodeStateToString(nodeState->reportedState),
		   NodeStateToString(nodeState->goalState), nodeState->health);
	free(nodeState);

	NodeAddressArray *nodes = calloc(1, sizeof(NodeAddressArray));
	ok = parseNodesArray(NODES, nodes, 9);
	printf("nodes ok=%d count=%d\n", ok, nodes->count);
	for (int i = 0; i < nodes->count; i++)
	{
		printf("node id=%d name=%s host=%s port=%d lsn=%s primary=%d\n",
			   nodes->nodes[i].nodeId, nodes->nodes[i].name, nodes->nodes[i].host,
			   nodes->nodes[i].port, nodes->nodes[i].lsn, nodes->nodes[i].isPrimary);
	}
	free(nodes);

	IdentifySystem system = { 0 };
	system.timeline = 3;
	char *history = strdup(TIMELINE_HISTORY);   /* parseTimeLineHistory splits its input in place */
	ok = parseTimeLineHistory("00000003.history", history, &system);
	free(history);
	printf("timeline ok=%d count=%d\n", ok, system.timelines.count);
	for (int i = 0; ok && i < system.timelines.count; i++)
	{
		printf("tli=%u begin=%llx end=%llx\n", system.timelines.history[i].tli,
			   (unsigned long long) system.timelines.history[i].begin,
			   (unsigned long long) system.timelines.history[i].end);
	}
	free(system.timelines.history);

	char *name = NULL;
	char pgdata[64] = { 0 };
	int port = -1;
	IniOption options[] = {
		make_string_option_default("pg_autoctl", "name", NULL, false, &name, "node"),
		make_strbuf_option("postgresql", "pgdata", NULL, false, sizeof(pgdata), pgdata),
		make_int_option_default("postgresql", "port", NULL, false, &port, 5432),
		INI_OPTION_LAST
	};
	char *ini = strdup(
		"[pg_autoctl]\nname = keeper-a\n\n[postgresql]\npgdata = /var/lib/pg\nport = 5433\n");
	ok = parse_ini_buffer("kat.cfg", ini, options);   /* frees ini */
	printf("ini ok=%d name=%s pgdata=%s port=%d\n", ok, name, pgdata, port);
	free(name);

	return 0;
}
