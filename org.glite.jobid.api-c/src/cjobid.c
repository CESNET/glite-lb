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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#include "cjobid.h"
#include "strmd5.h"

struct _edg_wlc_JobId {
    char		*id;	/* unique job identification */
    /* additional information */
    char		*BShost;/* bookkeeping server hostname */
    unsigned int	BSport;	/* bookkeeping server port */
    char		*info;	/* additional information (after ? in URI) */
};

int glite_jobid_create(const char *bkserver, int port, glite_jobid_t *jobId)
{
    return glite_jobid_recreate(bkserver, port, NULL, jobId);
}


int glite_jobid_recreate(const char* bkserver, int port, const char *unique, glite_jobid_t *jobId)
{
    glite_jobid_t out;
    char hostname[200]; /* used to hold string for encrypt */
    struct timeval tv;
    int skip;
    char* portbeg;
    int rndaddr = 0;

    struct hostent* he;

    if (!bkserver)
        return EINVAL;

    if (unique == NULL) {
	gettimeofday(&tv, NULL);
	srandom(tv.tv_usec);
	gethostname(hostname, 100);
	he = gethostbyname(hostname);
	if (he) rndaddr = *((uint32_t *)he->h_addr_list[0]);
	else {
#define RAND8 (rand() & 0xFF)
#define RAND16 (RAND8 << 8 | RAND8)
		rndaddr = RAND16 << 16 | RAND16;
	}

    	skip = strlen(hostname);
    	skip += sprintf(hostname + skip, "-IP:0x%x-pid:%d-rnd:%d-time:%d:%d",
		    rndaddr,
		    getpid(), (int)random(), (int)tv.tv_sec, (int)tv.tv_usec);
    }

    *jobId = NULL;
    out = (glite_jobid_t) malloc (sizeof(*out));
    if (!out)
	return ENOMEM;

    memset(out, 0, sizeof(*out));

    /* check if it begins with prefix */
    /* unsupported */
    /* FIXME: fill in PROTO_PREFIX if missing */
    if (strncmp(bkserver, GLITE_JOBID_PROTO_PREFIX, sizeof(GLITE_JOBID_PROTO_PREFIX)-1) == 0)
        return EINVAL;

    out->BShost = strdup(bkserver);
    portbeg = strrchr(out->BShost, ':');
    if (portbeg) {
	*portbeg = 0;
        /* try to get port number */
	if (port == 0)
	    port = atoi(portbeg + 1);
    }

    if (port == 0)
        port = GLITE_JOBID_DEFAULT_PORT;

    out->BSport = port;

    out->id = (unique) ? strdup(unique) : str2md5base64(hostname);
    //printf("Encrypt: %s\nBASE64 %s\n", hostname, out->id);

    if (!out->id || !out->BShost) {
	glite_jobid_free(out);
	return ENOMEM;
    }

    *jobId = out;
    return 0;
}


int glite_jobid_dup(glite_jobid_const_t in, glite_jobid_t *out)
{
    glite_jobid_t jid;
    *out = NULL;
    if (in == NULL)
	return 0;

    jid = malloc(sizeof(*jid));
    if (!jid)
	return ENOMEM;

    memset(jid, 0,sizeof(*jid));
    jid->BShost = strdup(in->BShost);
    jid->id = strdup(in->id);
    if (in->info)
	jid->info = strdup(in->info);

    if (jid->BShost == NULL || jid->id == NULL) {
	glite_jobid_free(jid);
	return ENOMEM;
    }

    jid->BSport = in->BSport;
    *out = jid;
    return 0;
}


// XXX
// use recreate
// parse name, port, unique
int glite_jobid_parse(const char *idString, glite_jobid_t *jobId)
{
    char *pom, *pom1, *pom2;
    glite_jobid_t out;

    *jobId = NULL;

    out = (glite_jobid_t) malloc (sizeof(*out));
    if (out == NULL )
	return ENOMEM;

    memset(out,0,sizeof(*out));

    if (strncmp(idString, GLITE_JOBID_PROTO_PREFIX, sizeof(GLITE_JOBID_PROTO_PREFIX) - 1)) {
	out->BShost  = (char *) NULL;
	out->BSport  = 0;

	free(out);
	return EINVAL;
    }

    pom = strdup(idString + sizeof(GLITE_JOBID_PROTO_PREFIX) - 1);
    pom1 = strchr(pom, '/');

    if (!pom1) { free(pom); free(out); return EINVAL; }
    pom1[0] = '\0';

    pom2 = strrchr(pom, ':');
    if (pom2 && strchr(pom2,']')) pom2 = NULL;

    if ( pom2 ) {
	pom[pom2-pom]     = '\0';
	out->BShost  = strdup(pom);
	pom[pom1-pom]     = '\0';
	out->BSport  = (unsigned int) strtoul(pom2 + 1,NULL,10);
    } else {
	pom[pom1-pom]     = '\0';
	out->BShost  = strdup(pom);
	out->BSport  = GLITE_JOBID_DEFAULT_PORT;
    }

    /* XXX: localhost not supported in jobid 
    if (!strncmp(out->BShost,"localhost",9) {
	free(pom);
	free(out->BShost);
	free(out);
	return EINVAL;
    }
    */

    /* additional info from URI */
    pom2 = strchr(pom1+1,'?');
    if (pom2) {
	*pom2 = 0;
	out->info = strdup(pom2+1);
    }

    /* extract the unique part */
    out->id = strdup(pom1+1);

    for (pom1 = out->BShost; *pom1; pom1++)
	if (isspace(*pom1)) break;

    for (pom2 = out->id; *pom2; pom2++)
	if (isspace(*pom2)) break;

    if (*pom1 || *pom2) {
	    free(pom);
	    glite_jobid_free(out);
	    return EINVAL;
    }

    free(pom);
    *jobId = out;
    return 0;
}


void glite_jobid_free(glite_jobid_t job)
{
    if (job) {
	free(job->id);
	free(job->BShost);
	free(job->info);
	free(job);
    }
}


char* glite_jobid_unparse(glite_jobid_const_t jobid)
{
    char *out;

    if (!jobid)
	return NULL;

    if (asprintf(&out, GLITE_JOBID_PROTO_PREFIX"%s:%d/%s%s%s",
	     jobid->BShost,
	     jobid->BSport? jobid->BSport : GLITE_JOBID_DEFAULT_PORT,
	     jobid->id,
	     (jobid->info ? "?" : ""),
	     (jobid->info ? jobid->info : "")) == -1)
	return NULL;

    return out;
}


char* glite_jobid_getServer(glite_jobid_const_t jobid)
{
    char *bs = NULL;

    if (!jobid)
	return NULL;

    if (asprintf(&bs, "%s:%u", jobid->BShost,
	     jobid->BSport ? jobid->BSport : GLITE_JOBID_DEFAULT_PORT) == -1)
	return NULL;

    return bs;
}


void glite_jobid_getServerParts(glite_jobid_const_t jobid, char **srvName, unsigned int *srvPort)
{
    if (jobid) {
	*srvName = strdup(jobid->BShost);
	*srvPort = jobid->BSport ? jobid->BSport : GLITE_JOBID_DEFAULT_PORT;
    }
}


char* glite_jobid_getUnique(glite_jobid_const_t jobid)
{
    return jobid ? strdup(jobid->id) : NULL;
}


void glite_jobid_getServerParts_internal(glite_jobid_const_t jobid, char **srvName, unsigned int *srvPort)
{
    if (jobid) {
	*srvName = jobid->BShost;
	*srvPort = jobid->BSport ? jobid->BSport : GLITE_JOBID_DEFAULT_PORT;
    }
}


char* glite_jobid_getUnique_internal(glite_jobid_const_t jobid)
{
    return jobid ? jobid->id : NULL;
}
