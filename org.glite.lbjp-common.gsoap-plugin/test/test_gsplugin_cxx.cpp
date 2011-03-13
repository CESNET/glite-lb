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

#include <iostream>

#include "stdsoap2.h"
#include "glite_gsplugin.h"

#define TEST_STR "123456789ABCDEF"

int main() {
	struct soap *mydlo = NULL;
	glite_gsplugin_Context gsplugin_ctx = NULL;
	int ok1;

	// test 1 - stdsoap2.c compatibility
	if ((mydlo = soap_new()) == NULL) {
		std::cerr << "Couldn't create soap" << std::endl;
		return 1;
	}
	soap_set_endpoint(mydlo, TEST_STR);
	std::cout << mydlo->endpoint << std::endl;
	ok1 = strcmp(mydlo->endpoint, TEST_STR);

	// test 2 - glite_gsplugin.c compatibility
	//
	// not real test, just may crash in bad test case on calling
	// soap->fdelete where will be other function
	if ( glite_gsplugin_init_context(&gsplugin_ctx) ) {
		std::cerr << "Couldn't create gSOAP plugin context" << std::endl;
		goto err;
	}
	if (soap_register_plugin_arg(mydlo, glite_gsplugin, gsplugin_ctx)) {
		std::cerr << "Couldn't register gSoap plugin" << std::endl;
		goto err;
	}

	soap_done(mydlo);
	free(mydlo);
	glite_gsplugin_free_context(gsplugin_ctx);
	return ok1;

err:
	if (gsplugin_ctx) glite_gsplugin_free_context(gsplugin_ctx);
	if (mydlo) soap_destroy(mydlo);
	return 1;
}

Namespace namespaces[] = {{NULL, NULL, NULL, NULL}};
