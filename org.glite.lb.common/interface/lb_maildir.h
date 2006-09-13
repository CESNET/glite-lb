#ifndef LB_MAILDIR
#define LB_MAILDIR

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

extern int edg_wll_MaildirInit(const char *);
extern int edg_wll_MaildirStoreMsg(const char *, const char *, const char *);
extern int edg_wll_MaildirRetryTransStart(const char *, time_t, time_t, char **, char **);
extern int edg_wll_MaildirTransStart(const char *, char **, char **);
extern int edg_wll_MaildirTransEnd(const char *, char *, int);

#endif
