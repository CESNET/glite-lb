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

#include <glite_gsplugin.h>

#include "GSOAP_H.h"
#include "CalcService.nsmap"

static const char *server = "http://localhost:19999/";

int
main(int argc, char **argv)
{	
	struct soap		soap;
	double			a, b, result;
	int				ret;
	struct wscalc__addResponse	addresult;
	struct wscalc__subResponse	subresult;


	if (argc < 4) {
		fprintf(stderr, "Usage: [add|sub] num num\n");
		exit(1);
	}

	soap_init(&soap);
	soap_set_namespaces(&soap, namespaces);
	soap_register_plugin(&soap, glite_gsplugin);

	a = strtod(argv[2], NULL);
	b = strtod(argv[3], NULL);
	switch ( *argv[1] ) {
	case 'a':
		ret = soap_call_wscalc__add(&soap, server, "", a, b, &addresult);
		result = addresult.result;
		break;
	case 's':
		ret = soap_call_wscalc__sub(&soap, server, "", a, b, &subresult);
		result = subresult.result;
		break;
	default:
		fprintf(stderr, "Unknown command\n");
		exit(2);
	}

	if ( ret ) {
		fprintf(stderr, "NECO JE V ****\n\n");
		fprintf(stderr, "plugin err: %s", glite_gsplugin_errdesc(&soap));
		soap_print_fault(&soap, stderr);
	}
	else printf("result = %g\n", result);


	return 0;
}
