#ident "$Header$"

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/uio.h>


#include "glite/lb/trio.h"
#include "glite/lb/producer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/jobstat.h"

#include "get_events.h"
#include "store.h"
#include "lock.h"
#include "index.h"
#include "jobstat.h"

static int	rgma_fd = -1;
static int	rgma_sock = -1;
static struct sockaddr_un rgma_saddr;

static void write2rgma_sql(char *sqlstat)
{
	
	char *rgma_fname;
	int slen;
	int sysret;
	struct iovec iov[2];

	assert(sqlstat != NULL);

	if (rgma_fd == -1) {
		rgma_fname = getenv("EDG_WL_RGMA_FILE");
		if (rgma_fname == NULL) return;

		rgma_fd = open(rgma_fname, O_WRONLY|O_CREAT|O_APPEND, 0777);
		if (rgma_fd == -1) return;
	}

	slen = strlen(sqlstat);
	iov[0].iov_base = &slen;
	iov[0].iov_len = sizeof(slen);
	iov[1].iov_base = sqlstat;
	iov[1].iov_len = slen + 1;
	if ((sysret = flock(rgma_fd, LOCK_SH)) != -1) {
		sysret = writev(rgma_fd, iov, 2);
		flock(rgma_fd, LOCK_UN);
	}
	
	if (sysret == -1) return;

	if (rgma_sock == -1) {
		rgma_fname = getenv("EDG_WL_RGMA_SOCK");
		if (rgma_fname == NULL) return;

		if ((strlen(rgma_fname) + 1) > sizeof(rgma_saddr.sun_path))
			return ;

		rgma_sock = socket(PF_UNIX, SOCK_DGRAM,0);
		if (rgma_sock == -1) return;
		
		memset(&rgma_saddr, sizeof(rgma_saddr), 0);
		rgma_saddr.sun_family = PF_UNIX;
		strcpy(rgma_saddr.sun_path, rgma_fname);
	}

	sendto(rgma_sock, &slen, 1, 0,
		(struct sockaddr*) &rgma_saddr, SUN_LEN(&rgma_saddr));

	return;
}

void write2rgma_status(edg_wll_JobStat *stat)
{
	char *stmt = NULL;
	char *string_jobid, *string_stat, *string_server;

	string_jobid = edg_wlc_JobIdUnparse(stat->jobId);
	string_stat = edg_wll_StatToString(stat->state);
	string_server = edg_wlc_JobIdGetServer(stat->jobId);

	trio_asprintf(&stmt, "INSERT INTO JobStatusRaw VALUES ('%|Ss','%s','%|Ss','%|Ss', '%d%03d')",
			string_jobid, string_stat, stat->owner, string_server,
			stat->stateEnterTime.tv_sec, stat->stateEnterTime.tv_usec/1000);

	if (stmt) write2rgma_sql(stmt);

	free(string_jobid);
	free(string_stat);
	free(string_server);
	free(stmt);
}

#ifdef TEST
int main(int argc, char* argv[])
{
	setenv("EDG_WL_RGMA_FILE", "/tmp/rgma_statefile", 1);
	setenv("EDG_WL_RGMA_SOCK", "/tmp/rgma_statesock", 1);
 
	write2rgma_sql("INSERT INTO JobStatusRaw VALUES ('Job_id9','CLEARED','Owner','BKServer', '1050024158210')");
	return 0;
}
#endif
