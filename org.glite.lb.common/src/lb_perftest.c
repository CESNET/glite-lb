#ident "$Header$"

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

#include "glite/lbu/trio.h"

#include "lb_perftest.h"
#include "il_msg.h"

typedef struct {
  char *event;
  int   job_index;
  int   need_parent;
  int   need_seed;
} job_events_t;

static pthread_mutex_t perftest_lock = PTHREAD_MUTEX_INITIALIZER;
static struct timeval endtime;
static char *termination_string;

static int njobs = 0; 
static int nsubjobs = 0; 
static job_events_t *events;
static int nevents;
static char **jobids;

static int cur_event = 0;
static int cur_job = 0;
static int cur_group = 0;
static char *test_user;
static char *test_name;
static char *dest_host;
static int dest_port;
static int group_size = 1;

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
read_line(int fd, char **buff, int *maxsize)
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
                                return i;
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
read_events(int fd) 
{
	void *tmp;
	char *line = NULL;
	int linesize;
	size_t i = 0, max = EVENTS_BUFSIZ;
	char **subjobids = NULL;
	int len;

	if ((events = calloc(max, sizeof(job_events_t))) == NULL) goto nomem;
	if ((subjobids = calloc(max, sizeof(char*))) == NULL) goto nomem;
	nsubjobs = 0;

	while ((len = read_line(fd, &line, &linesize)) > 0) {
		char *d, *p, *q;

		if (i + 1 >= max) {
			max <<= 1;
			if ((tmp = realloc(events, max * sizeof(job_events_t))) == NULL) goto nomem;
			events = (job_events_t*)tmp;
		}
		/* if ((e[i] = strdup(line)) == NULL) goto nomem; */
		/* allocate memory for event, reserve enough space for variable keys */
		if((d = malloc(len + 1)) == NULL) 
			goto nomem;
		events[i].event = d;
		events[i].job_index = 0; 
                events[i].need_parent = 0;
		events[i].need_seed = 0;

		/* copy key by key, look for JOBID, PARENT, SEED and NSUBJOBS */
		for(p = q = line; q < line + len ; p = q) {

			/* look for next DG. */
			/* XXX - this works only for "nice" events with no spurious DG. */
			q = strstr(p+3, "DG.");
			if(q) {
			} else {
				/* no further key, copy till the end */
				q = line + len;
			}
			if(strncmp(p, "DG.JOBID", 8) == 0) {
				/* omit, will be filled in later */
				/* if we know jobid, we can fill in job_index */
				char *v;

				v = strchr(p+10, '"');
				if(v) {
					int m;

					*v = 0;
					/* lookup this jobid in subjobids */
					for(m = 0; (m < nsubjobs + 1) && subjobids[m]; m++) {
						if(strcmp(p+10, subjobids[m]) == 0) 
							break;
					}
					if(m >= nsubjobs + 1) m = 0;
					if(subjobids[m] == NULL) {
						subjobids[m] = strdup(p+10);
					}
					events[i].job_index = m;
					*v = '"';
				}
			} else if(strncmp(p, "DG.REGJOB.PARENT", 16) == 0) {
				/* omit, will be filled in later */
				events[i].need_parent = 1;
				if(*(q-1) == '\n') {
					*d++ = '\n';
				}
			} else if(strncmp(p, "DG.REGJOB.SEED", 14) == 0) {
				/* omit for now */
				events[i].need_seed = 1;
				if(*(q-1) == '\n') {
					*d++ = '\n';
				}
			} else if(strncmp(p, "DG.REGJOB.NSUBJOBS", 18) == 0) {
				int val;

				strncpy(d, p, q - p);
				d += q - p;
				/* scan the number of subjobs */
				val = atoi(p + 20);
				if(nsubjobs == 0 && val > 0) {
					nsubjobs = val;
					subjobids = realloc(subjobids, (nsubjobs+1)*sizeof(*subjobids));
					if(subjobids == NULL) goto nomem;
					memset(subjobids, 0, (nsubjobs+1)*sizeof(*subjobids));
				}
			} else {
				/* some other key */
				strncpy(d, p, q - p);
				d += q - p;
			}
		}
		*d = 0;
		free(line); line = NULL;
		i++;
	}

	return i;

nomem:
	/* XXX: should free all events[i].event */
	if(events) free(events);
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
	edg_wll_Context ctx;

	edg_wll_InitContext(&ctx);

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
			dest_port = GLITE_JOBID_DEFAULT_PORT;
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
		
		if((nevents=read_events(fd)) < 0)
			return(-1);

		close(fd);

		fprintf(stderr, "PERFTEST_JOB_SIZE=%d\n", nevents);
		fprintf(stderr, "PERFTEST_NUM_JOBS=%d\n", njobs);
		fprintf(stderr, "PERFTEST_NUM_SUBJOBS=%d\n", nsubjobs);
	}

	/* we suppose nsubjobs was filled in by read_events() */


	/* generate jobids[0..njobs-1, 0..nsubjobs] */
	jobids = calloc(njobs*(nsubjobs + 1), sizeof(char*));
	if(jobids == NULL) {
		fprintf(stderr, "glite_wll_perftest_init: not enough memory for job id's\n");
		return(-1);
	}
        while (--n >= 0) {
		glite_jobid_t jobid;
		glite_jobid_t *subjobid;
		int i;

		if(glite_wll_perftest_createJobId(dest_host,
						  dest_port,
						  test_user,
						  test_name,
						  n,
						  &jobid) != 0) {
			fprintf(stderr, "produceJobId: error creating jobid\n");
			return(-1);
		}
		if((jobids[n*(nsubjobs+1)]=edg_wlc_JobIdUnparse(jobid)) == NULL) {
			fprintf(stderr, "produceJobId: error unparsing jobid\n");
			return(-1);
		}

		/* generate subjob ids */
		if(nsubjobs > 0) {
			if(edg_wll_GenerateSubjobIds(ctx, jobid, nsubjobs, test_name,
						     &subjobid) < 0) {
				fprintf(stderr, "produceJobId: error generating subjob ids\n");
				return -1;
			}
		}
		for(i = 1; i <= nsubjobs; i++) {
			if((jobids[n*(nsubjobs+1) + i] = edg_wlc_JobIdUnparse(subjobid[i-1])) == NULL) {
				fprintf(stderr, "produceJobId: error unparsing jobid\n");
				return(-1);
			}
			glite_jobid_free(subjobid[i-1]);
		}
		glite_jobid_free(jobid);
	}

			
	return(0);
}


/** 
 * This produces njobs*(nsubjobs+1) jobids, one for each call.
 *
 * WARNING: do not mix calls to this function with calls to produceEventString!
 */
char *
glite_wll_perftest_produceJobId()
{
	char *jobid;
	static int n_cur_subjob = 0;

	if(pthread_mutex_lock(&perftest_lock) < 0)
		abort();

	/* is there anything to send? */
	if(cur_job < 0) {
		if(pthread_mutex_unlock(&perftest_lock) < 0)
			abort();
		return(NULL);
	}

	jobid = jobids[(nsubjobs+1)*cur_job + n_cur_subjob];

	if(++n_cur_subjob > nsubjobs) {
		n_cur_subjob = 0;
		cur_job++;
	}
	if(cur_job >= njobs) 
		cur_job = -1;
		
	if(pthread_mutex_unlock(&perftest_lock) < 0)
		abort();

	return(jobid);
}


/**
 * This produces (njobs*nsubjobs*nevents + 1) events, one event for each call.
 * For every nevents (one subjob) new jobid is inserted into 
 * event. The last event is termination - usertag.
 */
int
glite_wll_perftest_produceEventString(char **event, char **jobid)
{
	static int first = 1;
	char *e;
	int len, cur_subjob, jobi;

	assert(event != NULL);

	if(pthread_mutex_lock(&perftest_lock) < 0)
		abort();

	/* is there anything to send? */
	if(cur_event < 0) {
		if(pthread_mutex_unlock(&perftest_lock) < 0)
			abort();
		return(0);
	}

	/* use index to get current subjob */
	cur_subjob = events[cur_event].job_index;

	/* current job index */
	jobi = cur_group*group_size + cur_job;

	/* did we send all events? */
	if(jobi >= njobs) {
		
		/* construct termination event */
		if((len=trio_asprintf(&e, EDG_WLL_FORMAT_COMMON EDG_WLL_FORMAT_USER EDG_WLL_FORMAT_USERTAG "\n",
				      "now", /* date */
				      "localhost", /* host */
				      "highest", /* level */
				      0, /* priority */
				      "UserInterface", /* source */
				      "me again", /* source instance */
				      "UserTag", /* event */
				      jobids[0], /* jobid */
				      "UI=999980:NS=9999999980:WM=999980:BH=9999999980:JSS=999980:LM=999980:LRMS=999980:APP=999980", /* sequence */
				      "me", /* user */
				      PERFTEST_END_TAG_NAME,
				      PERFTEST_END_TAG_VALUE)) < 0) {
			fprintf(stderr, "produceEventString: error creating termination event\n");
			if(pthread_mutex_unlock(&perftest_lock) < 0)
				abort();
			return(-1);
		}
		*jobid = jobids[0];

		/* and refuse to produce more */
		cur_job = -1;
		cur_event = -1;
		cur_subjob = -1;

		

	} else {
		char *parent = NULL;
		char *seed = NULL;

		/* is this the first event? */
		if(first) {
			struct timeval now;
			
			gettimeofday(&now, NULL);
			fprintf(stderr, "PERFTEST_BEGIN_TIMESTAMP=%lu.%06lu\n",
				(unsigned long)now.tv_sec,(unsigned long)now.tv_usec);
			first = 0;
		}
		
		/* fill parent if needed */
		if(events[cur_event].need_parent) {
			trio_asprintf(&parent, "DG.REGJOB.PARENT=\"%s\"", 
				      (nsubjobs > 0) ? jobids[jobi*(nsubjobs+1)] : "");
		}
		/* fill seed if needed */
		if(events[cur_event].need_seed) {
			trio_asprintf(&seed, "DG.REGJOB.SEED=\"%s\"",
				      test_name);
		}
		/* return current event with jobid filled in */
		if((len=trio_asprintf(&e, "DG.JOBID=\"%s\" %s %s %s", 
				      jobids[jobi*(nsubjobs+1) + cur_subjob], 
				      parent ? parent : "",
				      seed ? seed : "",
				      /* nsubjobs, */
				      events[cur_event].event)) < 0) {
			fprintf(stderr, "produceEventString: error generating event\n");
			if(pthread_mutex_unlock(&perftest_lock) < 0)
				abort();
			return(-1);
		}
		if(parent) free(parent);
		if(seed) free(seed);
		*jobid = jobids[jobi*(nsubjobs+1) + cur_subjob];

		/* advance to the next job and/or event */
		if(++cur_job % group_size == 0) {
			if(++cur_event >= nevents) {
				cur_event = 0;
				cur_group++;
			}
			cur_job = 0;
		}

	}

	*event = e;


	if(pthread_mutex_unlock(&perftest_lock) < 0)
		abort();

	return(len);
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
	int ret;

	ret = snprintf(unique, sizeof(unique), "%s_%s_%d",test_user, test_name, job_num);
	if (ret < 0) return errno;
	if ((unsigned int)ret >= sizeof(unique)) return(E2BIG);

	return(edg_wlc_JobIdRecreate(bkserver, port, str2md5base64(unique), jobid));
}
