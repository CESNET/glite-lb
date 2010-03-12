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

#ifndef GLITE_LBU_MAILDIR_H
#define GLITE_LBU_MAILDIR_H

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
