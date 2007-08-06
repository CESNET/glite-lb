#ident "$Header$"

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "interlogd.h"

extern char *file_prefix;

extern time_t cert_mtime, key_mtime;

void *
recover_thread(void *q)
{
	if(init_errors(0) < 0) {
		il_log(LOG_ERR, "Error initializing thread specific data, exiting!");
		pthread_exit(NULL);
	}

	while(1) {
		il_log(LOG_INFO, "Looking up event files...\n");
		if(event_store_init(file_prefix) < 0) {
			il_log(LOG_ERR, "recover_thread: %s\n", error_get_msg());
			exit(1);
		}
		if(event_store_recover_all() < 0) {
			il_log(LOG_ERR, "recover_thread: %s\n", error_get_msg());
			exit(1);
		}
		if(event_store_cleanup() < 0) {
			il_log(LOG_ERR, "recover_thread: %s\n", error_get_msg());
			exit(1);
		}
		il_log(LOG_INFO, "Checking for new certificate...\n");
		if(pthread_mutex_lock(&cred_handle_lock) < 0)
			abort();
		if (edg_wll_gss_watch_creds(cert_file, &cert_mtime) > 0) {
			edg_wll_GssCred new_cred_handle = NULL;
			int ret;

			ret = edg_wll_gss_acquire_cred_gsi(cert_file,key_file, 
				&new_cred_handle, NULL, NULL);
			if (new_cred_handle != NULL) {
				edg_wll_gss_release_cred(&cred_handle, NULL);
				cred_handle = new_cred_handle;
				il_log(LOG_INFO, "New certificate found and deployed.\n");
			}
		}
		if(pthread_mutex_unlock(&cred_handle_lock) < 0)
			abort();
		sleep(INPUT_TIMEOUT);
	}
}
