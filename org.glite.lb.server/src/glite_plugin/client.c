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
   fprintf(stderr,"usage: %s [option] hostname\n"
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
doit(gss_cred_id_t cred, const char *hostname, short port)
{
   int ret;
   struct timeval timeout = {10,0};
   edg_wll_GssConnection con;
   char *msg;
   edg_wll_GssStatus gss_code;

   ret = edg_wll_gss_connect(cred, hostname, port, &timeout, &con, &gss_code);
   if (ret) {
      edg_wll_gss_get_error(&gss_code, "Error connecting to server", &msg);
      fprintf(stderr, "%s\n", msg);
      free(msg);
      return ret;
   }

   ret = proto(&con);

   edg_wll_gss_close(&con, NULL);

   return 0;
}

int
main(int argc, char *argv[])
{
   char *cert, *key, *cadir;
   const char *hostname = "localhost";
   short port = 3322;
   const char *name;
   int ret, opt;
   gss_cred_id_t cred = GSS_C_NO_CREDENTIAL;
   edg_wll_GssStatus gss_code;
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

   ret = edg_wll_gss_acquire_cred_gsi(cert, key, &cred, NULL, &gss_code);
   if (ret) {
      edg_wll_gss_get_error(&gss_code, "Failed to read credential", &msg);
      fprintf(stderr, "%s\n", msg);
      free(msg);
      exit(1);
   }

   ret = doit(cred, hostname, port);

   gss_release_cred(&min_stat, &cred);
   return ret;
}
