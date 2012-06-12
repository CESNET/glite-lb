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


#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <ares.h>
#include <ares_version.h>
#include <errno.h>
#ifdef GLITE_LBU_THREADED
#include <pthread.h>
#endif

#include <globus_common.h>
#ifndef NO_GLOBUS_GSSAPI
#include <globus_gsi_callback.h>
#endif

#include <gssapi.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/stack.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

#include "glite_gss.h"

#ifndef GSS_C_GLOBUS_SSL_COMPATIBLE
#define GSS_C_GLOBUS_SSL_COMPATIBLE	16384
#endif

#define tv_sub(a,b) {\
	(a).tv_usec -= (b).tv_usec;\
	(a).tv_sec -= (b).tv_sec;\
	if ((a).tv_usec < 0) {\
		(a).tv_sec--;\
		(a).tv_usec += 1000000;\
	}\
}

struct asyn_result {
	struct hostent *ent;
	int		err;
};

static int globus_common_activated = 0;
#ifdef GLITE_LBU_THREADED
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

typedef struct gssapi_mech {
    const char* name;
    gss_OID_desc oid;
} gssapi_mech;

static
struct gssapi_mech gssapi_mechs[] = {
    { "krb5", {9, "\x2A\x86\x48\x86\xF7\x12\x01\x02\x02"} },
    { "GSI", {9, "\x2b\x06\x01\x04\x01\x9b\x50\x01\x01"} },
    { "EAP", {9, "\x2B\x06\x01\x04\x01\xA9\x4A\x16\x01"} },
};

static gss_OID
get_oid(const char *mech);

static void
free_hostent(struct hostent *h);

static int
try_conn_and_auth(edg_wll_GssCred cred, gss_name_t target, gss_OID mech,
		  char *addr,  int addrtype, int port, struct timeval *timeout,
		  edg_wll_GssConnection *connection, edg_wll_GssStatus* gss_code);

static int
edg_wll_gss_oid_equal(const gss_OID a, const gss_OID b);

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

/* ares callback handler for ares_gethostbyname()       */
#if ARES_VERSION >= 0x010500
static void callback_ares_gethostbyname(void *arg, int status, int timeouts, struct hostent *h)
#else
static void callback_ares_gethostbyname(void *arg, int status, struct hostent *h)
#endif
{
	struct asyn_result *arp = (struct asyn_result *) arg;
	int n_addr = 0;
	int i = 0; 

	switch (status) {
	   case ARES_SUCCESS:
		if (h == NULL || h->h_addr_list[0] == NULL){
			arp->err = NO_DATA;
			break;
		}
		/*how many addresses are there in h->h_addr_list*/
		while (h->h_addr_list[n_addr])
			n_addr++;
		
		arp->ent->h_addr_list = (char **) calloc((n_addr+1), sizeof(char *));
		if (arp->ent->h_addr_list == NULL) {
			arp->err = NETDB_INTERNAL;
			break;
		}
		for (i = 0; i < n_addr; i++) {
			arp->ent->h_addr_list[i] = malloc(h->h_length);
			if (arp->ent->h_addr_list[i] == NULL) {
				free_hostent (arp->ent);
				arp->ent = NULL;	
				arp->err = NETDB_INTERNAL;
				break;
			}
			memcpy(arp->ent->h_addr_list[i], h->h_addr_list[i],
				h->h_length);
		}
			/* rest of h members might be assigned here(name,aliases), not necessery now */
			arp->ent->h_addr_list[n_addr] = NULL;
			arp->ent->h_addrtype = h->h_addrtype;
			arp->ent->h_length = h->h_length;
			arp->err = NETDB_SUCCESS;
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

static int asyn_getservbyname(int af, struct asyn_result *ar,char const *name, int port, struct timeval *timeout) {
	ares_channel channel;
	int nfds;
	fd_set readers, writers;
	struct timeval tv, *tvp;
	struct timeval start_time,check_time;
	int	err = NETDB_INTERNAL;
	char	*name2, *p;
	size_t	namelen;

	name2 = (char *)name;
	namelen = strlen(name);
	if (name[0]=='[' && name[namelen-1]==']') {
		/* IPv6 literal, strip brackets */
		name2 = strdup(name);
		if (!name2) return NETDB_INTERNAL;
		name2[namelen-1] = '\0';
		name2++;
		/* Ignore scope identifier, not supported by c-ares */
		p = strchr(name2, '%'); 
		if (p) *p = '\0';
	}	

/* start timer */
	gettimeofday(&start_time,0);

/* ares init */
	if ( ares_init(&channel) != ARES_SUCCESS ) return(NETDB_INTERNAL);

/* query DNS server asynchronously */
	ares_gethostbyname(channel, name2, af, callback_ares_gethostbyname,
			   (void *) ar);

/* wait for result */
	while (1) {
		FD_ZERO(&readers);
		FD_ZERO(&writers);
		nfds = ares_fds(channel, &readers, &writers);
		if (nfds == 0)
			break;

		gettimeofday(&check_time,0);
		if (timeout && decrement_timeout(timeout, start_time, check_time)) {
			ares_destroy(channel);
			return(TRY_AGAIN);
		}
		start_time = check_time;

		tvp = ares_timeout(channel, timeout, &tv);

		switch ( select(nfds, &readers, &writers, NULL, tvp) ) {
			case -1: if (errno != EINTR) {
					ares_destroy(channel);
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
	err = ar->err;

	/* literal conversion should always succeed */
	if (name2 != name) free(name2-1); 

	ares_destroy(channel);

	return err;
}

static int
do_connect(int *s, char *addr, int addrtype, int port, struct timeval *timeout)
{
   int sock;
   struct timeval before,after,to;
   struct sockaddr_storage a;
   struct sockaddr_storage *p_a=&a;
   socklen_t a_len;
   int sock_err;
   socklen_t err_len;
   int	opt;

   struct sockaddr_in *p4 = (struct sockaddr_in *)p_a;
   struct sockaddr_in6 *p6 = (struct sockaddr_in6 *)p_a;

   memset(p_a, 0, sizeof *p_a);
   p_a->ss_family = addrtype;
   switch (addrtype) {
	case AF_INET:
		memcpy(&p4->sin_addr, addr, sizeof(struct in_addr));
		p4->sin_port = htons(port);
		a_len = sizeof (struct sockaddr_in);
		break;
	case AF_INET6:
		memcpy(&p6->sin6_addr, addr, sizeof(struct in6_addr));
		p6->sin6_port = htons(port);
		a_len = sizeof (struct sockaddr_in6);
		break;
	default:
		return NETDB_INTERNAL;
		break;
   }

   sock = socket(a.ss_family, SOCK_STREAM, 0);
   if (sock < 0) return EDG_WLL_GSS_ERROR_ERRNO;

   opt = 1;
   setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,&opt,sizeof opt);

   if (timeout) {
	     int	flags = fcntl(sock, F_GETFL, 0);
	     if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
		     return EDG_WLL_GSS_ERROR_ERRNO;
	     gettimeofday(&before,NULL);
   }
   
   if (connect(sock,(struct sockaddr *) &a, a_len) < 0) {
	     if (timeout && errno == EINPROGRESS) {
		     fd_set	fds;
		     FD_ZERO(&fds);
		     FD_SET(sock,&fds);
		     memcpy(&to,timeout,sizeof to);
		     gettimeofday(&before,NULL);
		     switch (select(sock+1,NULL,&fds,NULL,&to)) {
			     case -1: close(sock);
				      return EDG_WLL_GSS_ERROR_ERRNO;
			     case 0: close(sock);
					tv_sub(*timeout, *timeout);
				     return EDG_WLL_GSS_ERROR_TIMEOUT;
		     }
		     gettimeofday(&after,NULL);
		     tv_sub(after,before);
		     tv_sub(*timeout,after);

		     err_len = sizeof sock_err;
		     if (getsockopt(sock,SOL_SOCKET,SO_ERROR,&sock_err,&err_len)) {
			     close(sock);
			     return EDG_WLL_GSS_ERROR_ERRNO;
		     }
		     if (sock_err) {
			     close(sock);
			     errno = sock_err;
			     return EDG_WLL_GSS_ERROR_ERRNO;
		     }
	     }
	     else {
		     close(sock);
		     return EDG_WLL_GSS_ERROR_ERRNO;
	     }
   }

   *s = sock;
   return 0;
}

static gss_OID
get_oid(const char *mech)
{
    unsigned int i;

    for (i = 0; i < sizeof(gssapi_mechs)/sizeof(gssapi_mechs[0]); i++)
	if (strcasecmp(mech, gssapi_mechs[i].name) == 0)
	    return &gssapi_mechs[i].oid;

    return GSS_C_NO_OID;
}

static int
send_token(int sock, void *token, size_t token_length, struct timeval *to)
{
   size_t num_written = 0;
   ssize_t count;
   fd_set fds;
   struct timeval timeout,before,after;
   int ret;

   if (to) {
       memcpy(&timeout,to,sizeof(timeout));
       gettimeofday(&before,NULL);
   }


   ret = 0;
   while(num_written < token_length) {
      FD_ZERO(&fds);
      FD_SET(sock,&fds);
      switch (select(sock+1, NULL, &fds, NULL, to ? &timeout : NULL)) {
	 case 0: ret = EDG_WLL_GSS_ERROR_TIMEOUT;
		 goto end;
		 break;
	 case -1: ret = EDG_WLL_GSS_ERROR_ERRNO;
		  goto end;
		  break;
      }

      count = write(sock, ((char *)token) + num_written,
	            token_length - num_written);
      if(count < 0) {
	 if(errno == EINTR)
	    continue;
	 else {
	    ret = EDG_WLL_GSS_ERROR_ERRNO;
	    goto end;
	 }
      }
      num_written += count;
   }

end:
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

static int
send_gss_token(int sock, gss_OID mech, void *token, size_t token_length, struct timeval *to)
{
    int ret;
    uint32_t net_len = htonl(token_length);

    /* Don't use the usual message framing when using Globus GSI, in order to be
       compatible with SSL on the wire. */
    if (!edg_wll_gss_oid_equal(mech, get_oid("GSI"))) {
	ret = send_token(sock, &net_len, 4, to);
	if (ret)
	    return ret;
    }

    return send_token(sock, token, token_length, to);
}

static int
recv_token(int sock, void *token, size_t token_length, struct timeval *to)
{
   ssize_t count;
   fd_set fds;
   struct timeval timeout,before,after;
   int ret;

   if (to) {
      memcpy(&timeout,to,sizeof(timeout));
      gettimeofday(&before,NULL);
   }

   do {
      FD_ZERO(&fds);
      FD_SET(sock,&fds);
      switch (select(sock+1, &fds, NULL, NULL, to ? &timeout : NULL)) {
	 case 0: 
	    ret = EDG_WLL_GSS_ERROR_TIMEOUT;
	    goto end;
	    break;
	 case -1:
	    ret = EDG_WLL_GSS_ERROR_ERRNO;
	    goto end;
	    break;
      }
      
      count = read(sock, token, token_length);
      if (count < 0) {
	 if (errno == EINTR)
	    continue;
	 else {
	    ret = EDG_WLL_GSS_ERROR_ERRNO;
	    goto end;
	 }
      }

      if (count==0) {
	  ret = EDG_WLL_GSS_ERROR_EOF;
	  goto end;
      }

   } while (count < 0); /* restart on EINTR */

   ret = count;

end:
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

/* similar to recv_token() but never returns partial data */
static int
read_token(int sock, void *token, size_t length, struct timeval *to)
{
    size_t remain = length;
    char *buf = token;
    int count;

    while (remain > 0) {
	count = recv_token(sock, buf, remain, to);
	if (count < 0)
	    return count;
	buf += count;
	remain -= count;
    }

    return length;
}

/*
   SSL/TLS framing:
   SSLv3 header:
   1st byte: SSL Handshake Record Type (\x16)
   2nd-3rd bytes: SSL Version 3.0 (\x03 \x00)
                  TLS Version 1.0 (\x03 \x01)
   4th-5th bytes: Length

   SSLv2 header:
   1st-2nd  bytes: length, 1st byte has the highest bit set to 1
                   N.B. this applies to client_hello, other messages may encode
		   lenght in 3 bytes
   3rd byte: type (CLIENT_HELLO) (== 1)
   4rd-5th bytes: SSL Version 3.0 (\x03 \x00)
		  TLS Version 1.0 (\x03 \x01)

   see also e.g. openssl's s23_srvr.c
*/
#define SSLv3_HEADER "\x16\x03\x00"
#define TLSv1_HEADER "\x16\x03\x01"

#define SSL2_CLIENT_HELLO	0x01
#define SSL3_VERSION_MAJOR	0x03

static int
is_ssl(unsigned char *header)
{
    if (memcmp(header, SSLv3_HEADER, 3) == 0)
	return 1;
    if (memcmp(header, TLSv1_HEADER, 3) == 0)
	return 1;
    if ((header[0] & 0x80) && header[2] == SSL2_CLIENT_HELLO &&
	    header[3] == SSL3_VERSION_MAJOR)
	return 1;

    return 0;
    /* payload len == (size_t)(header[3]) << 8) | header[4]) + 5; */
}


static int
recv_gss_token(int sock, gss_OID mech, void **token, size_t *token_length, struct timeval *to)
{
    int ret;
    uint32_t len, net_len;
    unsigned char *header;
    char buf[4096];
    char *b = NULL;

    if (edg_wll_gss_oid_equal(mech, get_oid("GSI"))) {
	ret = recv_token(sock, buf, sizeof(buf), to);
	if (ret < 0)
	    return ret;

	*token = malloc(ret);
	if (*token == NULL)
	    return EDG_WLL_GSS_ERROR_ERRNO;

	memcpy(*token, buf, ret);
	*token_length = ret;

	return 0;
    }

    ret = read_token(sock, &net_len, 4, to);
    if (ret < 0)
	return ret;

    header = (unsigned char *)&net_len;
    if (mech == GSS_C_NO_OID && is_ssl(header)) {
	/* SSL detected. Let's return this fragment to the caller relying on
	   the Globus libs being able to cope with partial messages. */
	*token = malloc(4);
	if (*token == NULL)
	    return EDG_WLL_GSS_ERROR_ERRNO;

	memcpy(*token, header, 4);
	*token_length = 4;

	return 0;
    }

    len =  ntohl(net_len);
    b = malloc(len);
    if (b == NULL)
	return EDG_WLL_GSS_ERROR_ERRNO;

    ret = read_token(sock, b, len, to);
    if (ret < 0) {
	free(b);
	return ret;
    }

    *token = b;
    *token_length = len;

    return 0;
}

static int
create_proxy(const char *cert_file, const char *key_file, char **proxy_file)
{
   char buf[4096];
   int in, out;
   char *name = NULL;
   int ret, len;

   *proxy_file = NULL;

   asprintf(&name, "%s/%d.lb.XXXXXX", P_tmpdir, getpid());

   out = mkstemp(name);
   if (out < 0)
      return EDG_WLL_GSS_ERROR_ERRNO;

   in = open(cert_file, O_RDONLY);
   if (in < 0) {
      ret = EDG_WLL_GSS_ERROR_ERRNO;
      goto end;
   }
   while ((ret = read(in, buf, sizeof(buf))) > 0) {
      len = write(out, buf, ret);
      if (len != ret) {
	 ret = -1;
	 break;
      }
   }
   close(in);
   if (ret < 0) {
      ret = EDG_WLL_GSS_ERROR_ERRNO;
      goto end;
   }

   len = write(out, "\n", 1);
   if (len != 1) {
      ret = EDG_WLL_GSS_ERROR_ERRNO;
      goto end;
   }

   in = open(key_file, O_RDONLY);
   if (in < 0) {
      ret = EDG_WLL_GSS_ERROR_ERRNO;
      goto end;
   }
   while ((ret = read(in, buf, sizeof(buf))) > 0) {
      len = write(out, buf, ret);
      if (len != ret) {
	 ret = -1;
	 break;
      }
   }
   close(in);
   if (ret < 0) {
      ret = EDG_WLL_GSS_ERROR_ERRNO;
      goto end;
   }

   ret = 0;
   *proxy_file = name;

end:
   close(out);
   if (ret) {
      unlink(name);
      free(name);
   }

   return ret;
}

static int
destroy_proxy(char *proxy_file)
{
   /* XXX we should erase the contents safely (i.e. overwrite with 0's) */
   unlink(proxy_file);
   return 0;
}

/** Load or reload credentials. It should be called regularly (credential files can be changed).
  This call works only for GSSAPI from Globus.
 @see edg_wll_gss_watch_creds
 */
int
edg_wll_gss_acquire_cred_gsi(const char *cert_file, const char *key_file, edg_wll_GssCred *cred,
      			     edg_wll_GssStatus* gss_code)
{
   OM_uint32 major_status = 0, minor_status, minor_status2;
   gss_cred_id_t gss_cred = GSS_C_NO_CREDENTIAL;
   gss_buffer_desc buffer = GSS_C_EMPTY_BUFFER;
   gss_name_t gss_name = GSS_C_NO_NAME;
   gss_OID_set_desc mechs;
   gss_OID_set avail_mechs = NULL;
   OM_uint32 lifetime;
   char *proxy_file = NULL;
   char *name = NULL;
   int ret, gsi_available;

   *cred = NULL;

   major_status = gss_indicate_mechs(&minor_status, &avail_mechs);
   /* ignore error */

   major_status = gss_test_oid_set_member(&minor_status, get_oid("GSI"),
					  avail_mechs, &gsi_available);
   if (!GSS_ERROR(major_status) && !gsi_available) {
       if (cert_file != NULL || key_file != NULL) {
	   errno = EINVAL;
	   ret = EDG_WLL_GSS_ERROR_ERRNO;
	   goto end;
       } else {
	       ret = 0;
	       lifetime = 0;
	       goto end1;
       }
   }

   if ((cert_file == NULL && key_file != NULL) ||
       (cert_file != NULL && key_file == NULL))
      return EINVAL;

   if (cert_file == NULL) {
      mechs.count = 1;
      mechs.elements = get_oid("GSI");
      
      major_status = gss_acquire_cred(&minor_status, GSS_C_NO_NAME, 0,
	    			      &mechs, GSS_C_BOTH,
				      &gss_cred, NULL, NULL);
      if (GSS_ERROR(major_status)) {
	 ret = EDG_WLL_GSS_ERROR_GSS;
	 goto end;
      }
   } else {
#ifndef NO_GLOBUS_GSSAPI
      proxy_file = (char *)cert_file;
      if (strcmp(cert_file, key_file) != 0 &&
	  (ret = create_proxy(cert_file, key_file, &proxy_file))) {
	 proxy_file = NULL;
	 goto end;
      }
      
      ret = asprintf((char**)&buffer.value, "X509_USER_PROXY=%s", proxy_file);
      if (ret == -1) {
	 errno = ENOMEM;
	 ret = EDG_WLL_GSS_ERROR_ERRNO;
	 goto end;
      }
      buffer.length = ret;

      major_status = gss_import_cred(&minor_status, &gss_cred,
				     GSS_C_NO_OID, 1,
				     &buffer, 0, NULL);
      free(buffer.value);
      if (GSS_ERROR(major_status)) {
	 ret = EDG_WLL_GSS_ERROR_GSS;
	 goto end;
      }
#else
      errno = EINVAL;
      ret = EDG_WLL_GSS_ERROR_ERRNO;
      goto end;
#endif
   }

  /* gss_import_cred() doesn't check validity of credential loaded, so let's
    * verify it now */
    major_status = gss_inquire_cred(&minor_status, gss_cred, &gss_name,
	  			    &lifetime, NULL, NULL);
    if (GSS_ERROR(major_status)) {
       ret = EDG_WLL_GSS_ERROR_GSS;
       goto end;
    }

    /* Must cast to time_t since OM_uint32 is unsinged and hence we couldn't
     * detect negative values. */
    if ((time_t) lifetime <= 0) {
       major_status = GSS_S_CREDENTIALS_EXPIRED;
       minor_status = 0; /* XXX */
       ret = EDG_WLL_GSS_ERROR_GSS;
       goto end;
    }

   major_status = gss_display_name(&minor_status, gss_name, &buffer, NULL);
   if (GSS_ERROR(major_status)) {
      ret = EDG_WLL_GSS_ERROR_GSS;
      goto end;
   }
   name = buffer.value;
   memset(&buffer, 0, sizeof(buffer));
    

end1:

   *cred = calloc(1, sizeof(**cred));
   if (*cred == NULL) {
      ret = EDG_WLL_GSS_ERROR_ERRNO;
      free(name);
      goto end;
   }

   (*cred)->gss_cred = gss_cred;
   gss_cred = GSS_C_NO_CREDENTIAL;
   (*cred)->lifetime = lifetime;
   (*cred)->name = name;

   ret = 0;

end:
   if (cert_file && key_file && proxy_file && strcmp(cert_file, key_file) != 0) {
      destroy_proxy(proxy_file);
      free(proxy_file);
   }

   if (gss_name != GSS_C_NO_NAME)
      gss_release_name(&minor_status2, &gss_name);

   if (gss_cred != GSS_C_NO_CREDENTIAL)
      gss_release_cred(&minor_status2, &gss_cred);

   if (avail_mechs)
       gss_release_oid_set(&minor_status2, &avail_mechs);

   if (GSS_ERROR(major_status)) {
      if (gss_code) {
	 gss_code->major_status = major_status;
	 gss_code->minor_status = minor_status;
      }
      ret = EDG_WLL_GSS_ERROR_GSS;
   }

   return ret;
}
/* XXX XXX This is black magic. "Sometimes" server refuses the client with SSL
 * alert "certificate expired" even if it is not true. In this case the server
 * slave terminates (which helps, usually), and we can reconnect transparently.
 */

/* This string appears in the error message in this case */
#define _EXPIRED_ALERT_MESSAGE "function SSL3_READ_BYTES: sslv3 alert certificate expired"
#define _EXPIRED_ALERT_RETRY_COUNT 10   /* default number of slaves, hope that not all
					                                              are in the bad state */
#define _EXPIRED_ALERT_RETRY_DELAY 10   /* ms */

/** Create a socket and initiate secured connection. */
static int
gss_connect(edg_wll_GssCred cred, char const *hostname, int port,
	    gss_name_t target, gss_OID_set mechs,
	    struct timeval *timeout, edg_wll_GssConnection *connection,
	    edg_wll_GssStatus* gss_code)
{
   int ret;
   struct asyn_result ar;
   int h_errno;
   int addr_types[] = {AF_INET6, AF_INET};
   int ipver = AF_INET6; //def value; try IPv6 first
   unsigned int j,k;
   int i;
   gss_OID mech;
   gss_OID_set_desc my_mechs = {.count = 0, .elements = GSS_C_NO_OID};
   const char *mech_name;

   memset(connection, 0, sizeof(*connection));
   for (j = 0; j< sizeof(addr_types)/sizeof(*addr_types); j++) {
	ipver = addr_types[j];
	ar.ent = (struct hostent *) calloc (1, sizeof(struct hostent));
	switch (h_errno = asyn_getservbyname(ipver, &ar, hostname, port, timeout)) {
		case NETDB_SUCCESS:
			break;
		case TRY_AGAIN:
			ret = EDG_WLL_GSS_ERROR_TIMEOUT;
			goto end;
		case NETDB_INTERNAL: 
			errno = h_errno;
			ret = EDG_WLL_GSS_ERROR_HERRNO;
			goto end; 
		default:
			/* h_errno may be thread safe with Linux pthread libs,
			 * but such an assumption is not portable
			 */
			errno = h_errno;
			ret = EDG_WLL_GSS_ERROR_HERRNO;
			continue; 
	}

	if (mechs == GSS_C_NO_OID_SET) {
	    mech_name = getenv("GLITE_GSS_MECH");
	    if (mech_name == NULL)
		mech_name = "GSI";

	    mech = get_oid(mech_name);
	    if (mech != GSS_C_NO_OID) {
		my_mechs.elements = mech;
		my_mechs.count = 1;
		mechs = &my_mechs;
	    }
	}
   
   	i = 0;
	while (ar.ent->h_addr_list[i])
	{
	    k = 0;
	    do {
		if (mechs == GSS_C_NO_OID_SET || mechs->count == 0)
		    mech = GSS_C_NO_OID;
		else
		    mech = &mechs->elements[k];

		ret = try_conn_and_auth (cred, target, mech,
					ar.ent->h_addr_list[i], 
					ar.ent->h_addrtype, port, timeout, connection, gss_code);
		if (ret == 0)
			goto end;
		if (timeout && (timeout->tv_sec < 0 ||(timeout->tv_sec == 0 && timeout->tv_usec <= 0)))
			goto end;
		k++;
	    } while (mechs != GSS_C_NO_OID_SET && k < mechs->count);
	    i++;
	}
	free_hostent(ar.ent);
	ar.ent = NULL;
   }

   end:
   if (ar.ent != NULL){
	free_hostent(ar.ent);
	ar.ent = NULL;
   }
   return ret;
}

int 
edg_wll_gss_connect_ext(edg_wll_GssCred cred, char const *hostname, int port,
			const char *service, gss_OID_set mechs, 
		        struct timeval *timeout, edg_wll_GssConnection *connection,
		        edg_wll_GssStatus* gss_code)
{
    char *servername = NULL;
    gss_name_t target = GSS_C_NO_NAME;
    OM_uint32 maj_stat, min_stat;
    gss_buffer_desc input_token = GSS_C_EMPTY_BUFFER;
    int ret;

    ret = asprintf(&servername, "%s@%s",
	    (service) ? service : "host", hostname);
    if (ret == -1) {
	errno = ENOMEM;
	return EDG_WLL_GSS_ERROR_ERRNO;
    }
    input_token.value = servername;
    input_token.length = strlen(servername) + 1;

    maj_stat = gss_import_name(&min_stat, &input_token,
	    GSS_C_NT_HOSTBASED_SERVICE, &target);
    if (GSS_ERROR(maj_stat)) {
	if (gss_code) {
	    gss_code->major_status = maj_stat;
	    gss_code->minor_status = min_stat;
	}
	ret = EDG_WLL_GSS_ERROR_GSS;
	goto end;
    }

    ret = gss_connect(cred, hostname, port, target, mechs,
		      timeout, connection, gss_code);

end:
    if (target != GSS_C_NO_NAME)
	gss_release_name(&min_stat, &target);
    free(servername);

    return ret;
}

int
edg_wll_gss_connect_name(edg_wll_GssCred cred,
			 char const *hostname,
			 int port,
			 const char *servername,
			 gss_OID_set mechs,
			 struct timeval *timeout,
			 edg_wll_GssConnection *connection,
			 edg_wll_GssStatus* gss_code)
{
    gss_name_t target = GSS_C_NO_NAME;
    OM_uint32 maj_stat, min_stat;
    gss_buffer_desc input_token = GSS_C_EMPTY_BUFFER;
    int ret;

    if (servername == NULL) {
	errno = ENOSYS;
	return EDG_WLL_GSS_ERROR_ERRNO;
    }
    input_token.value = (char *) servername;
    input_token.length = strlen(servername) + 1;

    maj_stat = gss_import_name(&min_stat, &input_token,
	    GSS_C_NT_USER_NAME, &target);
    if (GSS_ERROR(maj_stat)) {
	if (gss_code) {
	    gss_code->major_status = maj_stat;
	    gss_code->minor_status = min_stat;
	}
	ret = EDG_WLL_GSS_ERROR_GSS;
	goto end;
    }

    ret = gss_connect(cred, hostname, port, target, mechs,
		      timeout, connection, gss_code);

end:
    if (target != GSS_C_NO_NAME)
	gss_release_name(&min_stat, &target);

    return ret;
}


int
edg_wll_gss_connect(edg_wll_GssCred cred, char const *hostname, int port,
                    struct timeval *timeout, edg_wll_GssConnection *connection,
                    edg_wll_GssStatus* gss_code)
{
    return edg_wll_gss_connect_ext(cred, hostname, port,
				   NULL, NULL,
				   timeout, connection, gss_code);
}

/* try connection and authentication for the given addr*/
static int try_conn_and_auth (edg_wll_GssCred cred, gss_name_t target, gss_OID mech,
	                      char *addr,  int addrtype, int port,
			      struct timeval *timeout, edg_wll_GssConnection *connection,
			      edg_wll_GssStatus* gss_code) 
{
   int sock;
   int ret = 0;
   OM_uint32 maj_stat, min_stat, min_stat2, req_flags;
   int context_established = 0;
   gss_buffer_desc input_token = GSS_C_EMPTY_BUFFER;
   gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;
   gss_ctx_id_t context = GSS_C_NO_CONTEXT;
   gss_cred_id_t gss_cred = GSS_C_NO_CREDENTIAL;
   int retry = _EXPIRED_ALERT_RETRY_COUNT;

   maj_stat = min_stat = min_stat2 = req_flags = 0;

   if (edg_wll_gss_oid_equal(mech, get_oid("GSI"))) {
       req_flags = GSS_C_GLOBUS_SSL_COMPATIBLE;
   }

   ret = do_connect(&sock, addr, addrtype, port, timeout);
   if (ret)
      return ret;

   if (cred && cred->gss_cred)
       gss_cred = cred->gss_cred;

   do { /* XXX: the black magic above */

   while (!context_established) {
      /* XXX verify ret_flags match what was requested */
      maj_stat = gss_init_sec_context(&min_stat, gss_cred, &context,
				      target, mech,
				      req_flags | GSS_C_MUTUAL_FLAG | GSS_C_CONF_FLAG,
				      0, GSS_C_NO_CHANNEL_BINDINGS,
				      &input_token, NULL, &output_token,
				      NULL, NULL);
      if (input_token.length > 0) {
	 free(input_token.value);
	 input_token.length = 0;
      }

      if (mech == GSS_C_NO_OID) {
	  gss_OID oid;

	  maj_stat = gss_inquire_context(&min_stat, context, NULL, NULL,
					 NULL, &oid, NULL, NULL, NULL);
	  if (!GSS_ERROR(maj_stat))
	      mech = oid;
      }

      if (output_token.length != 0) {
	 ret = send_gss_token(sock, mech, output_token.value, output_token.length, timeout);
	 gss_release_buffer(&min_stat2, &output_token);
	 if (ret)
	    goto end;
      }

      if (GSS_ERROR(maj_stat)) {
	 if (context != GSS_C_NO_CONTEXT) {
	    gss_delete_sec_context(&min_stat2, &context, &output_token);
	    context = GSS_C_NO_CONTEXT;
      	    if (output_token.length) {
		send_gss_token(sock, mech, output_token.value, output_token.length, timeout);
	 	gss_release_buffer(&min_stat2, &output_token);
      	    }
	 }
	 ret = EDG_WLL_GSS_ERROR_GSS;
	 goto end;
      }

      if(maj_stat & GSS_S_CONTINUE_NEEDED) {
	 ret = recv_gss_token(sock, mech, &input_token.value, &input_token.length, timeout);
	 if (ret)
	    goto end;
      } else
	 context_established = 1;
   }

   /* XXX check ret_flags matches to what was requested */

   /* retry on false "certificate expired" */
   if (ret == EDG_WLL_GSS_ERROR_GSS) {
	   edg_wll_GssStatus	gss_stat;
	   char	*msg = NULL;

	   gss_stat.major_status = maj_stat;
	   gss_stat.minor_status = min_stat;
	   edg_wll_gss_get_error(&gss_stat,"",&msg);

	   if (strstr(msg,_EXPIRED_ALERT_MESSAGE)) {
		   usleep(_EXPIRED_ALERT_RETRY_DELAY);
		   retry--;
	   }
	   else retry = 0;

	   free(msg);
   }
   else retry = 0;

   } while (retry);

   connection->sock = sock;
   connection->context = context;
   connection->authn_mech = mech;
   ret = 0;

end:
   if (ret == EDG_WLL_GSS_ERROR_GSS && gss_code) {
      gss_code->major_status = maj_stat;
      gss_code->minor_status = min_stat;
   }
   if (ret)
      close(sock);

   return ret;
}

/** Accept a new secured connection on the listening socket. */
int
edg_wll_gss_accept(edg_wll_GssCred cred, int sock, struct timeval *timeout,
		   edg_wll_GssConnection *connection, edg_wll_GssStatus* gss_code)
{
   OM_uint32 maj_stat, min_stat, min_stat2;
   OM_uint32 ret_flags = 0;
   gss_buffer_desc input_token = GSS_C_EMPTY_BUFFER;
   gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;
   gss_name_t client_name = GSS_C_NO_NAME;
   gss_ctx_id_t context = GSS_C_NO_CONTEXT;
   gss_cred_id_t gss_cred = GSS_C_NO_CREDENTIAL;
   gss_OID mech = GSS_C_NO_OID;
   int ret;

   maj_stat = min_stat = min_stat2 = 0;
   memset(connection, 0, sizeof(*connection));

   if (cred && cred->gss_cred)
       gss_cred = cred->gss_cred;

   ret_flags = GSS_C_GLOBUS_SSL_COMPATIBLE;

   do {
      ret = recv_gss_token(sock, mech, &input_token.value, &input_token.length, timeout);
      if (ret)
	 goto end;

      if (client_name != GSS_C_NO_NAME)
        gss_release_name(&min_stat2, &client_name);

      maj_stat = gss_accept_sec_context(&min_stat, &context,
	    			        gss_cred, &input_token,
					GSS_C_NO_CHANNEL_BINDINGS,
					&client_name, &mech, &output_token,
					&ret_flags, NULL, NULL);
      if (input_token.length > 0) {
	 free(input_token.value);
	 input_token.length = 0;
      }

      if (output_token.length) {
	 ret = send_gss_token(sock, mech, output_token.value, output_token.length, timeout);
	 gss_release_buffer(&min_stat2, &output_token);
	 if (ret)
	    goto end;
      }
   } while(maj_stat & GSS_S_CONTINUE_NEEDED);

   if (GSS_ERROR(maj_stat)) {
      if (context != GSS_C_NO_CONTEXT) {
	 gss_delete_sec_context(&min_stat2, &context, &output_token);
	 context = GSS_C_NO_CONTEXT;
      	 if (output_token.length) {
		send_gss_token(sock, mech, output_token.value, output_token.length, timeout);
	 	gss_release_buffer(&min_stat2, &output_token);
      	 }
      }
      ret = EDG_WLL_GSS_ERROR_GSS;
      goto end;
   }

#if 0
   maj_stat = gss_display_name(&min_stat, client_name, &output_token, NULL);
   gss_release_buffer(&min_stat2, &output_token);
   if (GSS_ERROR(maj_stat)) {
      /* XXX close context ??? */
      ret = EDG_WLL_GSS_ERROR_GSS;
      goto end;
   }
#endif

   connection->sock = sock;
   connection->context = context;
   connection->authn_mech = mech;
   ret = 0;

end:
   if (ret == EDG_WLL_GSS_ERROR_GSS && gss_code) {
      gss_code->major_status = maj_stat;
      gss_code->minor_status = min_stat;
   }
   if (client_name != GSS_C_NO_NAME)
      gss_release_name(&min_stat2, &client_name);

   return ret;
}

/** Send data over the opened connection. */
int
edg_wll_gss_write(edg_wll_GssConnection *connection, const void *buf, size_t bufsize,
		  struct timeval *timeout, edg_wll_GssStatus* gss_code)
{
   OM_uint32 maj_stat, min_stat;
   gss_buffer_desc  input_token;
   gss_buffer_desc  output_token;
   int  ret;

   input_token.value = (void*)buf;
   input_token.length = bufsize;

   maj_stat = gss_wrap (&min_stat, connection->context, 1, GSS_C_QOP_DEFAULT,
			&input_token, NULL, &output_token);
   if (GSS_ERROR(maj_stat)) {
      if (gss_code) {
	 gss_code->minor_status = min_stat;
	 gss_code->major_status = maj_stat;
      }

      return EDG_WLL_GSS_ERROR_GSS;
   }

   ret = send_gss_token(connection->sock, connection->authn_mech,
		    output_token.value, output_token.length,
	            timeout);
   gss_release_buffer(&min_stat, &output_token);

   return ret;
}


/** Read a data chunk through the opened connection. */
int
edg_wll_gss_read(edg_wll_GssConnection *connection, void *buf, size_t bufsize,
		 struct timeval *timeout, edg_wll_GssStatus* gss_code)
{
   OM_uint32 maj_stat, min_stat, min_stat2;
   gss_buffer_desc input_token;
   gss_buffer_desc output_token;
   size_t i, len;
   int ret;

   if (connection->bufsize > 0) {
      len = (connection->bufsize < bufsize) ? connection->bufsize : bufsize;
      memcpy(buf, connection->buffer, len);
      if (connection->bufsize - len == 0) {
	 free(connection->buffer);
	 connection->buffer = NULL;
      } else {
	 for (i = 0; i < connection->bufsize - len; i++)
	    connection->buffer[i] = connection->buffer[i+len];
      }
      connection->bufsize -= len;

      return len;
   }

   do {
      ret = recv_gss_token(connection->sock, connection->authn_mech,
		       &input_token.value, &input_token.length,
	               timeout);
      if (ret)
	 return ret;

      /* work around a Globus bug */
      ERR_clear_error();

      maj_stat = gss_unwrap(&min_stat, connection->context, &input_token,
	  		    &output_token, NULL, NULL);
      gss_release_buffer(&min_stat2, &input_token);
      if (GSS_ERROR(maj_stat)) {
	 if (gss_code) {
           gss_code->minor_status = min_stat;
           gss_code->major_status = maj_stat;
         }
	 return EDG_WLL_GSS_ERROR_GSS;
      }
   } while (maj_stat == 0 && output_token.length == 0 && output_token.value == NULL);

   if (output_token.length > bufsize) {
      connection->bufsize = output_token.length - bufsize;
      connection->buffer = malloc(connection->bufsize);
      if (connection->buffer == NULL) {
	 connection->bufsize = 0;
	 ret = EDG_WLL_GSS_ERROR_ERRNO;
	 goto end;
      }
      memcpy(connection->buffer, output_token.value + bufsize, connection->bufsize);
      output_token.length = bufsize;
   }

   memcpy(buf, output_token.value, output_token.length);
   ret = output_token.length;

end:
   gss_release_buffer(&min_stat, &output_token);

   return ret;
}

/** Read data from the opened connection, repeat reading up to 'bufsize' or end of the stream. */
int
edg_wll_gss_read_full(edg_wll_GssConnection *connection, void *buf, 
       		      size_t bufsize, struct timeval *timeout, size_t *total,
		      edg_wll_GssStatus* gss_code)
{
   size_t	len, i;
   *total = 0;

   if (connection->bufsize > 0) {
      len = (connection->bufsize < bufsize) ? connection->bufsize : bufsize;
      memcpy(buf, connection->buffer, len);
      if (connection->bufsize - len == 0) {
	 free(connection->buffer);
	 connection->buffer = NULL;
      } else {
         for (i = 0; i < connection->bufsize - len; i++)
            connection->buffer[i] = connection->buffer[i+len];
      }
      connection->bufsize -= len;
      *total = len;
   }

   while (*total < bufsize) {
      int len;

      len = edg_wll_gss_read(connection, buf+*total, bufsize-*total,
	                     timeout, gss_code);
      if (len < 0) return len;
      *total += len;
   }

   return 0;
}

/** Send data over the opened connection. */
int
edg_wll_gss_write_full(edg_wll_GssConnection *connection, const void *buf,
                       size_t bufsize, struct timeval *timeout, size_t *total,
		       edg_wll_GssStatus* gss_code)
{
   return edg_wll_gss_write(connection, buf, bufsize, timeout, gss_code);
}

/** Request credential reload each 60 seconds in order to work around
 * Globus bug (not reloading expired CRLs)
 */
#define GSS_CRED_WATCH_LIMIT  60
int
edg_wll_gss_watch_creds(const char *proxy_file, time_t *last_time)
{
      struct stat     pstat;
      time_t  now;

      now = time(NULL);

      /* to work around a globus bug we enforce reloading credentials
	 quite often */
      if ( now >= (*last_time+GSS_CRED_WATCH_LIMIT) ) {
              *last_time = now;
              return 1;
      }

      if (!proxy_file) return 0;
      if (stat(proxy_file,&pstat)) return -1;

      if ( pstat.st_mtime >= *last_time ) {
              *last_time = now + 1;
              return 1;
      }

      return 0;
}

/** Close the connection. */
int
edg_wll_gss_watch_creds_gsi(const char *proxy_file, time_t *last_time)
{
    return edg_wll_gss_watch_creds(proxy_file, last_time);
}

/** Close the connection. */
int
edg_wll_gss_close(edg_wll_GssConnection *con, struct timeval *timeout)
{
   OM_uint32 min_stat;
   gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;
   /*struct timeval def_timeout = { 0, 100000};*/

   if (con->context != GSS_C_NO_CONTEXT) {
      gss_delete_sec_context(&min_stat, (gss_ctx_id_t *)&con->context, &output_token);

#if 0
      /* XXX: commented out till timeout handling in caller is fixed */

      /* send the buffer (if any) to the peer. GSSAPI specs doesn't
       * recommend sending it, but we want SSL compatibility */
      if (output_token.length && con->sock>=0) {
              send_token(con->sock, output_token.value, output_token.length,
                              timeout ? timeout : &def_timeout);
      }
#endif
      gss_release_buffer(&min_stat, &output_token);

      /* XXX can socket be open even if context == GSS_C_NO_CONTEXT) ? */
      /* XXX ensure that edg_wll_GssConnection is created with sock set to -1 */
      if (con->sock >= 0)
	 close(con->sock);
   }
   if (con->buffer)
      free(con->buffer);
   memset(con, 0, sizeof(*con));
   con->context = GSS_C_NO_CONTEXT;
   con->sock = -1;
   con->authn_mech = NULL;
   return 0;
}

/** Get error details. */
int
edg_wll_gss_get_error(edg_wll_GssStatus *gss_err, const char *prefix, char **msg)
{
   OM_uint32 maj_stat, min_stat;
   OM_uint32 msg_ctx = 0;
   gss_buffer_desc maj_status_string = GSS_C_EMPTY_BUFFER;
   gss_buffer_desc min_status_string = GSS_C_EMPTY_BUFFER;
   char *str = NULL;
   char *line, *tmp;

   str = strdup(prefix);
   do {
      maj_stat = gss_display_status(&min_stat, gss_err->major_status,
      				    GSS_C_GSS_CODE, GSS_C_NO_OID,
				    &msg_ctx, &maj_status_string);
      if (GSS_ERROR(maj_stat))
	 break;
      maj_stat = gss_display_status(&min_stat, gss_err->minor_status,
	    			    GSS_C_MECH_CODE, GSS_C_NULL_OID,
				    &msg_ctx, &min_status_string);
      if (GSS_ERROR(maj_stat)) {
	 gss_release_buffer(&min_stat, &maj_status_string);
	 break;
      }

      asprintf(&line, ": %s (%s)", (char *)maj_status_string.value,
	       (char *)min_status_string.value);
      gss_release_buffer(&min_stat, &maj_status_string);
      gss_release_buffer(&min_stat, &min_status_string);

      tmp = realloc(str, strlen(str) + strlen(line) + 1);
      if (tmp == NULL) {
         /* abort() ? */
	 free(line);
	 free(str);
	 str = "WARNING: Not enough memory to produce error message";
	 break;
      }
      str = tmp;
      strcat(str, line);
      free(line);
   } while (!GSS_ERROR(maj_stat) && msg_ctx != 0);

   *msg = str;
   return 0;
}

int
edg_wll_gss_oid_equal(const gss_OID a, const gss_OID b)
{
   if (a == b)
      return 1;
   else {
      if (a == GSS_C_NO_OID || b == GSS_C_NO_OID || a->length != b->length)
	 return 0;
      else
	 return (memcmp(a->elements, b->elements, a->length) == 0);
   }
}

int 
edg_wll_gss_reject(int sock)
{
   /* XXX is it possible to cut & paste edg_wll_ssl_reject() ? */
   return 0;
}


/**
 * Initialize routine of glite gss module.
 * It activates globus modules, and it should be called before using other gss routines.
 */
int
edg_wll_gss_initialize(void)
{
   int ret = 0;
   int index;

#ifdef GLITE_LBU_THREADED
   pthread_mutex_lock(&init_lock);
#endif
   setenv("GLOBUS_THREAD_MODEL", "pthread", 0);
   setenv("GLOBUS_GSSAPI_NAME_COMPATIBILITY", "STRICT_RFC2818", 0);

#ifndef NO_GLOBUS_GSSAPI
   if (globus_module_activate(GLOBUS_GSI_GSSAPI_MODULE) != GLOBUS_SUCCESS) {
      errno = EINVAL;
      ret = EDG_WLL_GSS_ERROR_ERRNO;
   }
#endif

   if (globus_module_activate(GLOBUS_COMMON_MODULE) == GLOBUS_SUCCESS)
	globus_common_activated = 1;

   // some pre-initializations (workarounds thread-safe problem
   // in gss_init_sec_context)
#ifndef NO_GLOBUS_GSSAPI
   globus_gsi_callback_get_SSL_callback_data_index(&index);
#endif

#ifdef GLITE_LBU_THREADED
   pthread_mutex_unlock(&init_lock);
#endif

   return ret;
}


/**
 * Clean up routine of gss module.
 * It can be called after using gss routines to free initializeted resources.
 */
void
edg_wll_gss_finalize(void)
{
#ifdef GLITE_LBU_THREADED
   pthread_mutex_lock(&init_lock);
#endif
#ifndef NO_GLOBUS_GSSAPI
   globus_module_deactivate(GLOBUS_GSI_GSSAPI_MODULE);
#endif
   if (globus_common_activated) {
      globus_module_deactivate(GLOBUS_COMMON_MODULE);
      globus_common_activated = 0;
   }
#ifdef GLITE_LBU_THREADED
   pthread_mutex_unlock(&init_lock);
#endif
}


/**
 * Release the acquired credentials.
 */
int
edg_wll_gss_release_cred(edg_wll_GssCred *cred, edg_wll_GssStatus* gss_code)
{
   OM_uint32 maj_stat, min_stat;
   int ret = 0;

   if (gss_code)
      gss_code->major_status = gss_code->minor_status = 0;

   if (cred == NULL || *cred == NULL)
      return ret;

   if ((*cred)->gss_cred) {
      maj_stat = gss_release_cred(&min_stat, (gss_cred_id_t*)&(*cred)->gss_cred); 
      if (GSS_ERROR(maj_stat)) {
         ret = EDG_WLL_GSS_ERROR_GSS;
         if (gss_code) {
            gss_code->major_status = maj_stat;
            gss_code->minor_status = min_stat;
         }
      }
   }

   if ((*cred)->name)
      free((*cred)->name);

   free(*cred);
   *cred = NULL;

   return ret;
}

/**
 * Get information about the the connection - principal (display name).
 */
int
edg_wll_gss_get_client_conn(edg_wll_GssConnection *connection,
                            edg_wll_GssPrincipal *principal,
			    edg_wll_GssStatus* gss_code)
{
   gss_buffer_desc token = GSS_C_EMPTY_BUFFER;
   OM_uint32 maj_stat, min_stat, ctx_flags;
   gss_name_t client_name = GSS_C_NO_NAME;
   edg_wll_GssPrincipal p;
   int ret;

   maj_stat = gss_inquire_context(&min_stat, connection->context, &client_name,
                                  NULL, NULL, NULL, &ctx_flags, NULL, NULL);
   if (GSS_ERROR(maj_stat))
      goto end;

   maj_stat = gss_display_name(&min_stat, client_name, &token, NULL);
   if (GSS_ERROR(maj_stat))
      goto end;

   p = calloc(1, sizeof(*p));
   if (p == NULL) {
      errno = ENOMEM;
      ret = EDG_WLL_GSS_ERROR_ERRNO;
      goto end;
   }

   p->name = strdup(token.value);
   p->flags = ctx_flags;

   *principal = p;
   ret = 0;

end:
   if (GSS_ERROR(maj_stat)) {
      ret = EDG_WLL_GSS_ERROR_GSS;
      if (gss_code) {
         gss_code->major_status = maj_stat;
         gss_code->minor_status = min_stat;
      }
   }
	
   if (token.length)
      gss_release_buffer(&min_stat, &token);
   if (client_name != GSS_C_NO_NAME)
      gss_release_name(&min_stat, &client_name);

   return ret;
}

/* Beware, this call manipulates with environment variables and is not
   thread-safe */
static int
get_peer_cred(edg_wll_GssConnection *gss, const char *my_cert_file,
              const char *my_key_file, STACK_OF(X509) **chain,
              edg_wll_GssStatus* gss_code)
{
   OM_uint32 maj_stat, min_stat;
   gss_buffer_desc buffer = GSS_C_EMPTY_BUFFER;
   BIO *bio = NULL;
   SSL_SESSION *session = NULL;
   unsigned char int_buffer[4];
   long length;
   int ret, index;
   STACK_OF(X509) *cert_chain = NULL;
   X509 *p_cert;
   char *orig_cert = NULL, *orig_key = NULL;

   if (!edg_wll_gss_oid_equal(gss->authn_mech, get_oid("GSI")))
       return EINVAL;

   maj_stat = gss_export_sec_context(&min_stat, (gss_ctx_id_t *) &gss->context,
				     &buffer);
   if (GSS_ERROR(maj_stat)) {
      if (gss_code) {
         gss_code->major_status = maj_stat;
         gss_code->minor_status = min_stat;
      }
      return EDG_WLL_GSS_ERROR_GSS;
   }

   /* The GSSAPI specs requires gss_export_sec_context() to destroy the
    * context after exporting. So we have to resurrect the context here by
    * importing from just generated buffer. gss_import_sec_context() must be
    * able to read valid credential before it loads the exported context
    * so we set the environment temporarily to point to the ones used by
    * the server.
    * */

#ifdef GLITE_LBU_THREADED
   // to protect the environment
   // XXX: only partial fix, every getenv() can still cause race-condition
   pthread_mutex_lock(&init_lock);
#endif
   orig_cert = getenv("X509_USER_CERT");
   orig_key = getenv("X509_USER_KEY");

   if (my_cert_file)
      setenv("X509_USER_CERT", my_cert_file, 1);
   if (my_key_file)
      setenv("X509_USER_KEY", my_key_file, 1);
   
   maj_stat = gss_import_sec_context(&min_stat, &buffer,
				     (gss_ctx_id_t *)&gss->context);

   if (orig_cert)
       setenv("X509_USER_CERT", orig_cert, 1);
   else
       unsetenv("X509_USER_CERT");

   if (orig_key) 
       setenv("X509_USER_KEY", orig_key, 1);
   else 
       unsetenv("X509_USER_KEY");
#ifdef GLITE_LBU_THREADED
   pthread_mutex_unlock(&init_lock);
#endif

   if (GSS_ERROR(maj_stat)) {
      if (gss_code) {
         gss_code->major_status = maj_stat;
         gss_code->minor_status = min_stat;
      }
      ret = EDG_WLL_GSS_ERROR_GSS;
      goto end;
   }

   bio = BIO_new(BIO_s_mem());
   if (bio == NULL) {
      ret = ENOMEM;
      goto end;
   }
   
   /* Store exported context to memory, skipping the version number and and cred_usage fields */
   BIO_write(bio, buffer.value + 8 , buffer.length - 8);

   /* decode the session data in order to skip at the start of the cert chain */
   session = d2i_SSL_SESSION_bio(bio, NULL);
   if (session == NULL) {
      ret = EINVAL;
      goto end;
   }
   SSL_SESSION_free(session);

   cert_chain = sk_X509_new_null();

   BIO_read(bio, (char *) int_buffer, 4);
   length  = (((size_t) int_buffer[0]) << 24) & 0xffff;
   length |= (((size_t) int_buffer[1]) << 16) & 0xffff;
   length |= (((size_t) int_buffer[2]) <<  8) & 0xffff;
   length |= (((size_t) int_buffer[3])      ) & 0xffff;

   if (length == 0) {
      ret = EINVAL;
      goto end;
   }

   for(index = 0; index < length; index++) {
      p_cert = d2i_X509_bio(bio, NULL);
      if (p_cert == NULL) {
	 ret = EINVAL;
	 sk_X509_pop_free(cert_chain, X509_free);
	 goto end;
      }

      sk_X509_push(cert_chain, p_cert);
   }

   *chain = cert_chain;
   ret = 0;

end:
   gss_release_buffer(&min_stat, &buffer);

   return ret;
}

/**
 * Get information about the the connection - pem string.
 */
int
edg_wll_gss_get_client_pem(edg_wll_GssConnection *connection,
                           const char *my_cert_file, const char *my_key_file,
                           char **pem_string)
{
   char *tmp = NULL;
   STACK_OF(X509) *chain = NULL;
   BIO *bio = NULL;
   int ret, i;
   size_t total_len, l;

   ret = get_peer_cred(connection, my_cert_file, my_key_file, &chain, NULL);
   if (ret)
      return ret;

   bio = BIO_new(BIO_s_mem());
   if (bio == NULL) {
      ret = ENOMEM;
      goto end;
   }

   for (i = 0; i < sk_X509_num(chain); i++) {
      ret = PEM_write_bio_X509(bio, sk_X509_value(chain, i));
      if (ret <= 0) {
	 ret = EINVAL;
	 goto end;
      }
   }

   total_len = BIO_pending(bio);
   if (total_len <= 0) {
      ret = EINVAL;
      goto end;
   }

   tmp = malloc(total_len + 1);
   if (tmp == NULL) {
      ret = ENOMEM;
      goto end;
   }

   l = 0;
   while (l < total_len) {
      ret = BIO_read(bio, tmp+l, total_len-l);
      if (ret <= 0) {
	 ret = EINVAL;
	 goto end;
      }
      l += ret;
   }

   tmp[total_len] = '\0';
   *pem_string = tmp;
   tmp = NULL;

   ret = 0;

end:
   sk_X509_pop_free(chain, X509_free);
   if (bio)
      BIO_free(bio);
   if (tmp)
      free(tmp);

   return ret;
}

/**
 * Free the principal.
 */
void
edg_wll_gss_free_princ(edg_wll_GssPrincipal principal)
{
   if (principal == NULL)
      return;

   if (principal->name)
      free(principal->name);

   free(principal);
}

/**
 * Get the hostname (using globus call if possible, or system's gethostbyname() if globus is not initialized).
 */
int
edg_wll_gss_gethostname(char *name, int len)
{
   int ret;

#ifdef GLITE_LBU_THREADED
   pthread_mutex_lock(&init_lock);
#endif
   if (globus_common_activated)
      ret = globus_libc_gethostname(name, len);
   else
      ret = gethostname(name, len);
#ifdef GLITE_LBU_THREADED
   pthread_mutex_unlock(&init_lock);
#endif

   return ret;
}

/**
 * Normalize subject name (stripping email address, /CN=proxy, ...).
 */
char *
edg_wll_gss_normalize_subj(char *in, int replace_in)
{
	char *new, *ptr;
	size_t len;

	if (in == NULL) return NULL;
	if (replace_in) 
		new = in;
	else
		new = strdup(in);
	
	while ((ptr = strstr(new, "/emailAddress="))) {
		memcpy(ptr, "/Email=",7);
		memmove(ptr+7, ptr+14, strlen(ptr+14)+1);
	}
	
	len = strlen(new);
	while (len > 9 && !strcmp(new+len-9, "/CN=proxy")) {
		*(new+len-9) = '\0';
		len -= 9;
	}

	return new;
}

/**
 * Compare subject names.
 */
int
edg_wll_gss_equal_subj(const char *a, const char *b)
{
	char *an,*bn;
	int res;

	an = edg_wll_gss_normalize_subj((char*)a, 0);
	bn = edg_wll_gss_normalize_subj((char*)b, 0);

	if (!an || !bn)
		res = 0;
	else 
		res = !strcasecmp(an,bn);
	
	free(an); free(bn);
	return res;
}

/**
 * Return data to the reading buffer.
 */
int
edg_wll_gss_unread(edg_wll_GssConnection *con, void *data, size_t len)
{
   char *tmp;

   if (len == 0)
       return 0;

   tmp = malloc(len + con->bufsize);
   if (tmp == NULL)
       return ENOMEM;

   memcpy(tmp, data, len);
   if (con->bufsize > 0)
       memcpy(tmp + len, con->buffer, con->bufsize);

   free(con->buffer);
   con->buffer = tmp;
   con->bufsize += len;

   return 0;
}


/**
 * Signal handler compatible with globus.
 * It is required to use this function instead of sigaction(), when using threaded globus flavour.
 *
 * As for many other gss routenes, edg_wll_initialize() must be called before using this routine.
 * edg_wll_gss_set_signal_handler() will falback to sigaction() if gss is not initialized.
 *
 * @see edg_wll_initialize
 */
int
edg_wll_gss_set_signal_handler(int signum,
			       void (*handler_func)(int))
{
   int ret;
   intptr_t signum2; 

#ifdef GLITE_LBU_THREADED
   pthread_mutex_lock(&init_lock);
#endif
   if (!globus_common_activated) {
	   struct sigaction	sa,osa;
	   
	   memset(&sa, 0, sizeof(sa));
	   sigemptyset(&sa.sa_mask);
	   sa.sa_handler = handler_func;
	   ret = sigaction(signum, &sa, &osa);
   } else {
	   signum2 = signum;
	   ret = globus_callback_space_register_signal_handler(signum,
						       GLOBUS_TRUE,
						       (globus_callback_func_t)handler_func,
						       (void *)signum2,
						       GLOBUS_CALLBACK_GLOBAL_SPACE);
   }
#ifdef GLITE_LBU_THREADED
   pthread_mutex_unlock(&init_lock);
#endif

   return ret;
}


/**
 * Check posix signals and performs signal handlers eventually.
 * Required when using non-threaded globus flavour.
 */
void
edg_wll_gss_poll_signal() {
	globus_poll_nonblocking();
}
