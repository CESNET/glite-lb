#include <iostream>
#include <cstdlib>
#include <cstring>
//#include <string.h>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#ifdef BUILDING_LB_COMMON
#include "events_parse.h"
#else
#include "glite/lb/events_parse.h"
#endif

class EventParseTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(EventParseTest);
@@@{
        for my $e ($event->getTypesOrdered) {
                my $u = lcfirst $e;
		if ($u =~ m/^pBS/) { $u = ucfirst $u; }
		if ($u =~ m/^condor/) { $u = ucfirst $u; }
                my $c = getTypeComment $event $e;
                gen "\tCPPUNIT_TEST($u);\n";
        }
@@@}
	CPPUNIT_TEST_SUITE_END();

public:
@@@{
        for my $e ($event->getTypesOrdered) {
                my $u = lcfirst $e;
		if ($u =~ m/^pBS/) { $u = ucfirst $u; }
		if ($u =~ m/^condor/) { $u = ucfirst $u; }
                my $c = getTypeComment $event $e;
                gen "\tvoid $u();\n";
        }
@@@}

};

@@@{
        for my $e ($event->getTypesOrdered) {
                my $l = lcfirst $e;
		if ($l =~ m/^pBS/) { $l = ucfirst $l; }
		if ($l =~ m/^condor/) { $l = ucfirst $l; }
                my $u = uc $e;
                my $c = getTypeComment $event $e;
                gen qq{
void EventParseTest::$l()
\{
	edg_wll_Context ctx;
	edg_wll_Event   *e1,*e2;
	char    *line,*et,*ed;

	edg_wll_InitContext(&ctx);
	e1 = edg_wll_InitEvent(EDG_WLL_EVENT_$u);

	gettimeofday(&e1->any.timestamp,NULL);
	
	e1->any.host = strdup("some.host");
	e1->any.level = 7;
	e1->any.priority = 0;
	edg_wlc_JobIdParse("https://some.host:1234/x67qr549qc",&e1->any.jobId);
	e1->any.seqcode = strdup(EDG_WLL_SEQ_BIGHELPER_INITIAL);
	e1->any.user = strdup("/O=Grid/CN=This User");
	e1->any.source = EDG_WLL_SOURCE_USER_INTERFACE;
	e1->any.src_instance = strdup("");

};

		selectType $event "$e";
		for ($event->getFieldsOrdered) {
			my $f = selectField $event $_;
			my $fn = getName $f;
			my $value = '';

			if ($f->{codes}) {
				my $fnu = uc $fn;
				
				my $v = "EDG_WLL_${u}_$f->{codes}->[0]->{name}";
				$value = "e1->$l.$fn = $v;";
			}
			else {
#				$value = "e1->$l.$fn = value;";
#				$value = $f->fromString('value',"e1->$l.$fn");
				if (($f->{type} eq "int") || ($f->{type} eq "port")) { 
					$value = "e1->$l.$fn = 654;";
				} elsif (($f->{type} eq "float") || ($f->{type} eq "double")) { 
					$value = "e1->$l.$fn = 3.1415;";
				} elsif ($f->{type} eq "string") { 
					$value = "e1->$l.$fn = strdup(\"teststring\");";
				} elsif ($f->{type} eq "logsrc") { 
					$value = "e1->$l.$fn = EDG_WLL_SOURCE_APPLICATION;";
				} elsif ($f->{type} eq "jobid") { 
					$value = "edg_wlc_JobIdParse(\"https://some.host:1234/x67qr549qc\",&e1->$l.$fn);";
				} elsif ($f->{type} eq "notifid") { 
					$value = "edg_wll_NotifIdParse(\"https://some.host:1234/NOTIF:x67qr549qc\",&e1->$l.$fn);";
				} else { 
					gen "% FIXME: unknown type $f->{type} for $fn \n"; 
				}
			}
			gen "\t$value\n";
		}
		gen '
	line = edg_wll_UnparseEvent(ctx,e1);
//	std::cerr << line << std::endl;
	
	if (!line) {
		edg_wll_Error(ctx,&et,&ed);
		CPPUNIT_ASSERT_MESSAGE(std::string("edg_wll_UnparseEvent():") + et + " " + ed, line);
	}

	if (edg_wll_ParseEvent(ctx,line,&e2)) {
		edg_wll_Error(ctx,&et,&ed);
		CPPUNIT_ASSERT_MESSAGE(std::string("edg_wll_ParseEvent():") + et + " " + ed, 0);
	}

	if ((edg_wll_CompareEvents(ctx,e1,e2))) {
		edg_wll_Error(ctx,&et,&ed);
		CPPUNIT_ASSERT_MESSAGE(std::string("edg_wll_CompareEvents():") + et + " " + ed, 0);
	}

	edg_wll_FreeEvent(e1); free(e1);
	edg_wll_FreeEvent(e2); free(e2);
	edg_wll_FreeContext(ctx);
	free(line);
}
';

        }
@@@}

CPPUNIT_TEST_SUITE_REGISTRATION( EventParseTest );

#include "test_main.cpp"
