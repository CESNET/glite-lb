#ident "$Header$"

#include <stdio.h>
#include <getopt.h>
#include <stdsoap2.h>

#include "ws_plugin.h"

static struct option long_options[] = {
	{ "cert",    required_argument,      NULL,   'c' },
	{ "key",     required_argument,      NULL,   'k' },
	{ "CAdir",   required_argument,      NULL,   'd' },
	{ NULL, 0, NULL, 0 }
};

void
usage(const char *me)
{
   fprintf(stderr,"usage: %s [option]\n"
	   "\t-c, --cred\t certificate file\n"
	   "\t-k, --key\t private key file\n"
	   "\t-d, --CAdir\t trusted certificates directory\n"
	   , me);
}

int
proto (edg_wll_GssConnection *con)
{
   return 0;
}

int
doit(int socket, gss_cred_id_t cred)
{
   int ret;
   struct timeval timeout = {10,0};
   edg_wll_GssConnection con;
   edg_wll_GssStatus gss_code;
   char *msg;

   ret = edg_wll_gss_accept(cred, socket, &timeout, &con, &gss_code);
   if (ret) {
      edg_wll_gss_get_error(&gss_code, "Failed to read credential", &msg);
      fprintf(stderr, "%s\n", msg);
      free(msg);
      return ret;
   }

   ret = proto(&con);

   edg_wll_gss_close(&con, &timeout);

   return ret;
}

int
main(int argc, char *argv[])
{
   const char *name;
   int ret, opt;
   int listener_fd;
   int client_fd;
   struct sockaddr_in client_addr;
   int client_addr_len;
   gss_cred_id_t cred = GSS_C_NO_CREDENTIAL;
   char *cert, *key, *cadir;
   edg_wll_GssStatus gss_code;
   char *subject = NULL;
   char *msg;
   OM_uint32 min_stat;

   cert = key = cadir = NULL;

   name = strrchr(argv[0],'/');
   if (name) name++; else name = argv[0];

   while ((opt = getopt_long(argc, argv, "c:k:d:", long_options, NULL)) != EOF) {
      switch (opt) {
	 case 'c': cert = optarg; break;
	 case 'k': key = optarg; break;
	 case 'd': cadir = optarg; break;
	 case '?':
	 default : usage(name); exit(1);
      }
   }

   ret = edg_wll_gss_acquire_cred_gsi(cert, key, &cred, &subject, &gss_code);
   if (ret) {
      edg_wll_gss_get_error(&gss_code, "Failed to read credential", &msg);
      fprintf(stderr, "%s\n", msg);
      free(msg);
      exit(1);
   }

   if (subject) {
      printf("server running with certificate: %s\n", subject);
      free(subject);
   }

   client_addr_len = sizeof(client_addr);
   bzero((char *) &client_addr, client_addr_len);

   while (1) {
      client_fd = accept(listener_fd, (struct sockaddr *) &client_addr,
	    		 &client_addr_len);
      if (client_fd < 0) {
	 close(listener_fd);
	 perror("accept");
	 gss_release_cred(&min_stat, &cred);
	 exit(1);
      }

      ret = doit(client_fd, cred);
      close(client_fd);
   }
}
