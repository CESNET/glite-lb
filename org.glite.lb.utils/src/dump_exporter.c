#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>


#include "glite/lbu/maildir.h"
#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/events_parse.h"


#define DUMP_FILE_STORE_PREFIX	"/tmp"
#define LB_MAILDIR_PATH			"/tmp/dumpinfo_lbmd"

#define KEYNAME_JOBID			"jobid  "
#define KEYNAME_FILE			"file   "
#define KEYNAME_JPPS			"jpps   "

typedef struct _buffer_t {
	char   *str;
	char   *eol;
	int		sz;
	int		use;
} buffer_t;

typedef struct _dump_storage_t {
	char   *job;
	char   *fname;
	int		fhnd;
} dump_storage_t;

static const char *optstr = "d:s:j:m:h";

static struct option opts[] = {
	{ "help",		0,	NULL,	'h'},
	{ "dump",		0,	NULL,	'd'},
	{ "store",		0,	NULL,	's'},
	{ "jpps",		0,	NULL,	'j'},
	{ "lbmaildir",	0,	NULL,	'm'},
	{ NULL,			0,	NULL,	0}
};

void usage(char *me)
{
	fprintf(stderr, "usage: %s [option]\n"
			"\t-h, --help              Shows this screen.\n"
			"\t-d, --dump <file>       Dump file location.\n"
			"\t-s, --store <prefix>    New dump files storage.\n"
			"\t-j, --jpps <host:port>  Target JPPS.\n"
			"\t-m, --lbmaildir <path>  LB maildir path.\n"
			, me);
}


static int read_line(int, buffer_t *, char **);
static dump_storage_t *dump_storage_find(dump_storage_t *, char *);
static dump_storage_t *dump_storage_add(dump_storage_t **, char *, char *, int);
static void dump_storage_free(dump_storage_t *);


int main(int argc, char **argv)
{
	edg_wll_Context		ctx;
	edg_wll_Event	   *ev = NULL;
	dump_storage_t	   *dstorage = NULL,
					   *st;
	buffer_t			buf;
	char			   *store_pref = DUMP_FILE_STORE_PREFIX,
					   *lb_maildir = LB_MAILDIR_PATH,
					   *jpps = NULL,
					   *name,
					   *fname,
					   *ln;
	int					fhnd,
						opt, ret,
						msg_format_sz;


	name = strrchr(argv[0], '/');
	if ( name ) name++; else name = argv[0];

	fname = NULL;
	while ( (opt = getopt_long(argc, argv, optstr, opts, NULL)) != EOF )
		switch ( opt ) {
		case 'd': fname = optarg; break;
		case 's': store_pref = optarg; break;
		case 'j': jpps = optarg; break;
		case 'm': lb_maildir = optarg; break;
		case 'h': usage(name); return 0;
		case '?': usage(name); return 1;
		}

	msg_format_sz = sizeof(KEYNAME_JOBID) + 1 +
				sizeof(KEYNAME_FILE) + 1 +
				(jpps? sizeof(KEYNAME_JPPS) + 1: 0);

	if ( fname ) {
		if ( (fhnd = open(fname, O_RDONLY)) < 0 ) {
			perror("Opening input file");
			exit(1);
		}
	} else fhnd = 0;

	if ( glite_lbu_MaildirInit(lb_maildir) ) {
		perror(lbm_errdesc);
		exit(1);
	}


#ifndef cleanup
#	define cleanup(r)	{ ret = (r); goto cleanup_lbl; }
#endif

	ret = 0;
	memset(&buf, 0, sizeof(buf));
	edg_wll_InitContext(&ctx);
	while ( 1 ) {
		int		rl_ret,
				written,
				lnsz, ct;
		char   *jobid,
			   *unique;

		if ( (rl_ret = read_line(fhnd, &buf, &ln)) < 0 ) {
			perror("reading input file");
			cleanup(1);
		}
		if ( !ln ) break;

		if (*ln == 0) continue;

		if ( edg_wll_ParseEvent(ctx, ln, &ev) != 0 ) {
			cleanup(1);
		}
		if ( !(jobid = edg_wlc_JobIdUnparse(ev->any.jobId)) ) {
			perror("Can't unparse jobid from event");
			cleanup(1);
		}
		if ( !(unique = edg_wlc_JobIdGetUnique(ev->any.jobId)) ) {
			perror("Can't unparse jobid from event");
			cleanup(1);
		}

		if ( !(st = dump_storage_find(dstorage, jobid)) ) {
			int	fd, i;
			char	fname[PATH_MAX];
			struct	timeval  tv;

			i = 0;
			while ( 1 ) {
				if ( ++i > 10 ) {
					errno = ECANCELED;
					perror("Can't create dump file - max tries limit reached ");
					cleanup(1);
				}
				gettimeofday(&tv, NULL);
				snprintf(fname, PATH_MAX, "%s/%s.%ld_%ld", store_pref, unique, tv.tv_sec, tv.tv_usec);
				if ( (fd = open(fname, O_CREAT|O_EXCL|O_RDWR, 00640)) < 0 ) {
					if ( errno == EEXIST ) { usleep(1000); continue; }
					perror(fname);
					cleanup(1);
				}
				break;
			}

			if ( !(st = dump_storage_add(&dstorage, jobid, fname, fd)) ) {
				perror("Can't record dump informations");
				cleanup(1);
			}
		}
		free(jobid);
		free(unique);

		lnsz = strlen(ln);
		ln[lnsz++] = '\n';
		written = 0;
		while ( written < lnsz ) {
			if ( (ct = write(st->fhnd, ln+written, lnsz-written)) < 0 ) {
				if ( errno == EINTR ) { errno = 0; continue; }
				perror(fname);
				cleanup(1);
			}
			written += lnsz;
		}


		if ( !rl_ret ) break;
		edg_wll_FreeEvent(ev); ev = NULL;
	}

	/* store info in lb_maildir */
	for ( st = dstorage; st && st->job; st++ ) {
		char *msg;
		if ( !(msg = malloc(msg_format_sz + strlen(st->fname) + strlen(st->job))) ) {
			perror("allocating message");
			cleanup(1);
		}
		if ( jpps ) 
			/* XXX: used to be 5x %s here, God knows why ... */
			sprintf(msg, "%s%s\n%s%s%s%s",
					KEYNAME_JOBID, st->job, KEYNAME_FILE, st->fname, KEYNAME_JPPS, jpps);
		else 
			sprintf(msg, "%s%s\n%s%s",
					KEYNAME_JOBID, st->job, KEYNAME_FILE, st->fname);
		if ( glite_lbu_MaildirStoreMsg(lb_maildir, "localhost", msg) < 0 ) {
			perror(lbm_errdesc);
			exit(1);
		}
		free(msg);
	}

cleanup_lbl:
	edg_wll_FreeContext(ctx);
	if ( ev ) edg_wll_FreeEvent(ev);
	for ( st = dstorage; st && st->job; st++ ) if ( st->fhnd > 0 ) close(st->fhnd);
	dump_storage_free(dstorage);
	close(fhnd);
	free(buf.str);

	return (ret);
}


static dump_storage_t *dump_storage_find(dump_storage_t *st, char *job)
{
	while ( st && st->job ) {
		if ( !strcmp(job, st->job) ) break;
		st++;
	}
	if ( st && st->job ) return st;
	return NULL;
}

static dump_storage_t *dump_storage_add(dump_storage_t **st, char *job, char *fname, int fhnd)
{
	dump_storage_t *tmp;
	int				ct;

	for ( ct = 0, tmp = *st; tmp && tmp->job; ct++, tmp++ ) ;
	if ( ct ) tmp = realloc(*st, (ct+2)*sizeof(*tmp));
	else tmp = calloc(2, sizeof(*tmp));
	if ( !tmp ) return NULL;

	*st = tmp;
	while ( tmp && tmp->job ) tmp++;
	
	if ( !(tmp->job = strdup(job)) ) return NULL;
	if ( !(tmp->fname = strdup(fname)) ) { free(tmp->job); return NULL; }
	tmp->fhnd = fhnd;
	(tmp+1)->job = NULL;

	return tmp;
}

static void dump_storage_free(dump_storage_t *st)
{
	dump_storage_t *tmp;
	for ( tmp = st; tmp && tmp->job; tmp++ ) {
		free(tmp->job);
		free(tmp->fname);
	}
	free(st);
}

/* Reads line from file and returns pointer to that.
 * Returned string is not mem allocated. It only points to the buffer!
 *
 * Buffer must be given - it is handled (allocated etc.) fully in the
 * function. It has to be freed outside.
 *
 * returns: -1	on error
 * 			0	eof ('ln' could points to the last line in file)
 * 			1	line successfuly read
 */
static int read_line(int fhnd, buffer_t *buf, char **ln)
{
	int		ct, toread;


	if ( buf->eol ) {
		int tmp = buf->eol - buf->str + 1;
		
		if ( tmp < buf->use ) {
			char *aux;
			if ( (aux = memchr(buf->eol+1, '\n', buf->use-tmp)) ) {
				*ln = buf->eol+1;
				*aux = 0;
				buf->eol = aux;
				return 1;
			}
		}
		memmove(buf->str, buf->eol+1, buf->use - tmp);
		buf->eol = NULL;
		buf->use -= tmp;
	}

	do {
		if ( buf->use == buf->sz ) {
			char *tmp = realloc(buf->str, buf->sz+BUFSIZ);
			if ( !tmp ) return -1;
			buf->str = tmp;
			buf->sz += BUFSIZ;
		}

		toread = buf->sz - buf->use;
		if ( (ct = read(fhnd, buf->str+buf->use, toread)) < 0 ) {
			if ( errno == EINTR ) continue;
			return -1;
		}
		if ( ct == 0 ) break;
		buf->eol = memchr(buf->str+buf->use, '\n', ct);
		buf->use += ct;
	} while ( ct == toread && !buf->eol );

	*ln = buf->use? buf->str: NULL;
	if ( buf->eol ) {
		*buf->eol = 0;
		return 1;
	}

	return 0;
}
