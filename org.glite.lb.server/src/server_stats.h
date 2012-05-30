#ifndef GLITE_LB_SERVER_STATS_H
#define GLITE_LB_SERVER_STATS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include "glite/lb/context.h"

typedef struct _edg_wll_server_statistics{
	uint64_t value;
	time_t start;
} edg_wll_server_statistics;

enum edg_wll_server_statistics_type{
	SERVER_STATS_GLITEJOB_REGS = 0,
	SERVER_STATS_PBSJOB_REGS,
	SERVER_STATS_CONDOR_REGS,
	SERVER_STATS_CREAM_REGS,
	SERVER_STATS_SANDBOX_REGS,
	SERVER_STATS_JOB_EVENTS,
	SERVER_STATS_HTML_VIEWS,
	SERVER_STATS_TEXT_VIEWS,
	SERVER_STATS_RSS_VIEWS,
	SERVER_STATS_NOTIF_LEGACY_REGS,
	SERVER_STATS_NOTIF_MSG_REGS,
	SERVER_STATS_NOTIF_LEGACY_SENT,
	SERVER_STATS_NOTIF_MSG_SENT,
	SERVER_STATS_WS_QUERIES,
	SERVER_STATS_LBPROTO,
	SERVER_STATISTICS_COUNT
};
// Add new values in front of LAST and do not change order

static const char *edg_wll_server_statistics_type_title[] = {
	"gLite job regs",
	"PBS job regs",
	"Condor job regs",
	"CREAM job regs",
	"Sandbox regs",
	"Job events",
	"HTML accesses",
	"Plain text accesses",
	"RSS accesses",
	"Notification regs (legacy interface)",
	"Notification regs (msg interface)",
	"Notifications sent (legacy)",
	"Notifications sent (msg)",
	"WS queries",
	"L&B protocol accesses" };

static const char *edg_wll_server_statistics_type_key[] = {
	"glite_jobs",
	"pbs_jobs",
	"condor_jobs",
	"cream_jobs",
	"sb_ft_jobs",
	"events",
	"queries_html",
	"queries_text",
	"queries_rss",
	"notif_regs_legacy",
	"notif_regs_msg",
	"notifs_legacy",
	"notis_msg",
	"queries_ws",
	"queries_api" };

int edg_wll_InitServerStatistics(edg_wll_Context ctx, char *prefix);

int edg_wll_ServerStatisticsIncrement(edg_wll_Context ctx, enum edg_wll_server_statistics_type type);
int edg_wll_ServerStatisticsGetValue(edg_wll_Context ctx, enum edg_wll_server_statistics_type type);
time_t* edg_wll_ServerStatisticsGetStart(edg_wll_Context ctx, enum edg_wll_server_statistics_type type);
int edg_wll_ServerStatisticsInTmp();

#endif /* GLITE_LB_SERVER_STATS_H */
