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


@@@LANG: C
/*
@@@AUTO
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h> // log
#include <ctype.h> // isspace

#include "glite/security/glite_gss.h"
#include "glite/jobid/cjobid.h"
#include "glite/lb/notifid.h"
#include "glite/lb/events.h"
#include "glite/lb/context.h"
#include "glite/lb/context-int.h"
#include "glite/lb/ulm_parse.h"

#include "producer.h"
#include "args.h"

    //"    %s -p -l 100000 -j https://localhost/First_JobIV?localhost:7771 -s UserInterface -e jobAbort DG.JOB.ABORT.REASON=\"oops\"\n\n",

// undefine to disable support for -l option
#define ENABLE_REASON_LENGTH

static int flesh_seq(int);
static const struct timeval null_timeval = {0,0};

int main(int argc, char *argv[])
{
	char	/* *fmt,*fname = NULL,*/ *fmt_arg=NULL;
        char	*server = NULL, *code = NULL;
	char	*ff = NULL, *jobid_s = NULL;
	char	*src_instance = NULL;
	int	err = 0/*,i,done = 0,fmtlen*/;
	int     pri = 0, noinc = 0;
	int		use_lbproxy = 0;
	char	*lbproxy_user = NULL;
	char	*lbproxy_store_sock = NULL;
        int 	deb = 0;
#ifdef ENABLE_REASON_LENGTH
	int     elength = 0;
#endif
	edg_wll_Context	ctx;
        edg_wll_Source source;
	edg_wll_EventCode event = EDG_WLL_EVENT_UNDEF;
	edg_wlc_JobId jobid = 0;
	int seq = EDG_WLL_SEQ_NORMAL;
        int (*logev)(edg_wll_Context context,
		     edg_wll_EventCode event,
		     char *fmt, ...);

	/* all possible variables */
@@@{
    my %vars = ();
    for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
	       $event->getTypes) {
	selectType $event $t;
	for ($event->getFieldsOrdered) {
	    my $f = selectField $event $_;
	    my $fn = $f->getName;
	    my $type = {
		"type" => $f->getType,
		"defval" => $f->getDefaultNullValue
	    };

	    $type = { type=>'char *',defval=>'NULL' } if $f->{codes};
            warn "$fn: $vars{$fn}->{type} to $type->{type}\n"
			 if $vars{$fn} && ($vars{$fn}->{type} ne $type->{type}
				|| $vars{$fn}->{defval} ne $type->{defval});
	    $vars{$fn} = $type;
	}
    }
    foreach $name ( sort keys %vars ) {
	gen qq{\t$vars{$name}->{"type"} $name = $vars{$name}->{"defval"};\n};
    }
@@@}

	/* all possible arguments */
	edg_wll_Args parray[] = {
	    { EDG_WLL_ARGS_EVENT, "e", "event", "select event type (see -e help)", &event },
	    { EDG_WLL_ARGS_BOOL, "p", "priority", "send as priority event", &pri },
	    { EDG_WLL_ARGS_BOOL, "x", "lbproxy", "send directly to L&B Proxy (not locallogger)", &use_lbproxy },
	    { EDG_WLL_ARGS_STRING, "S", "lbproxy-store-sock", "L&B Proxy store socket path", &lbproxy_store_sock },
	    { EDG_WLL_ARGS_STRING, "U", "lbproxy-user-dn", "User certificate DN L&B Proxy store socket path", &lbproxy_user },

	    { EDG_WLL_ARGS_STRING, "m", "machine", 0, &server },
	    { EDG_WLL_ARGS_SOURCE, "s", "source", "event source (see -s help)", &source },
	    { EDG_WLL_ARGS_STRING, "i", "source-instance", "event source instance", &src_instance },
	    { EDG_WLL_ARGS_STRING, "j", "jobid", "JobId", &jobid_s },
	    { EDG_WLL_ARGS_STRING, "c", "sequence", "event sequence code", &code },
	    { EDG_WLL_ARGS_BOOL, "d", "debug mode", "enable debug mode", &deb },
#ifdef ENABLE_REASON_LENGTH
	    { EDG_WLL_ARGS_INT, "l", "reason-length", "extend 'reason' string to length (debug only)", &elength, 0, 1000000000 },
	    { EDG_WLL_ARGS_BOOL, "I", "dont-increment", "don't increment initial seqence code", &noinc },
#endif
@@@{
    my %typetab = (
	"char *", "EDG_WLL_ARGS_STRING",
	"int", "EDG_WLL_ARGS_INT",
	"double", "EDG_WLL_ARGS_DOUBLE",
	"glite_jobid_t", "EDG_WLL_ARGS_JOBID",
	"edg_wll_NotifId", "EDG_WLL_ARGS_NOTIFID",
	"edg_wll_Source", "EDG_WLL_ARGS_SOURCE",
	"uint16_t", "EDG_WLL_ARGS_UINT16",
	"struct timeval", "EDG_WLL_ARGS_TIMEVAL",
	"edg_wll_TagValue *", "EDG_WLL_ARGS_TAGLIST",
		  );
    my %vars = ();
    for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
	       $event->getTypes) {
	selectType $event $t;
	for ($event->getFieldsOrdered) {
	    my $f = selectField $event $_;
            my $ft = $f->{codes} ? 'char *' : $f->getType;
	    my $fn = $f->getName;
	    my $fc = $f->getComment;
	    $namehelp = { "type" => $ft, "help" => $fc };
#	   gen qq{ $fn $ft $fc \n};
	   $vars{$fn} = $namehelp;
	}
    }
    foreach $name ( sort keys %vars ) {
#	gen qq{ $vars{$name} $name \n };
	gen qq{\t    \{ $typetab{$vars{$name}->{"type"}}, 0, "$name", "$vars{$name}->{"help"}", &$name \},\n};
    }

@@@}
	    { EDG_WLL_ARGS_HELP },
	    { EDG_WLL_ARGS_NONE },
	};
	edg_wll_ParseArgs(&argc, argv, parray,
			  "Usage: %s [-d] [-p] [[-x] [-S sock] [-U user_dn]] [-l eventlen] [-j dg_jobid]"
			  "[-s source_id] -e event [key=value ...]");


	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context\n");
		exit(1);
	}

	/* log the event - priority/normal/proxy */
	logev = (pri) ? edg_wll_LogEventSync : edg_wll_LogEvent;
	logev = (use_lbproxy) ? edg_wll_LogEventProxy : logev;

        /* if no job given - generate some */
	if (jobid_s == 0) {
	    	const char* s;
		if (!server) {
		    s = "localhost";
		    err = 1; // result fail if used normaly
		} else
		    s = server;
		edg_wlc_JobIdCreate(s, 0, &jobid);
		jobid_s = edg_wlc_JobIdUnparse(jobid);
		fprintf(stderr, "JobId not given: created %s (server: %s)\n",jobid_s, s);
	}
	else if ((errno = edg_wlc_JobIdParse(jobid_s,&jobid))) {
		perror(jobid_s);
		exit(1);
	}

	if (event == EDG_WLL_EVENT_UNDEF) {
		fprintf(stderr,"%s: unknown or unspecified event\n",argv[0]);
		exit(1);
	}
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_SOURCE, source);
	if (src_instance) edg_wll_SetParam(ctx, EDG_WLL_PARAM_INSTANCE, src_instance);
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_LEVEL,
			 (deb) ? EDG_WLL_LEVEL_DEBUG : EDG_WLL_LEVEL_SYSTEM);

	seq = flesh_seq(event);

	if ( use_lbproxy ) {
		if ( !lbproxy_user ) {
			edg_wll_GssCred		gss_cred = NULL;
			edg_wll_GssStatus   gss_stat;
			if ( edg_wll_gss_acquire_cred_gsi(NULL, NULL, &gss_cred, &gss_stat) )
				lbproxy_user = "anonymous";
                        else
				lbproxy_user = strdup(gss_cred->name);
			if ( gss_cred != NULL ) edg_wll_gss_release_cred(&gss_cred, NULL);
		}
		if ( lbproxy_store_sock )
			edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_STORE_SOCK, lbproxy_store_sock);
		if (edg_wll_SetLoggingJobProxy(ctx,jobid,code,lbproxy_user,seq)) {
			char 	*et,*ed;
			edg_wll_Error(ctx,&et,&ed);
			fprintf(stderr,"SetLoggingJobProxy(%s,%s,%s): %s (%s)\n",jobid_s,code,lbproxy_user,et,ed);
			exit(1);
		}
	} else {
		if (edg_wll_SetLoggingJob(ctx,jobid,code,seq)) {
			char 	*et,*ed;
			edg_wll_Error(ctx,&et,&ed);
			fprintf(stderr,"SetLoggingJob(%s,%s): %s (%s)\n",jobid_s,code,et,ed);
			exit(1);
		}
	}

	if (noinc) edg_wll_SetSequenceCode(ctx,code,seq);

#ifdef ENABLE_REASON_LENGTH
	if (elength > 200000000) {
		fprintf(stderr,"%s: usupported reason message length: %d\n", argv[0], elength);
	} else if (elength > 0) {
		int d;
		int i = 0;
		reason = realloc(reason, elength + 1);
                reason[elength] = 0;
		while (i < (elength)) {
                        if ((i % 20) == 0 && (i + 14) < elength)
				i += sprintf(reason + i, "%d", i);
			reason[i++] = '.';
		}
		// amount of decimal digits + "end="
                d = (int)ceil(log(i)/log(10)) + 5;
		if (i > d)
			sprintf(reason + i - (int)(d), "end=%d",i);
		reason[i] = 0;
	}
#endif
#if 0
	if (fname) {
		FILE *f = fopen(fname,"r");
		long s;
                size_t r;

		fmt = "%s FILE.CONTENT=\"%|Us\" ";

		if (!f) { perror(fname); return 1; }
		fseek(f,0L,SEEK_END);
		s = ftell(f); rewind(f);
		ff = (char *) malloc(s+1); if (!ff) { perror(NULL); return 0; }
		r = fread(ff,1,s,f);
		printf("%s: read %d\n",fname,r);
		ff[s] = 0;
		fclose(f);
	}
	else
		fmt = "%s ";
#endif

	//err = edg_wll_LogEvent(ctx, event, fmt, fmt_arg, ff);

	switch (event)
	{
@@@{
$indent = "\t";
    for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
	       $event->getTypes) {

        my $tu = uc $t;
	gen $indent."case EDG_WLL_EVENT_${tu}: {\n";

	selectType $event $t;
	my $argl = "";
	my $values = "";
	my $frees = "";
	for ($event->getFieldsOrdered) {
	    my $f = selectField $event $_;
	    my $fn = $f->getName;
	    my $fdecl = $f->{codes} ? 'char *' : $f->getType;
	    my $ft = $f->{type};

	    if ($ft eq 'timeval') {
gen "\t    char s_${fn}[ULM_DATE_STRING_LENGTH + 1];\n";
		$values .= "\n\t    ".$f->toString($fn, "s_${fn}");
		$argl .= ", s_${fn}";
	    } elsif ($ft eq 'jobid' or $ft eq 'notifid' or $ULMasString{$ft}) {
gen "\t    char *s_${fn};\n";
		$values .= "\n\t    ".$f->toString($fn, "s_${fn}");
		$frees .= "\n\t    free(s_$fn);";
		$argl .= ", s_${fn}";
	    } else {
		$argl .= ", $fn";
	    }
	}
gen qq{$values
\t    //edg_wll_Log$t();
\t    err |= logev(ctx, EDG_WLL_EVENT_$tu, EDG_WLL_FORMAT_${tu}$argl);$frees
\t    \}
\t    break;
};
    }
@@@}
	default:
	    fprintf(stderr, "unknown event\n");
	}

	edg_wlc_JobIdFree(jobid);
	if (jobid_s) free(jobid_s); // add all strings

	if (err) {
	    char	*et,*ed;

	    edg_wll_Error(ctx,&et,&ed);
	    fprintf(stderr,"%s: edg_wll_LogEvent*(): %s (%s)\n",
		    argv[0],et,ed);
	    free(et); free(ed);
	}

	code = edg_wll_GetSequenceCode(ctx);
	puts(code);
	free(code);

	edg_wll_FreeContext(ctx);

	if (ff) free(ff);
	if (fmt_arg) free(fmt_arg);
#ifdef ENABLE_REASON_LENGTH
	if (reason) free(reason);
#endif

	return err;
}

static int flesh_seq(int event)
{
	switch (event) {
@@@{
	%seq = ( CREAM => 'CREAM', 'PBS' => 'PBS', 'gLite' => 'NORMAL', Condor=>'CONDOR', Transfer=>'NORMAL', 'VirtualMachine'=>'NORMAL' );
	for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
	       $event->getTypes)
	{
		selectType $event $t;

		my $type = uc $t;
		my $sf = $seq{$event->{flesh}->{$t}};

		print "\t\tcase EDG_WLL_EVENT_$type: return EDG_WLL_SEQ_$sf;\n";
	}
@@@}
	default: return -1;
	}
}
