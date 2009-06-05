#include "pretty_print_wrapper.h"

#include <classad_distribution.h>
#include <string>
#include <string.h>

int pretty_print(char *jdl, char **formated_print){
	ClassAd        *classad;
	ClassAdParser  parser;

	classad = parser.ParseClassAd(std::string(jdl), true);
	if (! classad){
		*formated_print = NULL;
		return -1;	// not ClassAd data
	}

	PrettyPrint	pp;
	std::string buf;
	pp.Unparse(buf, classad);
	*formated_print = strdup(buf.c_str());

	return 0;
}

