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

#include "pretty_print_wrapper.h"

#include <string>
#include <string.h>
#include <classad_distribution.h>

int pretty_print(char *jdl, char **formated_print){
	CLASSAD_NAMESPACE ClassAd        *classad;
	CLASSAD_NAMESPACE ClassAdParser  parser;

	classad = parser.ParseClassAd(std::string(jdl), true);
	if (! classad){
		*formated_print = NULL;
		return -1;	// not ClassAd data
	}

	CLASSAD_NAMESPACE PrettyPrint	pp;
	std::string buf;
	pp.Unparse(buf, classad);
	*formated_print = strdup(buf.c_str());

	return 0;
}

