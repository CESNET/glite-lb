#ident "$Header$"

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "interlogd.h"

extern char *file_prefix;

void *
recover_thread(void *q)
{
	time_t cert_mtime = 0, key_mtime = 0;

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
		if (edg_wll_ssl_watch_creds(key_file,
					    cert_file,
					    &key_mtime,
					    &cert_mtime) > 0) {
			void * new_cred_handle = edg_wll_ssl_init(SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 
								  0,
								  cert_file,
								  key_file,
								  0,
								  0);
			if (new_cred_handle) {
				edg_wll_ssl_free(cred_handle);
				cred_handle = new_cred_handle;
				il_log(LOG_INFO, "New certificate found and deployed.\n");
			}
		}
		if(pthread_mutex_unlock(&cred_handle_lock) < 0)
			abort();
		sleep(INPUT_TIMEOUT);
	}
}
