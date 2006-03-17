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


#include <cclassad.h>

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

static void write2rgma_line(char *line, int old_style)
{
	
	int slen;
	int sysret;
	struct iovec iov[2];

	static int	rgma_sock = -1;
	static char    *rgma_fname = NULL;
	static struct sockaddr_un rgma_saddr;

	static int	rgma_fd_locked = 0;
        struct stat 	stat_buf,stat_fbuf;
        struct flock 	filelock;
        int 		filelock_status;

	assert(line != NULL);

	if (rgma_fd < -1) return;
	if (rgma_fd == -1) {
		rgma_fname = getenv("GLITE_WMS_LCGMON_FILE");
		if (rgma_fname == NULL) {
			rgma_fname = getenv("EDG_WL_RGMA_FILE");
			if (rgma_fname == NULL) {
				rgma_fd = -2; 
				return;
			}
		}

		rgma_fd = open(rgma_fname, O_WRONLY|O_CREAT|O_APPEND, 0777);
		if (rgma_fd == -1) return;
	}


	slen = strlen(line);
	if (old_style) {
		iov[0].iov_base = &slen;
		iov[0].iov_len = sizeof(slen);
		iov[1].iov_base = line;
		iov[1].iov_len = slen + 1;
	} else {
		iov[0].iov_base = line;
		iov[0].iov_len = slen;
	}
	
	if (old_style) {

		if ((sysret = flock(rgma_fd, LOCK_SH)) != -1) {
			sysret = writev(rgma_fd, iov, 2);
			flock(rgma_fd, LOCK_UN);
		}
		if (sysret == -1) return;
	} else {
		if (!rgma_fd_locked) {
	            do {
	              filelock.l_type = F_WRLCK;
	              filelock.l_whence = SEEK_SET;
	              filelock.l_start = 0;
	              filelock.l_len = 0;
	            } while((filelock_status = fcntl(rgma_fd,F_SETLKW,&filelock))<0 && errno==EINTR);
		    if (filelock_status<0) goto leave;
		}
		rgma_fd_locked = 1;
		if (fstat(rgma_fd, &stat_fbuf)<0 || stat(rgma_fname, &stat_buf)<0)
			goto leave;
		if (stat_fbuf.st_dev != stat_buf.st_dev || stat_fbuf.st_ino != stat_buf.st_ino)
			goto leave;
                if (writev(rgma_fd, iov, 1) < 0) 
			goto leave;
		fsync(rgma_fd);
		do {
            		filelock.l_type = F_UNLCK;
            		filelock.l_whence = SEEK_SET;
            		filelock.l_start = 0;
            		filelock.l_len = 0;
          	} while((filelock_status = fcntl(rgma_fd,F_SETLKW,&filelock))<0 && errno==EINTR);
		if (filelock_status < 0) goto leave;
		rgma_fd_locked = 0;
	leave:	
		/* add explicit unlock, incase the later close should fail for some reason */
	        if (rgma_fd_locked) {
       			do {
              			filelock.l_type = F_UNLCK;
              			filelock.l_whence = SEEK_SET;
              			filelock.l_start = 0;
              			filelock.l_len = 0;
            		} while((filelock_status = fcntl(rgma_fd,F_SETLKW,&filelock))<0 && errno==EINTR);
        	  	if (filelock_status == 0) {
            			rgma_fd_locked = 0;
          		}
        	}
        	/* close the statefile */
        	if (close(rgma_fd)==0) {
          		rgma_fd = -1;
          		rgma_fd_locked = 0;
        	}

	}

	if (rgma_sock < -1) return;

	if (rgma_sock == -1) {
		rgma_fname = getenv("EDG_WL_RGMA_SOCK");
		if (rgma_fname == NULL 
		    || (strlen(rgma_fname) + 1) > sizeof(rgma_saddr.sun_path) ) {
			rgma_sock = -2;
			return;
		}

		rgma_sock = socket(PF_UNIX, SOCK_DGRAM,0);
		if (rgma_sock == -1) return;
		
		memset(&rgma_saddr, sizeof(rgma_saddr), 0);
		rgma_saddr.sun_family = PF_UNIX;
		strcpy(rgma_saddr.sun_path, rgma_fname);
	}

	sendto(rgma_sock, &slen, 1, MSG_DONTWAIT,
		(struct sockaddr*) &rgma_saddr, SUN_LEN(&rgma_saddr));

	return;
}


char* write2rgma_statline(edg_wll_JobStat *stat)
{
	char *stmt = NULL;
	char *string_jobid, *string_stat, *string_server;
	char *string_vo = NULL;

	string_jobid = edg_wlc_JobIdUnparse(stat->jobId);
	string_stat = edg_wll_StatToString(stat->state);
	string_server = edg_wlc_JobIdGetServer(stat->jobId);

	if (stat->jdl != NULL) {
		struct cclassad *ad;
	
		ad = cclassad_create(stat->jdl);
		if (ad) {
			if (!cclassad_evaluate_to_string(ad, "VirtualOrganisation", &string_vo))
				string_vo = NULL;
			cclassad_delete(ad);
		} 
	}

	trio_asprintf(&stmt,
		"JOBID=%|Ss "
		"OWNER=%|Ss "
		"BKSERVER=%|Ss "
		"NETWORKSERVER=%|Ss "
		"VO=%|Ss "
		"LASTUPDATETIME=%d "
		"STATENAME=%s "
		"STATEENTERTIME=%d "
		"CONDORID=%|Ss "
		"DESTINATION=%|Ss "
		"EXITCODE=%d "
		"DONECODE=%d "
		"STATUSREASON=%|Ss"
		"\n",
		string_jobid,
		stat->owner,
		string_server,
		(stat->network_server) ? (stat->network_server) : "",
		(string_vo) ? string_vo : "",
		stat->lastUpdateTime.tv_sec,
		string_stat,
		stat->stateEnterTime.tv_sec,
		(stat->condorId) ? (stat->condorId) : "",
		(stat->destination) ? (stat->destination) : "",
		stat->exit_code,
		stat->done_code,
		(stat->reason) ? (stat->reason) : ""
	);
				
	free(string_vo);
	free(string_jobid);
	free(string_stat);
	free(string_server);
	
	return stmt;
}

void write2rgma_status(edg_wll_JobStat *stat)
{
	char *line;
	int lcgmon = 0;
	
	if (rgma_fd < -1) return;

	line = write2rgma_statline(stat);
	if (line) {
		if (getenv("GLITE_WMS_LCGMON_FILE")) lcgmon = 1;
		write2rgma_line(line, !lcgmon);
	}
	free(line);
}

/* Export status record only if new status line is different from
   previous one. free() prev_statline parameter. */

void write2rgma_chgstatus(edg_wll_JobStat *stat, char *prev_statline)
{
	char *line;
	int lcgmon = 0;
	
	if (rgma_fd < -1) return;

	line = write2rgma_statline(stat);
	if (line && (!prev_statline || strcmp(line, prev_statline))) {
		if (getenv("GLITE_WMS_LCGMON_FILE")) lcgmon = 1;
		write2rgma_line(line, !lcgmon);
	}
	free(line);
	free(prev_statline);
}

#ifdef TEST
int main(int argc, char* argv[])
{
	setenv("EDG_WL_RGMA_FILE", "/tmp/rgma_statefile", 1);
	setenv("EDG_WL_RGMA_SOCK", "/tmp/rgma_statesock", 1);
 
	write2rgma_line("123---789\n", 0);
	return 0;
}
#endif
