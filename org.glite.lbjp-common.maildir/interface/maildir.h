#ifndef GLITE_LBU_MAILDIR_H
#define GLITE_LBU_MAILDIR_H

#include <time.h>

/*
 * Functions for reading and writing messages via
 * maildir protocol.
 * Used when registering job to the JP, i.e.
 */

enum {
	LBMD_TRANS_OK,
	LBMD_TRANS_FAILED,
	LBMD_TRANS_FAILED_RETRY,
};
	
extern char lbm_errdesc[];

extern int glite_lbu_MaildirInit(const char *);
extern int glite_lbu_MaildirStoreMsg(const char *, const char *, const char *);
extern int glite_lbu_MaildirRetryTransStart(const char *, time_t, time_t, char **, char **);
extern int glite_lbu_MaildirTransStart(const char *, char **, char **);
extern int glite_lbu_MaildirTransEnd(const char *, char *, int);

#endif /* GLITE_LBU_MAILDIR_H */
