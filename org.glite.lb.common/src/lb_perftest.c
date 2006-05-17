#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lb_perftest.h"
#include "glite/lb/producer.h"
#include "glite/lb/trio.h"
#include "il_msg.h"

#ident "$Header$"

static pthread_mutex_t perftest_lock = PTHREAD_MUTEX_INITIALIZER;
static struct timeval endtime;
static char *termination_string;
static char *events[];
static int nevents;
static int njobs = 0;
static int cur_event = 0;
static int cur_job = 0;

#define EVENTS_BUFSIZ 16
#define BUFFSZ        1024


/* 
 * reading lines (pasted from load.c)
 */

static 
int 
read_line(int fd, char **buff, size_t *maxsize)
{
        int             ct, i;
        void            *tmp;


        if ( *buff == NULL )
        {
                *buff = malloc(BUFFSZ);
                if ( *buff == NULL )
                        return -1;
                *maxsize = BUFFSZ;
        }

        i = 0;
        while ( 1 )
        {
                if (i >= *maxsize) {
                        (*maxsize) *= 2;
                        if ((tmp = realloc(*buff, *maxsize)) == NULL) return -1;
                        *buff = (char *)tmp;
                }
                if ( (ct = read(fd, (*buff)+i, 1)) == -1 )
                        return -1;

                if ( ct == 0 )
                        return 0;

                if ( (*buff)[i] == '\n' )
                {
                        (*buff)[i--] = '\0';
                        while ( (i != -1) && isspace((*buff)[i]) ) i--;
                        if ( i == -1 )
                        {
                                /**     empty line
                                 */
                                i = 0;
                                continue;
                        }
                        else
                                return 1;
                }

                i++;
        }

}


/* 
 * read events from the stream
 */
static 
int 
read_events(int fd, char *(*events[])) 
{
	void *tmp;
	size_t maxlinesize;
	char **e = NULL, *line = NULL;
	size_t i = 0, max = EVENTS_BUFSIZ;

	if ((e = malloc(max * sizeof(char *))) == NULL) goto nomem;

	while (read_line(fd, &line, &maxlinesize)) {
		if (i + 1 >= max) {
			max <<= 1;
			if ((tmp = realloc(e, max * sizeof(char *))) == NULL) goto nomem;
			e = (char **)tmp;
		}
		if ((e[i] = strdup(line)) == NULL) goto nomem;
		free(line); line = NULL;
		i++;
	}

	*events = e;
	return i;

nomem:
	if(e) free(e);
	if(line) free(line);
	fprintf(stderr, "read_events: insufficient memory\n");
	return -1;
}


int 
glite_wll_perftest_init(const char *host,
			int port,
			const char *user,
			const char *testname,
			const char *filename, 
			int n)
{

	if(trio_asprintf(&termination_string, EDG_WLL_FORMAT_USERTAG,
		    PERFTEST_END_TAG_NAME, PERFTEST_END_TAG_VALUE) < 0)
		return(-1);

	/* if we are asked to read events in, read them */
	if(filename) {
		int fd;
		
		if((fd=open(filename, O_RDONLY)) < 0) {
			fprintf(stderr, "glite_wll_perftest_init: Could not open event file %s: %s",
				filename, strerror(errno));
			return(-1);
		}
		
		if((nevents=read_events(&events)) < 0)
			return(-1);

		close(fd);

		/* reset event source */
		cur_event = cur_job = 0;

		njobs = n;
	}

	return(0);
}


static char *cur_jobid;

/**
 * This produces (njobs*nevents + 1) events, one event for each call.
 * For every nevents (one job) new jobid is generated and inserted into 
 * event. The last event is termination - usertag.
 */
int
glite_wll_perftest_produceEventString(char **event)
{
	char *e;

	assert(event != NULL);

	if(pthread_mutex_lock(&perftest_lock) < 0)
		abort();


	/* did we send all jobs? */
	if(cur_job >= njobs) {
		
		/* construct termination event */
		
		/* and reset counters back to beginning */
		cur_job = 0;
		cur_event = 0;

	} else {

		/* are we starting new job? */
		if(cur_event == 0) {
			edg_wlc_JobId jobid;
			
			/* generate new jobid */
			if(cur_jobid) free(cur_jobid);
			if(glite_wll_perftest_createJobId(dest_host,
							  dest_port,
							  test_user,
							  test_name,
							  cur_job,
							  &jobid) != 0) {
				fprintf(stderr, "produceEventString: error creating jobid\n");
				return(-1)
					}
			if((cur_jobid=edg_wlc_JobIdUnparse(jobid)) == NULL) {
				fprintf(stderr, "produceEventString: error unparsing jobid\n");
				return(-1);
			}
		}
		
		/* return current event with jobid filled in */
		if(trio_asprintf(&e, "DG.JOBID=\"%s\" %s", cur_jobid, events[cur_event]) < 0) {
			fprintf(stderr, "produceEventString: error generating event\n");
			return(-1);
		}
	}

	*event = e;

	/* advance to the next event */
	if(++cur_event >= nevents) {
		cur_job++;
		cur_event = 0;
	}

	if(pthread_mutex_unlock(&perftest_lock) < 0)
		abort();

	return(0);
}


int
glite_wll_perftest_consumeEvent(edg_wll_Event *event)
{
	int ret = 0;

	assert(event != NULL);

	if(pthread_mutex_lock(&perftest_lock) < 0)
		abort();

	gettimeofday(&endtime, NULL);

	/* check for the termination event */
	if((event->any.type == EDG_WLL_EVENT_USERTAG) &&
	   (strcmp(event->userTag.name, PERFTEST_END_TAG_NAME) == 0) &&
	   (strcmp(event->userTag.value, PERFTEST_END_TAG_VALUE) == 0)) {
		/* print the timestamp */
		fprintf(stderr, "PERFTEST_END_TIMESTAMP: %lu\n",
			1000000L*(unsigned long)endtime.tv_sec + (unsigned long)endtime.tv_usec); 
		ret = 1;
	}

	if(pthread_mutex_unlock(&perftest_lock) < 0)
		abort();

	return(ret);
}


int
glite_wll_perftest_consumeEventString(const char *event_string)
{
	int ret = 0;

	assert(event_string != NULL);

	if(pthread_mutex_lock(&perftest_lock) < 0)
		abort();

	gettimeofday(&endtime, NULL);

	/* check for the termination event */
	if(strstr(event_string, termination_string) != NULL) {
		/* print the timestamp */
		fprintf(stderr, "PERFTEST_END_TIMESTAMP: %lu\n",
			1000000L*(unsigned long)endtime.tv_sec + (unsigned long)endtime.tv_usec);
		ret = 1;
	}

	if(pthread_mutex_unlock(&perftest_lock) < 0)
		abort();

	return(ret);
}


int
glite_wll_perftest_consumeEventIlMsg(const char *msg, int len)
{
	int ret = 0;
	char *event;

	assert(msg != NULL);

	if(pthread_mutex_lock(&perftest_lock) < 0)
		abort();

	gettimeofday(&endtime, NULL);

	/* decode event */
	if(decode_il_msg(&event, msg+17) < 0) {
		fprintf(stderr, "PERFTEST: error decoding consumed event, aborting!\n");
		abort();
	}
		
	/* check for the termination event */
	if(strstr(event, termination_string) != NULL) {
		/* print the timestamp */
		fprintf(stderr, "PERFTEST_END_TIMESTAMP: %lu\n",
			1000000L*(unsigned long)endtime.tv_sec + (unsigned long)endtime.tv_usec);
		ret = 1;
	}

	if(pthread_mutex_unlock(&perftest_lock) < 0)
		abort();

	free(event);

	return(ret);
}


int 
glite_wll_perftest_createJobId(const char *bkserver,
			       int port,
			       const char *test_user,
			       const char *test_name,
			       int job_num,
			       edg_wlc_JobId *jobid)
{
	char unique[256];

	if(snprintf(unique, sizeof(unique), "%s_%s_%d", 
		    test_user, test_name, job_num) >= sizeof(unique)) 
		return(E2BIG);

	return(edg_wlc_JobIdRecreate(bkserver, port, unique, jobid));
}
