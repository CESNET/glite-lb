#ident "$Header$"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ares.h>

#include "globus_config.h"
#include "glite/wms/thirdparty/globus_ssl_utils/sslutils.h"

#include "dgssl.h"

/* This two functions are not in globus sslutils.h */
void proxy_verify_ctx_release(proxy_verify_ctx_desc *pvxd);
void proxy_verify_ctx_init(proxy_verify_ctx_desc *pvxd);

#include "globus_gss_assist.h"

#define tv_sub(a,b) {\
	(a).tv_usec -= (b).tv_usec;\
	(a).tv_sec -= (b).tv_sec;\
	if ((a).tv_usec < 0) {\
		(a).tv_sec--;\
		(a).tv_usec += 1000000;\
	}\
}

static int decrement_timeout(struct timeval *timeout, struct timeval before, struct timeval after)
{
        (*timeout).tv_sec = (*timeout).tv_sec - (after.tv_sec - before.tv_sec);
        (*timeout).tv_usec = (*timeout).tv_usec - (after.tv_usec - before.tv_usec);
        while ( (*timeout).tv_usec < 0) {
                (*timeout).tv_sec--;
                (*timeout).tv_usec += 1000000;
        }
        if ( ((*timeout).tv_sec < 0) || (((*timeout).tv_sec == 0) && ((*timeout).tv_usec == 0)) ) return(1);
        else return(0);
}


static int handle_ssl_error(int sock,int err,struct timeval *to);

static unsigned char dh512_p[]={
        0xDA,0x58,0x3C,0x16,0xD9,0x85,0x22,0x89,0xD0,0xE4,0xAF,0x75,
        0x6F,0x4C,0xCA,0x92,0xDD,0x4B,0xE5,0x33,0xB8,0x04,0xFB,0x0F,
        0xED,0x94,0xEF,0x9C,0x8A,0x44,0x03,0xED,0x57,0x46,0x50,0xD3,
        0x69,0x99,0xDB,0x29,0xD7,0x76,0x27,0x6B,0xA2,0xD3,0xD4,0x12,
        0xE2,0x18,0xF4,0xDD,0x1E,0x08,0x4C,0xF6,0xD8,0x00,0x3E,0x7C,
        0x47,0x74,0xE8,0x33,
        };
static unsigned char dh512_g[]={
        0x02,
        };

static DH *get_dh512(void)
        {
        DH *dh=NULL;

        if ((dh=DH_new()) == NULL) return(NULL);
        dh->p=BN_bin2bn(dh512_p,sizeof(dh512_p),NULL);
        dh->g=BN_bin2bn(dh512_g,sizeof(dh512_g),NULL);
        if ((dh->p == NULL) || (dh->g == NULL))
                return(NULL);
        return(dh);
        }

struct asyn_result {
	struct hostent *ent;
	int		err;
};

/* ares callback handler for ares_gethostbyname()       */
static void callback_handler(void *arg, int status, struct hostent *h) {
	struct asyn_result *arp = (struct asyn_result *) arg;

	switch (status) {
	   case ARES_SUCCESS:
		if (h && h->h_addr_list[0]) {
			arp->ent->h_addr_list =
				(char **) malloc(2 * sizeof(char *));
			if (arp->ent->h_addr_list == NULL) {
				arp->err = NETDB_INTERNAL;
				break;
			}
			arp->ent->h_addr_list[0] =
				malloc(sizeof(struct in_addr));
			if (arp->ent->h_addr_list[0] == NULL) {
				free(arp->ent->h_addr_list);
				arp->err = NETDB_INTERNAL;
				break;
			}
			memcpy(arp->ent->h_addr_list[0], h->h_addr_list[0],
				sizeof(struct in_addr));
			arp->ent->h_addr_list[1] = NULL;
			arp->err = NETDB_SUCCESS;
		} else {
			arp->err = NO_DATA;
		}
		break;
	    case ARES_EBADNAME:
	    case ARES_ENOTFOUND:
		arp->err = HOST_NOT_FOUND;
		break;
	    case ARES_ENOTIMP:
		arp->err = NO_RECOVERY;
		break;
	    case ARES_ENOMEM:
	    case ARES_EDESTRUCTION:
	    default:
		arp->err = NETDB_INTERNAL;
		break;
	}
}


static void free_hostent(struct hostent *h){
        int i;

        if (h) {
                if (h->h_name) free(h->h_name);
		if (h->h_aliases) {
			for (i=0; h->h_aliases[i]; i++) free(h->h_aliases[i]);
			free(h->h_aliases);
		}
                if (h->h_addr_list) {
                        for (i=0; h->h_addr_list[i]; i++) free(h->h_addr_list[i]);
                        free(h->h_addr_list);
                }
                free(h);
        }
}

static int asyn_gethostbyname(char **addrOut, char const *name, struct timeval *timeout) {
	struct asyn_result ar;
	ares_channel channel;
	int nfds;
	fd_set readers, writers;
	struct timeval tv, *tvp;
	struct timeval start_time,check_time;


/* start timer */
	gettimeofday(&start_time,0);

/* ares init */
	if ( ares_init(&channel) != ARES_SUCCESS ) return(NETDB_INTERNAL);
	ar.ent = (struct hostent *) calloc (sizeof(*ar.ent),1);

/* query DNS server asynchronously */
	ares_gethostbyname(channel, name, AF_INET, callback_handler,
				(void *) &ar);

/* wait for result */
	while (1) {
		FD_ZERO(&readers);
		FD_ZERO(&writers);
		nfds = ares_fds(channel, &readers, &writers);
		if (nfds == 0)
			break;

		gettimeofday(&check_time,0);
		if (decrement_timeout(timeout, start_time, check_time)) {
			ares_destroy(channel);
			free_hostent(ar.ent);
			return(TRY_AGAIN);
		}
		start_time = check_time;

		tvp = ares_timeout(channel, timeout, &tv);

		switch ( select(nfds, &readers, &writers, NULL, tvp) ) {
			case -1: if (errno != EINTR) {
					ares_destroy(channel);
				  	free_hostent(ar.ent);
				  	return NETDB_INTERNAL;
				 } else
					continue;
			case 0: 
				FD_ZERO(&readers);
				FD_ZERO(&writers);
				/* fallthrough */
			default: ares_process(channel, &readers, &writers);
		}
	}


	ares_destroy(channel);

	if (ar.err == NETDB_SUCCESS) {
		*addrOut = malloc(sizeof(struct in_addr));
		memcpy(*addrOut,ar.ent->h_addr_list[0], sizeof(struct in_addr));
		free_hostent(ar.ent);
	}
	return(ar.err);
}



void edg_wll_ssl_set_noauth(proxy_cred_desc* cred_handle)
{ DH *dh=NULL;

  SSL_CTX_set_cipher_list(cred_handle->gs_ctx, 
     "ADH:RSA:HIGH:MEDIUM:LOW:EXP:+eNULL:+aNULL");
  dh=get_dh512();
  SSL_CTX_set_tmp_dh(cred_handle->gs_ctx,dh);
  DH_free(dh);
}

proxy_cred_desc *edg_wll_ssl_init(int verify, int callback, char *p_cert_file,char *p_key_file,
                  int ask_passwd, int noauth)
{
  proxy_cred_desc * cred_handle = NULL;
  char *certdir=NULL;
  int (*pw_cb)() = NULL;
  int load_err = 0;

  if (ask_passwd == 0)
     pw_cb = proxy_password_callback_no_prompt;

  cred_handle = proxy_cred_desc_new();
  proxy_get_filenames(cred_handle,1,NULL,&certdir,NULL,NULL,NULL);
  if (!noauth) {
     if ((p_cert_file!=NULL) && (p_key_file!=NULL)) {
	load_err = proxy_load_user_cert(cred_handle,p_cert_file,NULL,NULL);
	if (load_err == 0)
	   load_err = proxy_load_user_key(cred_handle,p_key_file,pw_cb,NULL);
        if (load_err == 0) {
             if (proxy_check_proxy_name(cred_handle->ucert)>0) {
                  cred_handle->type =CRED_TYPE_PROXY;
                if (cred_handle->cert_chain == NULL)
                     cred_handle->cert_chain = sk_X509_new_null();
                proxy_load_user_proxy(cred_handle->cert_chain,p_cert_file,NULL);
                /*
                if (cred_handle->cert_chain)
                     for (int i=0;i<sk_X509_num(cred_handle->cert_chain);i++) 
                         X509_STORE_add_cert(cred_handle->gs_ctx->cert_store,
                                     sk_X509_value(cred_handle->cert_chain,i));
                */
             } else cred_handle->type = CRED_TYPE_PERMANENT;
        } 
     } 
     if (load_err == 0)
	proxy_init_cred(cred_handle,pw_cb,NULL);
  }

  if (   (cred_handle->gs_ctx != NULL
      && !SSL_CTX_check_private_key(cred_handle->gs_ctx))
      || noauth == 1
      || load_err) {
    if (cred_handle->ucert != NULL) {
      X509_free(cred_handle->ucert);
      cred_handle->ucert = NULL;
    }
    if (cred_handle->upkey != NULL) {
      EVP_PKEY_free(cred_handle->upkey);
      cred_handle->upkey = NULL;
    }
    if (cred_handle->gs_ctx)
      SSL_CTX_free(cred_handle->gs_ctx);
    cred_handle->gs_ctx=SSL_CTX_new(SSLv3_method());
    SSL_CTX_set_options(cred_handle->gs_ctx,0);
    SSL_CTX_sess_set_cache_size(cred_handle->gs_ctx,5);
    SSL_CTX_load_verify_locations(cred_handle->gs_ctx,NULL,certdir);
  }

  if (cred_handle->gs_ctx != NULL) {
      SSL_CTX_set_verify(cred_handle->gs_ctx,verify,
            (callback==0)?NULL:proxy_verify_callback);
      SSL_CTX_set_purpose(cred_handle->gs_ctx,X509_PURPOSE_ANY);
      SSL_CTX_set_session_id_context(cred_handle->gs_ctx, "DG_BKSERVER",
	    strlen("DG_BKSERVER"));
      if (noauth == 1)
	 edg_wll_ssl_set_noauth(cred_handle);
  }

  free(certdir);
  return(cred_handle);
}

void edg_wll_ssl_get_my_subject(proxy_cred_desc *cred_handle, char **my_subject_name)
{
  if ((my_subject_name!=NULL) && (cred_handle->ucert!=NULL))
     *my_subject_name=X509_NAME_oneline(X509_get_subject_name(
          cred_handle->ucert), NULL, 0);
}

void edg_wll_ssl_get_my_subject_base(proxy_cred_desc *cred_handle, char **my_subject_name)
{
  X509_NAME *base;
  if ((my_subject_name!=NULL) && (cred_handle->ucert!=NULL)) {
	X509_NAME	*s = X509_get_subject_name(cred_handle->ucert);
	
	base = X509_NAME_dup(s);
	proxy_get_base_name(base);
     	*my_subject_name=strdup(X509_NAME_oneline(base,NULL, 0));
	X509_NAME_free(base);
  }
}

int edg_wll_ssl_free(proxy_cred_desc * cred_handle)
{ return(proxy_cred_desc_free(cred_handle));
}

int edg_wll_ssl_connect(proxy_cred_desc *cred_handle,char const *hostname,int port,
	struct timeval *timeout,SSL **sslp)
{
	int	sock,ret;
	struct sockaddr_in	a;
	SSL_CTX	*ctx;
	SSL	*ssl;
	struct timeval	before,after,to;
	int	sock_err;
	socklen_t	err_len;
        char *certdir=NULL, *addr;
        proxy_verify_ctx_desc verify_ctx_area;
        proxy_verify_desc verify_area;
	int	h_errno;

		
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) return EDG_WLL_SSL_ERROR_ERRNO;

	if (timeout) {
		int	flags = fcntl(sock, F_GETFL, 0);
	       	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
			return EDG_WLL_SSL_ERROR_ERRNO;
		gettimeofday(&before,NULL);
	}

	switch (h_errno = asyn_gethostbyname(&addr, hostname, timeout)) {
		case NETDB_SUCCESS:
			memset(&a,0,sizeof a);
			a.sin_family = AF_INET;
			memcpy(&a.sin_addr.s_addr,addr,sizeof a.sin_addr.s_addr);
			a.sin_port = htons(port);
			free(addr);
			break;
		case TRY_AGAIN:
			close(sock);
			return EDG_WLL_SSL_ERROR_TIMEOUT;
		case NETDB_INTERNAL: 
			/* fall through */
		default:
			close(sock);
			/* h_errno may be thread safe with Linux pthread libs,
			 * but such an assumption is not portable
			 */
			errno = h_errno;
			return EDG_WLL_SSL_ERROR_HERRNO;
	}

	if (connect(sock,(struct sockaddr *) &a,sizeof a) < 0) {
		if (timeout && errno == EINPROGRESS) {
			fd_set	fds;
			FD_ZERO(&fds);
			FD_SET(sock,&fds);
			memcpy(&to,timeout,sizeof to);
			gettimeofday(&before,NULL);
			switch (select(sock+1,NULL,&fds,NULL,&to)) {
				case -1: close(sock);
					 return EDG_WLL_SSL_ERROR_ERRNO;
				case 0: close(sock);
					return EDG_WLL_SSL_ERROR_TIMEOUT;
			}
			gettimeofday(&after,NULL);
			tv_sub(after,before);
			tv_sub(*timeout,after);

			err_len = sizeof sock_err;
			if (getsockopt(sock,SOL_SOCKET,SO_ERROR,&sock_err,&err_len)) {
				close(sock);
				return EDG_WLL_SSL_ERROR_ERRNO;
			}
			if (sock_err) {
				close(sock);
				errno = sock_err;
				return EDG_WLL_SSL_ERROR_ERRNO;
			}
		}
		else {
			close(sock);
			return EDG_WLL_SSL_ERROR_ERRNO;
		}
	}

	ctx = cred_handle->gs_ctx;
	ssl = SSL_new(ctx);
	if (!ssl) {
		close(sock);
		return EDG_WLL_SSL_ERROR_SSL;
	}
	SSL_set_ssl_method(ssl,SSLv3_method());
	SSL_set_fd(ssl, sock);
        proxy_get_filenames(NULL,1,NULL,&certdir,NULL,NULL,NULL);
        proxy_verify_ctx_init(&verify_ctx_area);
        proxy_verify_init(&verify_area,&verify_ctx_area);
        SSL_set_ex_data(ssl, PVD_SSL_EX_DATA_IDX, (char *)&verify_area);
        if (certdir!=NULL) verify_ctx_area.certdir=certdir;
        
	if (timeout) SSL_set_mode(ssl, SSL_MODE_ENABLE_PARTIAL_WRITE);

	ret = SSL_connect(ssl);
	while (ret <= 0) {
		int	err = SSL_get_error(ssl,ret);
		if ((err = handle_ssl_error(sock,err,timeout))) {
                        proxy_verify_release(&verify_area);
                        proxy_verify_ctx_release(&verify_ctx_area);
			SSL_free(ssl);
			close(sock);
			return err;
		}
		ret = SSL_connect(ssl);
	}
        proxy_verify_release(&verify_area);
        proxy_verify_ctx_release(&verify_ctx_area);
	*sslp = ssl;
	return EDG_WLL_SSL_OK;
}
	
SSL *edg_wll_ssl_accept(proxy_cred_desc *cred_handle,int sock,struct timeval *timeout)
{ SSL *ssl = NULL;
  char *certdir=NULL;
  SSL_CTX *sslContext;
  int ret;


  proxy_verify_ctx_desc verify_ctx_area;
  proxy_verify_desc verify_area;

  sslContext=cred_handle->gs_ctx;

  ssl = SSL_new(sslContext);
  if (ssl == NULL) {
      fprintf(stderr, "SSL_new(): %s\n",
              ERR_error_string(ERR_get_error(), NULL));
      return(NULL);
  }

  SSL_set_ssl_method(ssl,SSLv23_method());
  SSL_set_options(ssl,SSL_OP_NO_SSLv2|SSL_OP_NO_TLSv1);

  proxy_get_filenames(NULL,1,NULL,&certdir,NULL,NULL,NULL);
  proxy_verify_ctx_init(&verify_ctx_area);
  proxy_verify_init(&verify_area,&verify_ctx_area);
  SSL_set_ex_data(ssl, PVD_SSL_EX_DATA_IDX, (char *)&verify_area);
  if (certdir!=NULL) verify_ctx_area.certdir=certdir;

  SSL_set_accept_state(ssl);
  SSL_set_fd(ssl, sock);

  ret = SSL_accept(ssl);
  while (ret <= 0) {
     int     err = SSL_get_error(ssl,ret);
     if ((err = handle_ssl_error(sock,err,timeout))) {
       proxy_verify_release(&verify_area);
       proxy_verify_ctx_release(&verify_ctx_area);
       SSL_free(ssl);
       return (NULL);
     }
     ret = SSL_accept(ssl);
  }

  proxy_verify_release(&verify_area);
  proxy_verify_ctx_release(&verify_ctx_area);

  return(ssl);
}

void edg_wll_ssl_reject(proxy_cred_desc *cred_handle,int sock)
{ SSL *ssl = NULL;
  SSL_CTX *sslContext;

  sslContext=cred_handle->gs_ctx;

  ssl = SSL_new(sslContext);
  if (ssl == NULL) {
      fprintf(stderr, "SSL_new(): %s\n",
              ERR_error_string(ERR_get_error(), NULL));
      return;
  }

  SSL_set_ssl_method(ssl,SSLv23_method());
  SSL_set_options(ssl,SSL_OP_NO_SSLv2|SSL_OP_NO_TLSv1);

  SSL_set_accept_state(ssl);
  SSL_set_fd(ssl, sock);

  SSL_shutdown(ssl);
  SSL_free(ssl);

  return;
}

int edg_wll_ssl_close_timeout(SSL *ssl, struct timeval *timeout) 
{
  int ret, sock;
  
  if (!ssl) return(0);
  sock = SSL_get_fd(ssl);
  ret = SSL_shutdown(ssl);
  while (ret <= 0) {
     int err = SSL_get_error(ssl, ret);
     if (ret == 0 && err == SSL_ERROR_SYSCALL) {
	/* translate misleading error returned by SSL_shutdown() to correct one */
	switch (errno) {
		case 0: goto closed; break;
		case EAGAIN: err = SSL_ERROR_WANT_READ; break;
		default: /* obey err */ break;
	}
     }
     if (handle_ssl_error(sock, err, timeout))
	   break;
     ret = SSL_shutdown(ssl);
  }

closed:
  ret = SSL_clear(ssl);
  ret = close(sock);
  SSL_free(ssl);

  return(0);
}

int edg_wll_ssl_close(SSL *ssl)
{
   struct timeval timeout = {120, 0};
   return edg_wll_ssl_close_timeout(ssl, &timeout);
}

int edg_wll_ssl_read(SSL *ssl,void *buf,size_t bufsize,struct timeval *timeout)
{
	int	len = 0;
	int	sock = SSL_get_fd(ssl);
       
	len = SSL_read(ssl,buf,bufsize);
	while (len <= 0) {
		int	err = SSL_get_error(ssl,len);
		if ((err = handle_ssl_error(sock,err,timeout))) return err;
		len = SSL_read(ssl,buf,bufsize);
	}
	return len;
}

int edg_wll_ssl_write(SSL *ssl,const void *buf,size_t bufsize,struct timeval *timeout)
{
	int	len = 0;
	int	sock = SSL_get_fd(ssl);
       
	len = SSL_write(ssl,buf,bufsize);
	while (len <= 0) {
		int	err = SSL_get_error(ssl,len);
		if ((err = handle_ssl_error(sock,err,timeout))) return err;
		len = SSL_write(ssl,buf,bufsize);
	}
	return len;
}

int edg_wll_ssl_read_full(SSL *ssl,void *buf,size_t bufsize,struct timeval *timeout,size_t *total)
{
	int	len;
	*total = 0;

	while (*total < bufsize) {
		len = edg_wll_ssl_read(ssl,buf+*total,bufsize-*total,timeout);
		if (len < 0) return len;
		*total += len;
	}
	return 0;
}

int edg_wll_ssl_write_full(SSL *ssl,const void *buf,size_t bufsize,struct timeval *timeout,size_t *total)
{
	int	len;
	*total = 0;

	while (*total < bufsize) {
		len = edg_wll_ssl_write(ssl,buf+*total,bufsize-*total,timeout);
		if (len < 0) return len;
		*total += len;
	}
	return 0;
}

static int handle_ssl_error(int sock,int err,struct timeval *to)
{
	struct timeval	timeout,before,after;
	fd_set	fds;
	int	ret = EDG_WLL_SSL_OK;
	int	saved_errno = errno;

	if (to) { 
           memcpy(&timeout,to,sizeof timeout);
	   gettimeofday(&before,NULL);
        }
	switch (err) {
		case SSL_ERROR_ZERO_RETURN:
			ret = EDG_WLL_SSL_ERROR_EOF;
			break;
		case SSL_ERROR_WANT_READ:
			FD_ZERO(&fds);
			FD_SET(sock,&fds);
			switch (select(sock+1,&fds,NULL,NULL,to?&timeout:NULL)){
				case 0: ret = EDG_WLL_SSL_ERROR_TIMEOUT;
					break;
				case -1: ret = EDG_WLL_SSL_ERROR_ERRNO;
					 break;
			}
			break;
		case SSL_ERROR_WANT_WRITE:
			FD_ZERO(&fds);
			FD_SET(sock,&fds);
			switch (select(sock+1,NULL,&fds,NULL,to?&timeout:NULL)){
				case 0: ret = EDG_WLL_SSL_ERROR_TIMEOUT;
					break;
				case -1: ret = EDG_WLL_SSL_ERROR_ERRNO;
					 break;
			}
			break;
		case SSL_ERROR_SYSCALL:
			if (saved_errno == 0) ret = EDG_WLL_SSL_ERROR_EOF;
			else ret = EDG_WLL_SSL_ERROR_ERRNO;
			break;
		case SSL_ERROR_SSL:
			ret = EDG_WLL_SSL_ERROR_SSL;
			break;
		default:
			fprintf(stderr,"unexpected SSL_get_error code %d\n",err);
			abort();
	}
	if (to) {
           gettimeofday(&after,NULL);
	   tv_sub(after,before);
	   tv_sub(*to,after);
	   if (to->tv_sec < 0) {
		to->tv_sec = 0;
		to->tv_usec = 0;
	   }
        }

	return ret;
}

/* XXX: I'm afraid the contents of stuct stat is somewhat OS dependent */

int edg_wll_ssl_watch_creds(
		const char	*key_file,
		const char	*cert_file,
		time_t		*key_mtime,
		time_t		*cert_mtime)
{
	struct stat	kstat,cstat;
	int	reload = 0;

	if (!key_file || !cert_file) return 0;
	if (stat(key_file,&kstat) || stat(cert_file,&cstat)) return -1;

	if (!*key_mtime) *key_mtime = kstat.st_mtime;
	if (!*cert_mtime) *cert_mtime = cstat.st_mtime;

	if (*key_mtime != kstat.st_mtime) {
		*key_mtime = kstat.st_mtime;
		reload = 1;
	}

	if (*cert_mtime != cstat.st_mtime) {
		*cert_mtime = cstat.st_mtime;
		reload = 1;
	}

	return reload;
}
