#ident "$Header$"

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <assert.h>

#include "glite/lbu/trio.h"
#include "glite/lb/context-int.h"

#include "intjobstat.h"
#include "seqcode_aux.h"


/*
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <regex.h>
#include <syslog.h>

#include "glite/jobid/cjobid.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/db.h"
#include "glite/lb/context-int.h"

#include "store.h"
#include "index.h"
#include "jobstat.h"
#include "get_events.h"
*/

int component_seqcode(const char *a, edg_wll_Source index)
{
	unsigned int    c[EDG_WLL_SOURCE__LAST];
	int		res;
	char		sc[EDG_WLL_SEQ_SIZE];

	if (!strstr(a, "LBS")) snprintf(sc,EDG_WLL_SEQ_SIZE,"%s:LBS=000000",a);
	else snprintf(sc,EDG_WLL_SEQ_SIZE,"%s",a);

	res =  sscanf(sc, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d:LBS=%d",
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION],
			&c[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SOURCE__LAST-1) {
/* FIXME:		syslog(LOG_ERR, "unparsable sequence code %s\n", sc); */
		fprintf(stderr, "unparsable sequence code %s\n", sc);
		return -1;
	}

	return(c[index]);	
}

char * set_component_seqcode(char *a,edg_wll_Source index,int val)
{
	unsigned int    c[EDG_WLL_SOURCE__LAST];
	int		res;
	char 		*ret;
	char		sc[EDG_WLL_SEQ_SIZE];

	if (!strstr(a, "LBS")) snprintf(sc,EDG_WLL_SEQ_SIZE,"%s:LBS=000000",a);
	else snprintf(sc,EDG_WLL_SEQ_SIZE,"%s",a);

	res =  sscanf(sc, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d:LBS=%d",
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION],
			&c[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SOURCE__LAST-1) {
/* FIXME:		syslog(LOG_ERR, "unparsable sequence code %s\n", sc); */
		fprintf(stderr, "unparsable sequence code %s\n", sc);
		return NULL;
	}

	c[index] = val;
	trio_asprintf(&ret,"UI=%06d:NS=%010d:WM=%06d:BH=%010d:JSS=%06d"
                                ":LM=%06d:LRMS=%06d:APP=%06d:LBS=%06d",
                        c[EDG_WLL_SOURCE_USER_INTERFACE],
                        c[EDG_WLL_SOURCE_NETWORK_SERVER],
                        c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
                        c[EDG_WLL_SOURCE_BIG_HELPER],
                        c[EDG_WLL_SOURCE_JOB_SUBMISSION],
                        c[EDG_WLL_SOURCE_LOG_MONITOR],
                        c[EDG_WLL_SOURCE_LRMS],
                        c[EDG_WLL_SOURCE_APPLICATION],
                        c[EDG_WLL_SOURCE_LB_SERVER]);
	return ret;
}

int before_deep_resubmission(const char *a, const char *b)
{
	if (component_seqcode(a, EDG_WLL_SOURCE_WORKLOAD_MANAGER) < 
	    component_seqcode(b, EDG_WLL_SOURCE_WORKLOAD_MANAGER) )
		return(1);
	else
		return(0);

}

int same_branch(const char *a, const char *b) 
{
	if (component_seqcode(a, EDG_WLL_SOURCE_WORKLOAD_MANAGER) == 
	    component_seqcode(b, EDG_WLL_SOURCE_WORKLOAD_MANAGER) )
		return(1);
	else
		return(0);
}

int edg_wll_compare_pbs_seq(const char *a,const char *b)
{
	char	timestamp_a[14], pos_a[10], src_a;
	char	timestamp_b[14], pos_b[10], src_b;
	int	ev_code_a, ev_code_b;
	int	res;

	res = sscanf(a,"TIMESTAMP=%14s:POS=%10s:EV.CODE=%3d:SRC=%c", timestamp_a, pos_a, &ev_code_a, &src_a);	

	if (res != 4) {
/* FIXME:		syslog(LOG_ERR, "unparsable sequence code %s\n", a); */
		fprintf(stderr, "unparsable sequence code %s\n", a);
		return -1;
	}

	res = sscanf(b,"TIMESTAMP=%14s:POS=%10s:EV.CODE=%3d:SRC=%c", timestamp_b, pos_b, &ev_code_b, &src_b);	

	if (res != 4) {
/* FIXME:		syslog(LOG_ERR, "unparsable sequence code %s\n", b); */
		fprintf(stderr, "unparsable sequence code %s\n", b);
		return -1;
	}

	/* wild card for PBSJobReg - this event should always come as firt one 		*/
	/* bacause it hold job.type, which is necessary for further event processing	*/
	if (ev_code_a == EDG_WLL_EVENT_REGJOB) return -1;
	if (ev_code_b == EDG_WLL_EVENT_REGJOB) return 1;

	/* sort event w.t.r. to timestamps */
	if ((res = strcmp(timestamp_a,timestamp_b)) != 0) {
		return res;
	}
	else {
		/* if timestamps equal, sort if w.t.r. to file position */
		/* if you both events come from the same log file	*/
		if (src_a == src_b) {
			/* zero mean in fact duplicate events in log	*/
			return strcmp(pos_a,pos_b);
		}
		/* if the events come from diffrent log files		*/
		/* it is possible to prioritize some src log file	*/
		else	{
			/* prioritize events from pbs_mom */
			if (src_a == 'm') return 1;
			if (src_b == 'm') return -1;

			/* then prioritize events from pbs_server */
			if (src_a == 's') return 1;
			if (src_b == 's') return -1;

			/* other priorities comes here... */
		}	
	}

	return 0;
}

edg_wll_PBSEventSource get_pbs_event_source(const char *pbs_seq_num) {
	switch (pbs_seq_num[EDG_WLL_SEQ_PBS_SIZE - 2]) {
		case 'c': return(EDG_WLL_PBS_EVENT_SOURCE_SCHEDULER);
		case 's': return(EDG_WLL_PBS_EVENT_SOURCE_SERVER);
		case 'm': return(EDG_WLL_PBS_EVENT_SOURCE_MOM);
		case 'a': return(EDG_WLL_PBS_EVENT_SOURCE_ACCOUNTING);
		default: return(EDG_WLL_PBS_EVENT_SOURCE_UNDEF);
	}
}

edg_wll_CondorEventSource get_condor_event_source(const char *condor_seq_num) {
	switch (condor_seq_num[EDG_WLL_SEQ_CONDOR_SIZE - 2]) {
		case 'L': return(EDG_WLL_CONDOR_EVENT_SOURCE_COLLECTOR);
		case 'M': return(EDG_WLL_CONDOR_EVENT_SOURCE_MASTER);
		case 'm': return(EDG_WLL_CONDOR_EVENT_SOURCE_MATCH);
		case 'N': return(EDG_WLL_CONDOR_EVENT_SOURCE_NEGOTIATOR);
		case 'C': return(EDG_WLL_CONDOR_EVENT_SOURCE_SCHED);
		case 'H': return(EDG_WLL_CONDOR_EVENT_SOURCE_SHADOW);
		case 's': return(EDG_WLL_CONDOR_EVENT_SOURCE_STARTER);
		case 'S': return(EDG_WLL_CONDOR_EVENT_SOURCE_START);
		case 'j': return(EDG_WLL_CONDOR_EVENT_SOURCE_JOBQUEUE);
		default: return(EDG_WLL_CONDOR_EVENT_SOURCE_UNDEF);
	}
}

int edg_wll_compare_seq(const char *a, const char *b)
{
	unsigned int    c[EDG_WLL_SOURCE__LAST];
	unsigned int    d[EDG_WLL_SOURCE__LAST];
	int		res, i;
	char		sca[EDG_WLL_SEQ_SIZE], scb[EDG_WLL_SEQ_SIZE];


	if ( (strstr(a,"TIMESTAMP=") == a) && (strstr(b,"TIMESTAMP=") == b) ) 
		return edg_wll_compare_pbs_seq(a,b);

	if (!strstr(a, "LBS")) snprintf(sca,EDG_WLL_SEQ_SIZE,"%s:LBS=000000",a);
	else snprintf(sca,EDG_WLL_SEQ_SIZE,"%s",a);
	if (!strstr(b, "LBS")) snprintf(scb,EDG_WLL_SEQ_SIZE,"%s:LBS=000000",b);
	else snprintf(scb,EDG_WLL_SEQ_SIZE,"%s",b);

	assert(EDG_WLL_SOURCE__LAST == 10);

	res =  sscanf(sca, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d:LBS=%d",
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION],
			&c[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SOURCE__LAST-1) {
/* FIXME:		syslog(LOG_ERR, "unparsable sequence code %s\n", sca); */
		fprintf(stderr, "unparsable sequence code %s\n", sca);
		return -1;
	}

	res =  sscanf(scb, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d:LBS=%d",
			&d[EDG_WLL_SOURCE_USER_INTERFACE],
			&d[EDG_WLL_SOURCE_NETWORK_SERVER],
			&d[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&d[EDG_WLL_SOURCE_BIG_HELPER],
			&d[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&d[EDG_WLL_SOURCE_LOG_MONITOR],
			&d[EDG_WLL_SOURCE_LRMS],
			&d[EDG_WLL_SOURCE_APPLICATION],
			&d[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SOURCE__LAST-1) {
/* FIXME:		syslog(LOG_ERR, "unparsable sequence code %s\n", scb); */
		fprintf(stderr, "unparsable sequence code %s\n", scb);
		return 1;
	}

	for (i = EDG_WLL_SOURCE_USER_INTERFACE ; i < EDG_WLL_SOURCE__LAST; i++) {
		if (c[i] < d[i]) return -1;
		if (c[i] > d[i]) return  1;
	}

	return 0;
}


int compare_events_by_seq(const void *a, const void *b)
{
        const edg_wll_Event *e = (edg_wll_Event *) a;
        const edg_wll_Event *f = (edg_wll_Event *) b;
	int ret;


	ret = edg_wll_compare_seq(e->any.seqcode, f->any.seqcode);
	if (ret) return ret;
	
	if (e->any.timestamp.tv_sec < f->any.timestamp.tv_sec) return -1;
	if (e->any.timestamp.tv_sec > f->any.timestamp.tv_sec) return 1;
	if (e->any.timestamp.tv_usec < f->any.timestamp.tv_usec) return -1;
	if (e->any.timestamp.tv_usec > f->any.timestamp.tv_usec) return 1;
	return 0;
}

