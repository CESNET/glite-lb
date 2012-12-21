#include <sys/file.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "server_stats.h"

#include "glite/lb/context-int.h"

#include "glite/lbu/log.h"

static edg_wll_server_statistics *serverStatisticsMap = NULL;
static int serverStatisticsFD;
static int stats_in_tmp = 0;
static int msync_counter = 0;

char *edg_wll_server_statistics_type_title[] = {
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
        "L&B protocol accesses",
	"VM regs" };

char *edg_wll_server_statistics_type_key[] = {
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
        "queries_api",
	"vm_jobs" };

int edg_wll_InitServerStatistics(edg_wll_Context ctx, char *prefix)
{
	//TODO get file name from command line
        char *fname;
	int err;

/*	if (prefix)
		asprintf(&fname, "%s/lb_server_stats", file);
	else{*/
	if (! prefix){
		if (! (prefix = getenv("GLITE_LB_LOCATION_VAR"))) {
			struct stat info;
			if (stat("/var/lib/glite", &info) == 0 && S_ISDIR(info.st_mode))
				asprintf(&prefix, "/var/lib/glite/");
			else {
				asprintf(&prefix, "/tmp/");
				stats_in_tmp = 1;
			}
			glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_INFO,
                        "server stats file not configured, using default %s/lb_server_stats", prefix);
		}
	}
	asprintf(&fname, "%s/lb_server_stats", prefix);
	serverStatisticsFD = open(fname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (serverStatisticsFD < 0){
		serverStatisticsFD = open("/tmp/lb_server_stats", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_WARN,
                        "Server statistics: cannot open %s, trying to use /tmp/lb_server_stats instead.", fname);
		stats_in_tmp = 1;
	}
	if (serverStatisticsFD < 0) {
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_WARN, "Cannot use server statistics!");
		err = edg_wll_SetError(ctx,errno,fname);
		free(fname);
		return err;
	}

	off_t size = lseek(serverStatisticsFD, 0, SEEK_END) - lseek(serverStatisticsFD, 0, SEEK_SET);

	glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG,
                        "server stats: using %s, size %i", fname, (int)size);

	// modify file size if does not exist or format changed
	if (size == 0){
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_INFO,
                        "Server statistics does not exist, creating them...");
	        edg_wll_server_statistics *zero = calloc(SERVER_STATISTICS_COUNT, sizeof(*serverStatisticsMap));
		time_t now = time(NULL);
		int i;
		for (i = 0; i < SERVER_STATISTICS_COUNT; i++)
			zero[i].start = now;
		
        	write(serverStatisticsFD, zero, SERVER_STATISTICS_COUNT*sizeof(*serverStatisticsMap));
		free(zero);
	}
	else if (size < SERVER_STATISTICS_COUNT*sizeof(*serverStatisticsMap)){
		int new_stats = SERVER_STATISTICS_COUNT-size/sizeof(*serverStatisticsMap);
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_INFO,
                        "Server statistics format extended, adding %i new items.", new_stats);
		lseek(serverStatisticsFD, 0, SEEK_END);
		edg_wll_server_statistics *zero = calloc(new_stats, sizeof(*serverStatisticsMap));
		int i;
		time_t now = time(NULL);
		for (i = 0; i < new_stats; i++)
			zero[i].start = now;
		write(serverStatisticsFD, zero, SERVER_STATISTICS_COUNT*sizeof(*serverStatisticsMap) - size);
		lseek(serverStatisticsFD, 0, SEEK_SET);
		free(zero);
	}

	// read and mmap statistics
	serverStatisticsMap = mmap(NULL, SERVER_STATISTICS_COUNT*sizeof(*serverStatisticsMap), PROT_READ|PROT_WRITE, MAP_SHARED, serverStatisticsFD, 0);
        if (serverStatisticsMap == MAP_FAILED) return edg_wll_SetError(ctx,errno,"mmap()");
	read(serverStatisticsFD, serverStatisticsMap, SERVER_STATISTICS_COUNT*sizeof(*serverStatisticsMap));
//	msync(serverStatisticsMap, SERVER_STATISTICS_COUNT*sizeof(*serverStatisticsMap), MS_ASYNC);

	free(fname);

        return 0;
}

int edg_wll_ServerStatisticsIncrement(edg_wll_Context ctx, edg_wll_server_statistics_type type)
{
	if (! serverStatisticsMap)
		return -1;

	if (type >= SERVER_STATISTICS_COUNT){
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_WARN,
			"edg_wll_ServerStatisticsIncrement(): unknown statistic item.");
		return edg_wll_SetError(ctx,NULL,"edg_wll_ServerStatisticsIncrement()");
	}

	if (flock(serverStatisticsFD, LOCK_EX))
                return edg_wll_SetError(ctx,errno,"flock()");
	serverStatisticsMap[type].value++;
	flock(serverStatisticsFD, LOCK_UN);

	if (++msync_counter % 100 == 0)
		msync(serverStatisticsMap, SERVER_STATISTICS_COUNT*sizeof(*serverStatisticsMap), MS_ASYNC);

	return 0;
}

int edg_wll_ServerStatisticsGetValue(edg_wll_Context ctx, edg_wll_server_statistics_type type)
{
	if (! serverStatisticsMap)
		return -1;

        if (type >= SERVER_STATISTICS_COUNT){
                glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_WARN,
                        "edg_wll_ServerStatisticsGetValue(): unknown statistic item.");
                return -1;
        }

        if (flock(serverStatisticsFD, LOCK_EX))
                return edg_wll_SetError(ctx,errno,"flock()");
        int ret = serverStatisticsMap[type].value;
        flock(serverStatisticsFD, LOCK_UN);

        return ret;
}

time_t* edg_wll_ServerStatisticsGetStart(edg_wll_Context ctx, edg_wll_server_statistics_type type)
{
	if (! serverStatisticsMap)
		return NULL;

        if (type >= SERVER_STATISTICS_COUNT){
                glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_WARN,
                        "edg_wll_ServerStatisticsGetStart(): unknown statistic item.");
                return NULL;
        }

        return &(serverStatisticsMap[type].start);
}
int edg_wll_ServerStatisticsInTmp()
{
	return stats_in_tmp;
}


