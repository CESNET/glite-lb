#ifndef __EDG_WORKLOAD_LOGGING_COMMON_DGSSL_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_DGSSL_H__
/**
 * \file Client/dgssl.h
 * \brief Auxiliary SSL functions for L&B 
 */

#ident "$Header$"

/* openssl headers */
#include <globus_config.h>
#include "glite/wmsutils/thirdparty/globus_ssl_utils/sslutils.h"

#include <ssl.h>
#include <err.h>
#include <rand.h>
#include <bio.h>
#include <pem.h>
#include <x509.h>
#if SSLEAY_VERSION_NUMBER >= 0x0090581fL
#include <x509v3.h>
#endif

/**
 * Initialize SSL and GSI contexts and set SSL parameters for authentication.
 * \param verify IN: specifies SSL authentication mode to be used for peer 
 * verification. (SSL_VERIFY_NONE, SSL_VERIFY_PEER, 
 * SSL_VERIFY_FAIL_IF_NO_PEER_CERT, SSL_VERIFY_CLIENT_ONCE) -- see also ssl.h
 * \param callback IN: if nonzero the standard GSI authentication callback will
 * be called on each certificate in a chain in order to verify the chain
 * consists only of valid GSI proxy certs. If the parametr is zero only the
 * default SSL callback will be used.
 * \param p_cert_file IN: file containg the caller's cert.
 * \param p_key_file IN: file containg the caller's key. If at least one of
 * these parameters is NULL, the default proxy is examined and used (if
 * exists), otherwise the default long-term credential will be used.  \param
 * ask_passwd IN: if nonzero, the standard SSL prompt is used whenever an
 * encrypted keyfile is encountered. If the paramater zero, no password will be
 * prompted for even if the key is encrypted.
 * \param no_auth IN: if nonzero, only context necessary for authentication of
 * the peer will be set. No caller's credential is read.
 * \retval pointer to an opaque structure containg the initialized context, if
 * any error occurs, the context will contain only those basic parameters
 * necessary for authentication of the peer
 */
proxy_cred_desc *edg_wll_ssl_init(int verify, int callback, char *p_cert_file,char *p_key_file,
                  int ask_passwd, int noauth);

/**
 * frees cred_handle returned by edg_wll_ssl_init()
 * \param cred_handle IN: pointer returned by edg_wll_ssl_init()
 */
int edg_wll_ssl_free(proxy_cred_desc *cred_handle);

/**
 * Establish SSL context with server.
 * Called by the clients when they tryies to do SSL handshake with the server.
 * This includes authentication of the server and optionaly the client, and
 * establishing SSL security parameters.
 *
 * \param cred_handle IN: pointer returned by edg_wll_ssl_init()
 * \param hostname IN: hostname to connect to
 * \param port IN: port to connect to
 * \param timeout INOUT: if NULL, blocking SSL I/O is done, otherwise the
 * I/O and SSL handshake will take at most the specified time. Remaining
 * time is returned.
 * \param sslp OUT: on success, returned pointer to SSL connection
 * \return one of EDG_WLL_SSL_*
 */
int edg_wll_ssl_connect(proxy_cred_desc *cred_handle,char const *hostname,int port,
        struct timeval *timeout,SSL **sslp);

/**
 * Establish SSL context with client.
 * Called by the server when trying to do a SSL handshake with the client. A
 * network connection must be established before calling this function. Use
 * timeout set in context.
 * \param ctx IN: used to obtain timeout
 * \param cred_handle IN: pointer returned by edg_wll_ssl_init()
 * \param sock IN: specification of the network connection
 * \retval pointer to SSL connection on success
 * \retval NULL on error
 */
SSL *edg_wll_ssl_accept(proxy_cred_desc *cred_handle,int sock, struct timeval *timeout);

/**
 * Try to send SSL close alert on socket (reject the client in orderly way).
 * \param cred_handle IN: pointer returned by edg_wll_ssl_init(), to get SSL_CTX
 * \param sock IN: specification of the network connection
 */

void edg_wll_ssl_reject(proxy_cred_desc *cred_handle,int sock);

/**
 * Close SSL connection, possibly saving open sessions, and destroy the SSL object.
 * Called by anyone who wishes to close the connection.
 * \param ssl IN: object identifying the SSL connection
 * \param timeout INOUT: max time allowed for operation, remaining time
 * on return
 * \retval one of EDG_WLL_SSL_*
 */
int edg_wll_ssl_close_timeout(SSL *ssl, struct timeval *timeout);

/**
 * Close SSL connection, call edg_wll_ssl_close_timeout() with hard-wired
 * timeout 120 secs.
 * \param ssl IN: object identifying the SSL connection
 * \retval one of EDG_WLL_SSL_*
 */
int edg_wll_ssl_close(SSL *ssl);

/**
 * Return subject name of the caller as specified in the context.
 * \param cred_handle IN: pointer returned by edg_wll_ssl_init()
 * \param my_subject_name OUT: contains the subject name. The caller must free
 * this variable when no needed.
 */
void edg_wll_ssl_get_my_subject(proxy_cred_desc *cred_handle, char **my_subject_name);
void edg_wll_ssl_get_my_subject_base(proxy_cred_desc *cred_handle, char **my_subject_name);

/** no SSL errors */
#define EDG_WLL_SSL_OK		0
/** SSL specific error. Call ERR_* functions for details. */
#define EDG_WLL_SSL_ERROR_SSL		-1
/** Timeout */
#define EDG_WLL_SSL_ERROR_TIMEOUT	-2
/** EOF occured */
#define EDG_WLL_SSL_ERROR_EOF		-3
/** System error. See errno. */
#define EDG_WLL_SSL_ERROR_ERRNO		-4
/** Resolver error. See h_errno. */
#define EDG_WLL_SSL_ERROR_HERRNO	-5

/**
 * Read from SSL connection.
 * Needn't read entire buffer. Timeout is applicable only for non-blocking
 * connections (created with non-NULL timeout param of edg_wll_ssl_connect()).
 * \param ssl IN: connection to work with
 * \param buf OUT: buffer
 * \param bufsize IN: max size to read
 * \param timeout INOUT: max time allowed for operation, remaining time
 * on return
 * \retval bytes read (>0) on success
 * \retval one of EDG_WLL_SSL_* (<0) on error
 */

int edg_wll_ssl_read(SSL *ssl,void *buf,size_t bufsize,struct timeval *timeout);

/**
 * Write to SSL connection.
 * Needn't write entire buffer. Timeout is applicable only for non-blocking
 * connections (created with non-NULL timeout param of edg_wll_ssl_connect()).
 * \param ssl IN: connection to work with
 * \param buf IN: buffer
 * \param bufsize IN: max size to write
 * \param timeout INOUT: max time allowed for operation, remaining time
 * on return
 * \retval bytes written (>0) on success
 * \retval one of EDG_WLL_SSL_* (<0) on error
 */

int edg_wll_ssl_write(SSL *ssl,const void *buf,size_t bufsize,struct timeval *timeout);

/**
 * Read specified amount of data from SSL connection.
 * Attempts to call edg_wll_ssl_read() untill the entire request is satisfied
 * (or EOF reached, or times out)
 *
 * \param ssl IN: connection to work with
 * \param buf OUT: buffer
 * \param bufsize IN: max size to read
 * \param timeout INOUT: max time allowed for operation, remaining time
 * on return
 * \param total OUT: bytes actually read
 * \return one of EDG_WLL_SSL_*
 */

int edg_wll_ssl_read_full(SSL *ssl,void *buf,size_t bufsize,struct timeval *timeout,size_t *total);

/**
 * Write specified amount of data to SSL connection.
 * Attempts to call edg_wll_ssl_write() untill the entire request is satisfied
 * (or times out).
 *
 * \param ssl IN: connection to work with
 * \param buf IN: buffer
 * \param bufsize IN: max size to read
 * \param timeout INOUT: max time allowed for operation, remaining time
 * on return
 * \param total OUT: bytes actually written
 * \return one of EDG_WLL_SSL_*
 */

int edg_wll_ssl_write_full(SSL *ssl,const void *buf,size_t bufsize,struct timeval *timeout,size_t *total);

void edg_wll_ssl_set_noauth(proxy_cred_desc* cred_handle);

/**
 * Check modification times of key and cert files.
 */
int edg_wll_ssl_watch_creds(const char *,const char *,time_t *,time_t *);

#endif /* __EDG_WORKLOAD_LOGGING_COMMON_DGSSL_H__ */

