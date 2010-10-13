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

@@@LANG: C

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "glite/lbu/trio.h"
#include "producer.h"
#include "glite/lb/ulm_parse.h"

#define edg_wll_LogEvent edg_wll_LogEventTest
#define edg_wll_LogEventProxy edg_wll_LogEventTest

int edg_wll_LogEventTest(edg_wll_Context context, edg_wll_EventCode event, char *fmt, ...) {
	va_list fmt_args;
	int ret;

	va_start(fmt_args, fmt);
	ret = trio_vprintf(fmt,fmt_args);
	printf("\n");
	va_end(fmt_args);
	return ret;
}

@@@{
use strict;
require '../src/uiwrap.T';
@@@}

int main() {
	edg_wll_Context ctx;

@@@{

#use strict;

for my $e ($event->getTypesOrdered) {
	my $E = uc $e;
	my $func = "edg_wll_Log$e";		# name of generated function
	my $funcproxy = $func . "Proxy";
	my $p = "ctx";
	my $values = "";
	my $frees = "";

gen "// $e\n{\n";

	selectType $event $e;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->getName;
		my $fdecl = $f->getType;
		my $ft = $f->{type};
		my $value = undef;
		my $free = "free($fn);";

		if ($f->{codes}) {
			$fdecl = "char *";
			$value = "$fn = edg_wll_$e".ucfirst($fn)."ToString(EDG_WLL_${E}_$f->{codes}->[0]->{name});";
		} elsif (($ft eq "int") || ($ft eq "port")) { $value = "$fn = 654;"; $free = ""; }
		elsif (($ft eq "float") || ($ft eq "double")) { $value = "$fn = 3.14159265358979323846264;"; $free = ""; }
		elsif ($ft eq "string") { $value = "$fn = strdup(\"teststring\");"; }
		elsif ($ft eq "logsrc") { $value = "$fn = EDG_WLL_SOURCE_APPLICATION;"; $free = ""; }
		elsif ($ft eq "jobid") { $value = "glite_jobid_parse(\"https://some.host:1234/x67qr549qc\",&$fn);"; $free = "glite_jobid_free($fn);"; }
		elsif ($ft eq "notifid") { $value = "edg_wll_NotifIdParse(\"https://some.host:1234/NOTIF:x67qr549qc\",&$fn);"; $free = "edg_wll_NotifIdFree($fn);"; }
		elsif ($ft eq "timeval") { $value = "$fn.tv_sec = 274492800; $fn.tv_usec = 1; //tpb rox! :-)"; $free = ""; }
		else { 
gen "% FIXME: unknown type $ft for $fn \n"; 
		}
		$p .= ", $fn";
		$values .= "\n\t$value";
		if ($free) { $frees .= "\n\t$free"; }

gen "\t$fdecl $fn;\n";
	}
gen "$values\n\tprintf(\"%s: \", \"$e\");\n\tedg_wll_Log$e($p);\n\t$frees\n";
gen "}\n\n";
}

	
@@@}
	edg_wll_LogSandboxOUTPUT(ctx, "tj", "cj");
}
