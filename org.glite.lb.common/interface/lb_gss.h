#ifndef __EDG_WORKLOAD_LOGGING_COMMON_LB_GSS_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_LB_GSS_H__

#ident "$Header$"

#include <gssapi.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  EDG_WLL_GSS_OK		=  0,  /* no GSS errors */
  EDG_WLL_GSS_ERROR_GSS		= -1,  /* GSS specific error, call edg_wll_get_gss_error() for details */
  EDG_WLL_GSS_ERROR_TIMEOUT	= -2,  /* Timeout */
  EDG_WLL_GSS_ERROR_EOF		= -3,  /* EOF occured */
  EDG_WLL_GSS_ERROR_ERRNO	= -4,  /* System error. See errno */
  EDG_WLL_GSS_ERROR_HERRNO	= -5   /* Resolver error. See h_errno */
};

typedef struct _edg_wll_GssConnection {
  gss_ctx_id_t context;
  int sock;
  char *buffer;
  size_t bufsize;
} edg_wll_GssConnection;

typedef struct _edg_wll_GssStatus {
  OM_uint32 major_status;
  OM_uint32 minor_status;
} edg_wll_GssStatus;

/* XXX Support anonymous connections. Are we able/required to support
 * anonymous servers as well. */

int
edg_wll_gss_acquire_cred_gsi(char *cert_file,
		             char *key_file,
		             gss_cred_id_t *cred,
		             char **name,
			     edg_wll_GssStatus* gss_code);

int 
edg_wll_gss_connect(gss_cred_id_t cred,
		    char const *hostname,
		    int port,
		    struct timeval *timeout,
		    edg_wll_GssConnection *connection,
		    edg_wll_GssStatus* gss_code);

int
edg_wll_gss_accept(gss_cred_id_t cred,
		   int sock,
		   struct timeval *timeout,
		   edg_wll_GssConnection *connection,
		   edg_wll_GssStatus* gss_code);

int
edg_wll_gss_read(edg_wll_GssConnection *connection,
		 void *buf,
	         size_t bufsize,
		 struct timeval *timeout,
		 edg_wll_GssStatus* gss_code);

int
edg_wll_gss_write(edg_wll_GssConnection *connection,
		  const void *buf,
		  size_t bufsize,
		  struct timeval *timeout,
		  edg_wll_GssStatus* gss_code);

int
edg_wll_gss_read_full(edg_wll_GssConnection *connection,
		      void *buf,
		      size_t bufsize,
		      struct timeval *timeout,
		      size_t *total,
		      edg_wll_GssStatus* gss_code);

int
edg_wll_gss_write_full(edg_wll_GssConnection *connection,
		       const void *buf,
		       size_t bufsize,
		       struct timeval *timeout,
		       size_t *total,
		       edg_wll_GssStatus* gss_code);

int
edg_wll_gss_watch_creds(const char * proxy_file,
			time_t * proxy_mtime);

int
edg_wll_gss_get_error(edg_wll_GssStatus* gss_code,
		      const char *prefix,
		      char **errmsg);

int
edg_wll_gss_close(edg_wll_GssConnection *connection,
		  struct timeval *timeout);

int 
edg_wll_gss_reject(int sock);

int
edg_wll_gss_oid_equal(const gss_OID a,
		      const gss_OID b);

/*
int
edg_wll_gss_get_name(gss_cred_id_t cred, char **name);
*/

#ifdef __cplusplus
} 
#endif
	
#endif /* __EDG_WORKLOAD_LOGGING_COMMON_LB_GSS_H__ */
