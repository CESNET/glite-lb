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
#include <stdsoap2.h>

#include "soap_version.h"
#include <glite/security/glite_gscompat.h>

int main() {
	printf("sizeof(struct soap) = %lu, original size = %lu\n",
		sizeof(struct soap), (size_t)GSOAP_SIZEOF_STRUCT_SOAP);

	return !(sizeof(struct soap) == GSOAP_SIZEOF_STRUCT_SOAP);
}
