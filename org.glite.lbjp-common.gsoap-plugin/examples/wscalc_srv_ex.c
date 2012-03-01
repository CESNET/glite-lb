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

#include "GSOAP_H.h"
#include "CalcService.nsmap"


static struct option long_options[] = {
	{ "cert",    required_argument,      NULL,   'c' },
	{ "key",     required_argument,      NULL,   'k' },
	{ NULL, 0, NULL, 0 }
};

void
usage(const char *me)
{
	fprintf(stderr,
			"usage: %s [option]\n"
			"\t-c, --cred\t certificate file\n"
			"\t-k, --key\t private key file\n", me);
}


int
main(int argc, char **argv)
{
	struct soap				soap;
	glite_gsplugin_Context	ctx = NULL;
	char				   *name;
	char				   *cert, *key;
	int						opt;

	cert = key = NULL;
	name = strrchr(argv[0],'/');
	if ( name ) name++; else name = argv[0];

	while ((opt = getopt_long(argc, argv, "c:k:", long_options, NULL)) != EOF) {
		switch (opt) {
		case 'c': cert = optarg; break;
		case 'k': key = optarg; break;
		case '?':
		default : usage(name); exit(1);
		}
	}

	soap_init(&soap);
	soap_set_namespaces(&soap, namespaces);

	if ( soap_register_plugin_arg(&soap, glite_gsplugin, NULL) ) {
		fprintf(stderr, "Can't register plugin\n");
		exit(1);
	}

	if ( cert || key ) {
		ctx = glite_gsplugin_get_context(&soap);
		if (ctx == NULL) {
			fprintf(stderr, "Can't get context\n");
			exit(1);
		}

		if (glite_gsplugin_set_credential(ctx, cert, key) != 0) {
		    fprintf(stderr, "Can't set credentials: %s\n",
			    glite_gsplugin_errdesc(&soap));
 		    exit(1);
		}
	}

	if ( soap_bind(&soap, NULL, 19999, 100) < 0 ) {
		soap_print_fault(&soap, stderr);
		exit(1);
	}

	while ( 1 ) {
		printf("accepting connection\n");
		if ( soap_accept(&soap) < 0 ) {
			fprintf(stderr, "soap_accept() failed!!!\n");
			soap_print_fault(&soap, stderr);
			fprintf(stderr, "plugin err: %s\n", glite_gsplugin_errdesc(&soap));
			continue;
		}

		printf("serving connection\n");
		if ( soap_serve(&soap) ) {
			soap_print_fault(&soap, stderr);
			fprintf(stderr, "plugin err: %s", glite_gsplugin_errdesc(&soap));
		}

		soap_destroy(&soap); /* clean up class instances */
		soap_end(&soap); /* clean up everything and close socket */
	}
	soap_done(&soap); /* close master socket */

	if ( ctx ) glite_gsplugin_free_context(ctx);

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
