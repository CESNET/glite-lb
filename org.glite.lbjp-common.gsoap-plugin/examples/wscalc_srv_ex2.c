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
#include <getopt.h>
#include <stdsoap2.h>
#include <glite_gsplugin.h>
#include <glite_gsplugin-int.h>

#include "GSOAP_H.h"
#include "CalcService.nsmap"


static struct option long_options[] = {
	{ "cert",    required_argument,      NULL,   'c' },
	{ "key",     required_argument,      NULL,   'k' },
	{ "port",    required_argument,      NULL,   'p' },
	{ NULL, 0, NULL, 0 }
};

void
usage(const char *me)
{
	fprintf(stderr,
			"usage: %s [option]\n"
			"\t-p, --port\t listening port\n"
			"\t-c, --cred\t certificate file\n"
			"\t-k, --key\t private key file\n", me);
}


int
main(int argc, char **argv)
{
	struct soap				soap;
	edg_wll_GssStatus		gss_code;
	edg_wll_GssCred			cred = NULL;
	edg_wll_GssConnection		connection;
	glite_gsplugin_Context		ctx;
	struct sockaddr_storage		a;
	int				alen;
	char			 	 *name, *msg;
	int			 	opt,
					port = 19999;
	char				*cert_filename = NULL, *key_filename = NULL;
	int				sock;
	struct addrinfo 		hints;
	struct addrinfo 		*res;
	char 				*portname;
  	int 				error;

	name = strrchr(argv[0],'/');
	if ( name ) name++; else name = argv[0];

	if ( glite_gsplugin_init_context(&ctx) ) { perror("init context"); exit(1); }

	while ((opt = getopt_long(argc, argv, "c:k:p:", long_options, NULL)) != EOF) {
		switch (opt) {
		case 'p': port = atoi(optarg); break;
		case 'c': cert_filename = strdup(optarg); break;
		case 'k': key_filename = strdup(optarg); break;
		case '?':
		default : usage(name); exit(1);
		}
	}

	if ( edg_wll_gss_acquire_cred_gsi(cert_filename, key_filename, &cred, &gss_code) ) {
		edg_wll_gss_get_error(&gss_code, "Failed to read credential", &msg);
		fprintf(stderr, "%s\n", msg);
		free(msg);
		exit(1);
	}
	if (cred->name) {
		printf("server running with certificate: %s\n", cred->name);
	}

	glite_gsplugin_use_credential(ctx, cred);

	soap_init(&soap);
	soap_set_namespaces(&soap, namespaces);

	if ( soap_register_plugin_arg(&soap, glite_gsplugin, ctx) ) {
		fprintf(stderr, "Can't register plugin\n");
		exit(1);
	}

	asprintf(&portname, "%d", port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(NULL, portname , &hints, &res);
	if (error) {
		perror(gai_strerror(error));
		exit(1);
	}
	alen = res->ai_addrlen;
	if ( (sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0 )
		{ perror("socket()"); exit(1); }
	if ( bind(sock, res->ai_addr, res->ai_addrlen) ) { perror("bind()"); exit(1); }
	freeaddrinfo(res);
	if ( listen(sock, 100) ) { perror("listen()"); exit(1); }

	bzero((char *) &a, alen);

	while ( 1 ) {
		int conn;

		printf("accepting connection\n");
		if ( (conn = accept(sock, (struct sockaddr *) &a, &alen)) < 0 ) {
			close(sock);
			perror("accept");
			exit(1);
		}
		if ( edg_wll_gss_accept(cred,conn,ctx->timeout,&connection,&gss_code) ){
			edg_wll_gss_get_error(&gss_code, "Failed to read credential", &msg);
			fprintf(stderr, "%s\n", msg);
			free(msg);
			exit(1);
		}

		glite_gsplugin_set_connection(ctx, &connection); 

		printf("serving connection\n");
		if ( soap_serve(&soap) ) {
			soap_print_fault(&soap, stderr);
			fprintf(stderr, "plugin err: %s", glite_gsplugin_errdesc(&soap));
		}

		soap_destroy(&soap); /* clean up class instances */
		soap_end(&soap); /* clean up everything and close socket */
		edg_wll_gss_close(&connection, NULL);
	}

	soap_done(&soap); /* close master socket */

	glite_gsplugin_free_context(ctx);
	edg_wll_gss_release_cred(&cred, NULL);

	return 0;
} 

int wscalc__add(struct soap *soap, double a, double b, struct wscalc__addResponse *result)
{
	result->result = a + b;
	return SOAP_OK;
} 

int wscalc__sub(struct soap *soap, double a, double b, struct wscalc__subResponse *result)
{
	result->result = a - b;
	return SOAP_OK;
} 
