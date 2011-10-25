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
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "interlogd.h"

extern char *file_prefix;

extern time_t cert_mtime, key_mtime;

void *
recover_thread(void *q)
{
	if(init_errors(0) < 0) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, 
				 "Error initializing thread specific data, exiting!");
		pthread_exit(NULL);
	}

	while(1) {
		glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
				 "Looking up event files.");
		if(event_store_init(file_prefix) < 0) {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, 
					 "recover_thread: %s", error_get_msg());
			exit(1);
		}
		if(event_store_recover_all() < 0) {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, 
					 "recover_thread: %s", error_get_msg());
			exit(1);
		}
		if(event_store_cleanup() < 0) {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, 
					 "recover_thread: %s", error_get_msg());
			exit(1);
		}
		glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_DEBUG, "Checking for new certificate.");
		int ret;
		ret = edg_wll_gss_watch_creds(cert_file, &cert_mtime);
		if (ret > 0) {
			edg_wll_GssCred new_creds = NULL;

			int int_ret;
			int_ret = edg_wll_gss_acquire_cred_gsi(cert_file,key_file, 
				&new_creds, NULL);
			if (new_creds != NULL) {
				if(pthread_mutex_lock(&cred_handle_lock) < 0)
					abort();
				/* if no one is using the old credentials, release them */
				if(cred_handle && cred_handle->counter == 0) {
					edg_wll_gss_release_cred(&cred_handle->creds, NULL);
					free(cred_handle);
					glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_DEBUG, 
							 "  freed old credentials");
				}
				cred_handle = malloc(sizeof(*cred_handle));
				if(cred_handle == NULL) {
					glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, 
							 "Failed to allocate structure for credentials.");
					exit(EXIT_FAILURE);
				}
				cred_handle->creds = new_creds;
				cred_handle->counter = 0;
				if(pthread_mutex_unlock(&cred_handle_lock) < 0)
					abort();
				glite_common_log(LOG_CATEGORY_SECURITY, LOG_PRIORITY_INFO, 
						 "New certificate %s found and deployed.",
						 new_creds->name);
			}
		}
		else if ( ret < 0)
			glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"edg_wll_gss_watch_creds failed, unable to access credetials\n");
		
#ifndef LB_PERF
		sleep(RECOVER_TIMEOUT);
#else
		sleep(2);
#endif
	}
}
