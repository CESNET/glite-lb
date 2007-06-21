#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include "glite/lb/lb_maildir.h"

#define DEFAULT_ROOT	"/tmp/lb_maildir"

enum {
	LBMD_DIR_TMP = 0,
	LBMD_DIR_NEW,
	LBMD_DIR_WORK,
	LBMD_DIR_POST,
	LBMD_DIR_UNDELIVERABLE
};

static const char *dirs[] = { "tmp", "new", "work", "post", "undeliverable" };


#define MAX_ERR_LEN		1024
char lbm_errdesc[MAX_ERR_LEN];


static int check_mkdir(const char *dir)
{
	struct stat sbuf;

	if ( stat(dir, &sbuf) ) {
		if ( errno == ENOENT ) {
			if ( mkdir(dir, S_IRWXU) ) return 1;
			if ( stat(dir, &sbuf) )  return 1;
		}
		else return 1;
	}

	if (!S_ISDIR(sbuf.st_mode)) return 1;

	if (access(dir, R_OK | W_OK)) return 1;

	return 0;
}


int edg_wll_MaildirInit(
	const char		   *dir)
{
	const char *root = dir? : DEFAULT_ROOT;
	char		dirname[PATH_MAX];
	int			i;

	if ( check_mkdir(root) ) {
		snprintf(lbm_errdesc, MAX_ERR_LEN, "%s: %s\n", root, strerror(errno));
		return 1;
	}
	
	for ( i = 0; i < sizeof(dirs)/sizeof((dirs)[0]); i++ ) {
		snprintf(dirname, PATH_MAX, "%s/%s", root, dirs[i]);
		if ( check_mkdir(dirname) ) {
			snprintf(lbm_errdesc, MAX_ERR_LEN, "%s: %s\n", dirname, strerror(errno));
			return 1;
		}
	}

	return 0;
}


int edg_wll_MaildirStoreMsg(
	const char	*root,
	const char	*srvname,
	const char	*msg)
{
	char	fname[PATH_MAX],
		newfname[PATH_MAX];
	int	fhnd,
		written,
		msgsz,
		ct, i;
	struct  timeval  tv;


	if ( !root ) root = DEFAULT_ROOT;

	errno = 0;
	i = 0;
	while ( 1 ) {
		if ( ++i > 10 ) {
			errno = ECANCELED;
			snprintf(lbm_errdesc, MAX_ERR_LEN, "Maximum tries limit reached with unsuccessful file creation");
			return -1;
		}
		gettimeofday(&tv, NULL);
		snprintf(fname, PATH_MAX, "%s/%s/%ld_%ld.%s", root, dirs[LBMD_DIR_TMP], tv.tv_sec, tv.tv_usec, srvname);
		if ( (fhnd = open(fname, O_CREAT|O_EXCL|O_WRONLY, 00600)) < 0 ) {
			if ( errno == EEXIST ) { usleep(1000); continue; }
			snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't create file %s", fname);
			return -1;
		}
		break;
	}

	msgsz = strlen(msg);
	written = 0;
	while ( written < msgsz ) {
		if ( (ct = write(fhnd, msg+written, msgsz-written)) < 0 ) {
			if ( errno == EINTR ) { errno = 0; continue; }
			snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't write into file %s", fname);
			return -1;
		}
		written += msgsz;
	}
	if ( fsync(fhnd) ) {
		snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't fsync file %s", fname);
		return -1;
	}
	if ( close(fhnd) ) {
		snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't close file %s", fname);
		return -1;
	}
	snprintf(newfname, PATH_MAX, "%s/%s/%s", root, dirs[LBMD_DIR_NEW], strrchr(fname, '/')+1);
	if ( link(fname, newfname) ) {
		snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't link new file %s", newfname);
		return -1;
	}

	return 0;
}


int edg_wll_MaildirTransEnd(
	const char		   *root,
	char			   *fname,
	int					tstate)
{
	char		workfname[PATH_MAX],
				newfname[PATH_MAX],
				origfname[PATH_MAX];
	struct stat	st;


	if ( !root ) root = DEFAULT_ROOT;

	snprintf(workfname, PATH_MAX, "%s/%s/%s", root, dirs[LBMD_DIR_WORK], fname);
	unlink(workfname);

	snprintf(origfname, PATH_MAX, "%s/%s/%s", root, dirs[LBMD_DIR_TMP], fname);
	if ( tstate == LBMD_TRANS_OK ) {
		unlink(origfname);
		return 0;
	}

	if ( tstate == LBMD_TRANS_FAILED ) return 0;

	if ( stat(origfname, &st) ) {
		snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't stat file '%s'", origfname);
		return -1;
	}

	snprintf(newfname, PATH_MAX, "%s/%s/%s", root, dirs[LBMD_DIR_POST], fname);
	if ( link(origfname, newfname) ) {
		snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't link new file %s", newfname);
		return -1;
	}

	return 0;
}


int edg_wll_MaildirRetryTransStart(
	const char	*root,
	time_t		retry,
	time_t		remove,
	char		**msg,
	char		**fname)
{
	static DIR	*dir = NULL;
	struct dirent	*ent;
	time_t		tlimit_retry, tlimit_remove;
	struct stat	st;
	char		newfname[PATH_MAX],
			oldfname[PATH_MAX],
			*buf = NULL;
	int		fhnd,
			toread, ct,
			bufsz, bufuse;


	if ( !root ) root = DEFAULT_ROOT;

	if ( !dir ) {
		char	dirname[PATH_MAX];
		snprintf(dirname, PATH_MAX, "%s/%s", root, dirs[LBMD_DIR_POST]);
		if ( !(dir = opendir(dirname)) ) {
			snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't open directory '%s'", root);
			goto err;
		}
	}

	tlimit_retry = time(NULL) - retry;
	tlimit_remove = time(NULL) - remove;
	do {
		errno = 0;
		if ( !(ent = readdir(dir)) ) {
			if ( errno == EBADF ) {
				snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't read directory '%s'", root);
				dir = NULL;
				goto err;
			} else {
				closedir(dir);
				dir = NULL;
				return 0;
			}
		}
		if ( ent->d_name[0] == '.' ) continue;

		snprintf(oldfname, PATH_MAX, "%s/%s/%s", root, dirs[LBMD_DIR_POST], ent->d_name);
		snprintf(newfname, PATH_MAX, "%s/%s/%s", root, dirs[LBMD_DIR_WORK], ent->d_name);

		if ( stat(oldfname, &st) < 0 ) {
			snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't stat file '%s'", oldfname);
			goto err;
		}

		/* if we cannot deliver the file for 'remove' time limit, */
		/* it is moved to undeliverable folder and forgotten      */
		if ( st.st_mtime < tlimit_remove ) {
			snprintf(newfname, PATH_MAX, "%s/%s/%s",
				 root, dirs[LBMD_DIR_UNDELIVERABLE], ent->d_name);
		}
		/* try to deliver file every 'retry' seconds */
		else if ( st.st_ctime > tlimit_retry ) continue;


		if ( rename(oldfname, newfname) ) {
			if ( errno == ENOENT ) {
				/* maybe some other instance moved this file away... */
				continue;
			} else {
				snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't move file '%s'", oldfname);
				goto err;
			}
		} else {
			if (st.st_mtime < tlimit_remove) {
				/* we have moved undeliverable file to undeliverable folder */
				/* no other action needed */
				snprintf(oldfname, PATH_MAX, "%s/%s/%s", root, dirs[LBMD_DIR_TMP], ent->d_name);
				unlink(oldfname);
				continue;
			} else {
				/* we have found and moved the file  to work folder */
				/* going to process it */
				break;
			}
		}
	} while ( 1 );

	if ( (fhnd = open(newfname, O_RDONLY)) < 0 ) {
		snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't open file '%s'", newfname);
		goto err;
	}

	bufuse = bufsz = toread = ct = 0;
	do {
		errno = 0;
		if ( bufuse == bufsz ) {
			char *tmp = realloc(buf, bufsz+BUFSIZ);
			if ( !tmp ) goto err;
			buf = tmp;
			bufsz += BUFSIZ;
		}
		toread = bufsz - bufuse;
		if ( (ct = read(fhnd, buf+bufuse, toread)) < 0 ) {
			if ( errno == EINTR ) continue;
			snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't read file '%s'", newfname);
			goto err;
		}
		if ( ct == 0 ) break;
		bufuse += ct;
	} while ( ct == toread );
	close(fhnd);

	if ( !(*fname = strdup(ent->d_name)) ) goto err;
	buf[bufuse] = 0;
	*msg = buf;
	return 1;


err:
	if ( buf ) free(buf);

	return -1;
}


int edg_wll_MaildirTransStart(
	const char		   *root,
	char              **msg,
	char			  **fname)
{
	static DIR	   *dir = NULL;
	struct dirent  *ent;
	char			newfname[PATH_MAX],
					oldfname[PATH_MAX],
				   *buf = NULL;
	int				fhnd,
					toread, ct,
					bufsz, bufuse;


	if ( !root ) root = DEFAULT_ROOT;

	if ( !dir ) {
		char	dirname[PATH_MAX];
		snprintf(dirname, PATH_MAX, "%s/%s", root, dirs[LBMD_DIR_NEW]);
		if ( !(dir = opendir(dirname)) ) {
			snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't open directory '%s'", root);
			goto err;
		}
	}

	do {
		errno = 0;
		if ( !(ent = readdir(dir)) ) {
			if ( errno == EBADF ) {
				snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't read directory '%s'", root);
				dir = NULL;
				goto err;
			} else {
				closedir(dir);
				dir = NULL;
				return 0;
			}
		}
		if ( ent->d_name[0] == '.' ) continue;
		snprintf(newfname, PATH_MAX, "%s/%s/%s", root, dirs[LBMD_DIR_WORK], ent->d_name);
		snprintf(oldfname, PATH_MAX, "%s/%s/%s", root, dirs[LBMD_DIR_NEW], ent->d_name);
		if ( rename(oldfname, newfname) ) {
			if ( errno == ENOENT ) {
				/* maybe some other instance moved this file away... */
				continue;
			} else {
				snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't move file '%s'", oldfname);
				goto err;
			}
		} else {
			/* we have found and moved the file with which we will work now */
			break;
		}
	} while ( 1 );

	if ( (fhnd = open(newfname, O_RDONLY)) < 0 ) {
		snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't open file '%s'", newfname);
		goto err;
	}

	bufuse = bufsz = toread = ct = 0;
	do {
		errno = 0;
		if ( bufuse == bufsz ) {
			char *tmp = realloc(buf, bufsz+BUFSIZ);
			if ( !tmp ) goto err;
			buf = tmp;
			bufsz += BUFSIZ;
		}
		toread = bufsz - bufuse;
		if ( (ct = read(fhnd, buf+bufuse, toread)) < 0 ) {
			if ( errno == EINTR ) continue;
			snprintf(lbm_errdesc, MAX_ERR_LEN, "Can't read file '%s'", newfname);
			goto err;
		}
		if ( ct == 0 ) break;
		bufuse += ct;
	} while ( ct == toread );
	close(fhnd);

	if ( !(*fname = strdup(ent->d_name)) ) goto err;
	buf[bufuse] = 0;
	*msg = buf;
	return 1;


err:
	if ( buf ) free(buf);

	return -1;
}
