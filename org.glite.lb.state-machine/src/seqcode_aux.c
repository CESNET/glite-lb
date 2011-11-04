#ident "$Header$"
/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


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

	res =  sscanf(sc, EDG_WLL_SEQ_FORMAT_SCANF,
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION],
			&c[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SEQ_FORMAT_NUMBER) {
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

	res =  sscanf(sc, EDG_WLL_SEQ_FORMAT_SCANF,
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION],
			&c[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SEQ_FORMAT_NUMBER) {
/* FIXME:		syslog(LOG_ERR, "unparsable sequence code %s\n", sc); */
		fprintf(stderr, "unparsable sequence code %s\n", sc);
		return NULL;
	}

	c[index] = val;
	trio_asprintf(&ret, EDG_WLL_SEQ_FORMAT_PRINTF,
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
	unsigned int c[EDG_WLL_SEQ_PBS_FORMAT_NUMBER+1], d[EDG_WLL_SEQ_PBS_FORMAT_NUMBER+1];
	int	i, res;

	res = sscanf(a, EDG_WLL_SEQ_PBS_FORMAT_SCANF,
		     &c[0], &c[1], &c[2], &c[3], &c[4]);
	if(res != EDG_WLL_SEQ_PBS_FORMAT_NUMBER) {
		fprintf(stderr, "unparsable sequence code %s\n", a);
		return -1;
	}
		     
	res = sscanf(b, EDG_WLL_SEQ_PBS_FORMAT_SCANF,
		     &d[0], &d[1], &d[2], &d[3], &d[4]);
	if(res != EDG_WLL_SEQ_PBS_FORMAT_NUMBER) {
		fprintf(stderr, "unparsable sequence code %s\n", b);
		return -1;
	}

	for (i = 0 ; i <= EDG_WLL_SEQ_PBS_FORMAT_NUMBER; i++) {
		if (c[i] < d[i]) return -1;
		if (c[i] > d[i]) return  1;
	}

	return 0;
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


int edg_wll_compare_condor_seq(const char *a, const char *b) {
        char    timestamp_a[14], pos_a[10], src_a;
        char    timestamp_b[14], pos_b[10], src_b;
        int     ev_code_a, ev_code_b;
        int     res;

        res = sscanf(a,"TIMESTAMP=%14s:POS=%10s:EV.CODE=%3d:SRC=%c", timestamp_a, pos_a, &ev_code_a, &src_a);   

        if (res != 4) {
/* FIXME:               syslog(LOG_ERR, "unparsable sequence code %s\n", a); */
                fprintf(stderr, "unparsable sequence code %s\n", a);
                return -1;
        }

        res = sscanf(b,"TIMESTAMP=%14s:POS=%10s:EV.CODE=%3d:SRC=%c", timestamp_b, pos_b, &ev_code_b, &src_b);   

        if (res != 4) {
/* FIXME:               syslog(LOG_ERR, "unparsable sequence code %s\n", b); */
                fprintf(stderr, "unparsable sequence code %s\n", b);
                return -1;
        }

        /* wild card for JobReg - this event should always come as firt one          */
        /* bacause it hold job.type, which is necessary for further event processing    */
        if (ev_code_a == EDG_WLL_EVENT_REGJOB) return -1;
        if (ev_code_b == EDG_WLL_EVENT_REGJOB) return 1;

        /* sort event w.t.r. to timestamps */
        if ((res = strcmp(timestamp_a,timestamp_b)) != 0) {
                return res;
        }
        else {
                /* if timestamps equal, sort if w.t.r. to file position */
                /* if you both events come from the same log file       */
                if (src_a == src_b) {
                        /* zero mean in fact duplicate events in log    */
                        return strcmp(pos_a,pos_b);
                }
                /* if the events come from diffrent log files           */
                /* it is possible to prioritize some src log file       */
                else    {
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


int edg_wll_compare_seq(const char *a, const char *b)
{
	unsigned int    c[EDG_WLL_SOURCE__LAST];
	unsigned int    d[EDG_WLL_SOURCE__LAST];
	int		res, i;
	char		sca[EDG_WLL_SEQ_SIZE], scb[EDG_WLL_SEQ_SIZE];


	if ( (strstr(a,"SMOM=") == a) && (strstr(b,"SMOM=") == b) ) 
		return edg_wll_compare_pbs_seq(a,b);

	if (!strstr(a, "LBS")) snprintf(sca,EDG_WLL_SEQ_SIZE,"%s:LBS=000000",a);
	else snprintf(sca,EDG_WLL_SEQ_SIZE,"%s",a);
	if (!strstr(b, "LBS")) snprintf(scb,EDG_WLL_SEQ_SIZE,"%s:LBS=000000",b);
	else snprintf(scb,EDG_WLL_SEQ_SIZE,"%s",b);

	res =  sscanf(sca, EDG_WLL_SEQ_FORMAT_SCANF,
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION],
			&c[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SEQ_FORMAT_NUMBER) {
/* FIXME:		syslog(LOG_ERR, "unparsable sequence code %s\n", sca); */
		fprintf(stderr, "unparsable sequence code %s\n", sca);
		return -1;
	}

	res =  sscanf(scb, EDG_WLL_SEQ_FORMAT_SCANF,
			&d[EDG_WLL_SOURCE_USER_INTERFACE],
			&d[EDG_WLL_SOURCE_NETWORK_SERVER],
			&d[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&d[EDG_WLL_SOURCE_BIG_HELPER],
			&d[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&d[EDG_WLL_SOURCE_LOG_MONITOR],
			&d[EDG_WLL_SOURCE_LRMS],
			&d[EDG_WLL_SOURCE_APPLICATION],
			&d[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SEQ_FORMAT_NUMBER) {
/* FIXME:		syslog(LOG_ERR, "unparsable sequence code %s\n", scb); */
		fprintf(stderr, "unparsable sequence code %s\n", scb);
		return 1;
	}

	for (i = EDG_WLL_SOURCE_USER_INTERFACE ; i <= EDG_WLL_SEQ_FORMAT_NUMBER; i++) {
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



int add_taglist(const char *new_item, const char *new_item2, const char *seq_code, intJobStat *js)
{
	edg_wll_TagValue 	*itptr;
	int			i;

	if (js->pub.user_tags == NULL) {
		itptr = (edg_wll_TagValue *) calloc(2,sizeof(edg_wll_TagValue));
		itptr[0].tag = strdup(new_item);
		itptr[0].value = strdup(new_item2);
		js->pub.user_tags = itptr;
	
		js->tag_seq_codes = (char **) calloc(2,sizeof(char *));
		js->tag_seq_codes[0] = strdup(seq_code);

		return 1;
	} else {
		for (i = 0, itptr = js->pub.user_tags; itptr[i].tag != NULL; i++) {
			if ( !strcasecmp(itptr[i].tag, new_item)) {
				if (edg_wll_compare_seq(seq_code,js->tag_seq_codes[i]) == 1) {
					free(itptr[i].value);
					itptr[i].value = strdup(new_item2);

					free(js->tag_seq_codes[i]);
					js->tag_seq_codes[i] = strdup(seq_code);

					return 1;
				}
				else return 1;
			}
		}

		itptr = (edg_wll_TagValue *) realloc(js->pub.user_tags, (i+2)*sizeof(edg_wll_TagValue));
		js->tag_seq_codes = (char **) realloc(js->tag_seq_codes, (i+2) * sizeof(char *));

		if (itptr != NULL && js->tag_seq_codes != NULL) {
			itptr[i].tag = strdup(new_item);
			itptr[i].value = strdup(new_item2);
			itptr[i+1].tag = NULL;
			itptr[i+1].value = NULL;
			js->pub.user_tags = itptr;
			
			js->tag_seq_codes[i] = strdup(seq_code);
			js->tag_seq_codes[i+1] = NULL;

			return 1;
		} else {
			return 0;
		}
	}
}

