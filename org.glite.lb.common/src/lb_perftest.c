#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>

#include "lb_perftest.h"
#include "glite/lb/producer.h"
#include "glite/lb/trio.h"
#include "il_msg.h"

#ident "$Header$"

static pthread_mutex_t perftest_lock = PTHREAD_MUTEX_INITIALIZER;
static struct timeval endtime;
static char *termination_string;

int 
glite_wll_perftest_init()
{
	if(trio_asprintf(&termination_string, EDG_WLL_FORMAT_USERTAG,
		    PERFTEST_END_TAG_NAME, PERFTEST_END_TAG_VALUE) < 0)
		return(-1);

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
