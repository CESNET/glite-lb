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

/*
@@@AUTO
*/
@@@LANG: C

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "glite/lbu/trio.h"

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/file_plugin.h"
#include "glite/jp/attr.h"
#include "glite/lb/job_attrs2.h"

/*
 * free the array of JP attr
 */
static void free_attrs(glite_jp_attrval_t *av) {
        glite_jp_attrval_t *item;

        item = av;
        while (item->name) {
                glite_jp_attrval_free(item++, 0);
        }
        free(av);
}

/*
 * process attributes
 */
void process_attrs2(glite_jp_context_t jpctx, glite_jpps_fplug_data_t plugin_data, void *data_handle, FILE *outfile) {

	glite_jp_attrval_t *attrval;

	/* <header> */
	fprintf(outfile,"<?xml version=\"1.0\"?>\n\n");
	fprintf(outfile,"<lbd:jobRecord\n");
	fprintf(outfile,"xmlns:lbd=\"http://glite.org/wsdl/types/lbdump\"\n");
	
	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_jobId, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"jobid=\"%|Xs\"\n", attrval->value);
		free_attrs(attrval);
	} else {
		fprintf(outfile,"jobid=\"default\"\n");
	}
	fprintf(outfile,">\n");
	/* </header> */
	
	/* <body> */
@@@{
# elems
        for my $n ($status->getAllFieldsOrdered) {
                my $f = selectField $status $n;
#                my $type = getType $f;
		my $type = $f->{type};
                my $name = getName $f;

#                $type = 'jobtypeType' if $name eq 'jobtype';

                @ln = split /_/,$name;
                my $un = shift @ln; $un .= ucfirst shift @ln while (@ln);
                my $c = getComment $f;
		my $strformat = "%s";
		if ($type eq 'string') { $strformat = "%|Xs"; }

                gen qq{
	/** $c */
        plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_$un, &attrval);
        if (attrval) {
                trio_fprintf(outfile,"<$un>$strformat</$un>\\n", attrval->value);
                free_attrs(attrval);
        }
};

        }
@@@}
	fprintf(outfile,"</lbd:jobRecord>\n\n");

	/* </body> */
}
