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
#include <unistd.h>
#include <ctype.h>

#include "lb_perftest.h"
#include "glite/lb/producer.h"
#include "trio.h"
#include "il_msg.h"

#ident "$Header$"

static pthread_mutex_t perftest_lock = PTHREAD_MUTEX_INITIALIZER;
static struct timeval endtime;
static char *termination_string;
static char **events; /* in fact it is *events[] */
static int nevents;
static int njobs = 0;
static int cur_event = 0;
static int cur_job = 0;
static char *test_user;
static char *test_name;
static char *dest_host;
static int dest_port;

#define EVENTS_BUFSIZ 16
#define BUFFSZ        1024


/*
 * strnstr - look only in first n characters
 * (taken from NSPR)
 */
static
char *
_strnstr(const char *big, const char *little, size_t max)
{
    size_t ll;

    if( ((const char *)0 == big) || ((const char *)0 == little) ) return (char *)0;
    if( ((char)0 == *big) || ((char)0 == *little) ) return (char *)0;

    ll = strlen(little);
    if( ll > (size_t)max ) return (char *)0;
    max -= ll;
    max++;

    for( ; max && *big; big++, max-- )
        if( *little == *big )
            if( 0 == strncmp(big, little, ll) )
                return (char *)big;

    return (char *)0;
}


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
                        else {
				/* i points to the last nonspace, nonnull character in line */
				/* add \n - we need it later */
				(*buff)[++i] = '\n'; /* now we are at \0 or at space */
				if(i >= *maxsize) {
					(*maxsize) += 1;
					if((tmp = realloc(*buff, *maxsize)) == NULL) return -1;
					*buff = (char *)tmp;
				}
				(*buff)[++i] = '\0';
                                return 1;
			}
                }

                i++;
        }

}


/* 
 * read events from the stream
 */
static 
int 
read_events(int fd, char ***evts /* *(*evts)[] */) 
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

	*evts = e;
	return i;

nomem:
	if(e) free(e);
	if(line) free(line);
	fprintf(stderr, "read_events: insufficient memory\n");
	return -1;
}


int 
glite_wll_perftest_init(const char *host,
			const char *user,
			const char *testname,
			const char *filename, 
			int n)
{

	if(trio_asprintf(&termination_string, EDG_WLL_FORMAT_USERTAG,
		    PERFTEST_END_TAG_NAME, PERFTEST_END_TAG_VALUE) < 0)
		return(-1);

	/* set parameters */
	if(user) 
		test_user = strdup(user);
	else {
		test_user = getenv("PERFTEST_USER");
		if(test_user == NULL) 
			test_user = "anonymous";
	}
	if(testname)
		test_name = strdup(testname);
	else {
		test_name = getenv("PERFTEST_NAME");
		if(test_name == NULL)
			test_name = "unspecified";
	}
	if(host == NULL) {
		host = getenv("PERFTEST_HOST");
		if(host == NULL)
			host = "localhost";
	}
	{
		char *p;

		p = strchr(host, ':');
  		if(p) 
			*p = 0;
		dest_host = strdup(host);
		if(p) {
			*p++ = ':';
			dest_port = atoi(p);
		} else 
			dest_port = GLITE_WMSC_JOBID_DEFAULT_PORT;
	}

	/* reset event source */
	cur_event = cur_job = 0;
	njobs = n;

	/* if we are asked to read events in, read them */
	if(filename) {
		int fd;
		
		if((fd=open(filename, O_RDONLY)) < 0) {
			fprintf(stderr, "glite_wll_perftest_init: Could not open event file %s: %s",
				filename, strerror(errno));
			return(-1);
		}
		
		if((nevents=read_events(fd, &events)) < 0)
			return(-1);

		close(fd);


		fprintf(stderr, "PERFTEST_JOB_SIZE=%d\n", nevents);
		fprintf(stderr, "PERFTEST_NUM_JOBS=%d\n", njobs);
	}

	return(0);
}


/** 
 * This produces njobs+1 jobids, one for each call.
 *
 * WARNING: do not mix calls to this function with calls to produceEventString!
 */
char *
glite_wll_perftest_produceJobId()
{
	edg_wlc_JobId jobid;
	char *jobids;

	if(pthread_mutex_lock(&perftest_lock) < 0)
		abort();

	/* is there anything to send? */
	if(cur_job < 0) {
		if(pthread_mutex_unlock(&perftest_lock) < 0)
			abort();
		return(NULL);
	}

	if(glite_wll_perftest_createJobId(dest_host,
					  dest_port,
					  test_user,
					  test_name,
					  cur_job,
					  &jobid) != 0) {
		fprintf(stderr, "produceJobId: error creating jobid\n");
		if(pthread_mutex_unlock(&perftest_lock) < 0)
			abort();
		return(NULL);
	}
	if((jobids=edg_wlc_JobIdUnparse(jobid)) == NULL) {
		fprintf(stderr, "produceJobId: error unparsing jobid\n");
		if(pthread_mutex_unlock(&perftest_lock) < 0)
			abort();
		return(NULL);
	}

	if(cur_job++ >= njobs) 
		cur_job = -1;
		
	return(jobids);
}


/**
 * This produces (njobs*nevents + 1) events, one event for each call.
 * For every nevents (one job) new jobid is generated and inserted into 
 * event. The last event is termination - usertag.
 */
const char *
glite_wll_perftest_produceEventString(char **event)
{
	static char *cur_jobid = NULL;
	char *e;

	assert(event != NULL);

	if(pthread_mutex_lock(&perftest_lock) < 0)
		abort();

	/* is there anything to send? */
	if(cur_job < 0) {
		if(pthread_mutex_unlock(&perftest_lock) < 0)
			abort();
		return(NULL);
	}

	/* did we send all jobs? */
	if(cur_job >= njobs) {
		
		/* construct termination event */
		if(trio_asprintf(&e, EDG_WLL_FORMAT_COMMON EDG_WLL_FORMAT_USERTAG "\n",
				 "now", /* date */
				 "localhost", /* host */
				 "highest", /* level */
				 0, /* priority */
				 "UserInterface", /* source */
				 "me again", /* source instance */
				 "UserTag", /* event */
				 cur_jobid, /* jobid */
				 "last", /* sequence */
				 PERFTEST_END_TAG_NAME,
				 PERFTEST_END_TAG_VALUE) < 0) {
			fprintf(stderr, "produceEventString: error creating termination event\n");
			if(pthread_mutex_unlock(&perftest_lock) < 0)
				abort();
			return(NULL);
		}

		/* and refuse to produce more */
		cur_job = -1;
		cur_event = -1;

	} else {

		/* are we starting new job? */
		if(cur_event == 0) {
			edg_wlc_JobId jobid;
			
			/* is this the first event? */
			if(cur_jobid) {
				free(cur_jobid);
			} else {
				struct timeval now;

				gettimeofday(&now, NULL);
				fprintf(stderr, "PERFTEST_BEGIN_TIMESTAMP=%lu.%06lu\n",
					(unsigned long)now.tv_sec,(unsigned long)now.tv_usec);

			}

			/* generate new jobid */
			if(glite_wll_perftest_createJobId(dest_host,
							  dest_port,
							  test_user,
							  test_name,
							  cur_job,
							  &jobid) != 0) {
				fprintf(stderr, "produceEventString: error creating jobid\n");
				if(pthread_mutex_unlock(&perftest_lock) < 0)
					abort();
				return(NULL);
			}
			if((cur_jobid=edg_wlc_JobIdUnparse(jobid)) == NULL) {
				fprintf(stderr, "produceEventString: error unparsing jobid\n");
				if(pthread_mutex_unlock(&perftest_lock) < 0)
					abort();
				return(NULL);
			}
		}
		
		/* return current event with jobid filled in */
		if(trio_asprintf(&e, "DG.JOBID=\"%s\" %s", cur_jobid, events[cur_event]) < 0) {
			fprintf(stderr, "produceEventString: error generating event\n");
			if(pthread_mutex_unlock(&perftest_lock) < 0)
				abort();
			return(NULL);
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

	return(cur_jobid);
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
		fprintf(stderr, "PERFTEST_END_TIMESTAMP=%lu.%06lu\n",
			(unsigned long)endtime.tv_sec,(unsigned long)endtime.tv_usec);
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
	/* if it is not in the first 1k chars, it is not there */
	if(_strnstr(event_string, termination_string, 1024) != NULL) {
		/* print the timestamp */
		fprintf(stderr, "PERFTEST_END_TIMESTAMP=%lu.%06lu\n",
			(unsigned long)endtime.tv_sec,(unsigned long)endtime.tv_usec);
		ret = 1;
	}

	if(pthread_mutex_unlock(&perftest_lock) < 0)
		abort();

	return(ret);
}


int
glite_wll_perftest_consumeEventIlMsg(const char *msg)
{
	int ret = 0;
	il_octet_string_t event;

	assert(msg != NULL);

	if(pthread_mutex_lock(&perftest_lock) < 0)
		abort();

	gettimeofday(&endtime, NULL);

	/* decode event */
	if(decode_il_msg(&event, msg) < 0) {
		fprintf(stderr, "PERFTEST: error decoding consumed event, aborting!\n");
		abort();
	}
		
	/* check for the termination event */
	if(_strnstr(event.data, termination_string, 1024) != NULL) {
		/* print the timestamp */
		fprintf(stderr, "PERFTEST_END_TIMESTAMP=%lu.%06lu\n",
			endtime.tv_sec, endtime.tv_usec);
		ret = 1;
	}

	if(pthread_mutex_unlock(&perftest_lock) < 0)
		abort();

	free(event.data);

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

	return(edg_wlc_JobIdRecreate(bkserver, port, str2md5base64(unique), jobid));
}
