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
#include <cstdlib>
#include <cstring>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>


#include "lb_xml_parse.h"
#include <glite/lb/xml_parse.h>
#include <glite/lb/events_parse.h>

class XMLParseTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(XMLParseTest);
	CPPUNIT_TEST(protoEventTest);
	CPPUNIT_TEST(protoStatusTest);
	CPPUNIT_TEST_SUITE_END();

public:
	void protoEventTest();
	void protoStatusTest();


};

static char * compare_events(const edg_wll_Event *e1, const edg_wll_Event *e2)
{
	char *tmp1, *tmp2;
	int tmp3;

	if (e1->any.type != e2->any.type) return "type";
@@@{
	selectType $event '_common_';
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = getName $f;
		my $ft = $f->{type};

		my $a = "e1->any.$fn";
		my $b = "e2->any.$fn";

		gen "\tif (!(".eval($main::compare{C}->{$ft}).")) return \"$fn\";\n";
	}
	
	gen "\tswitch(e1->any.type) {\n";
	for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes)
	{
		my $tu = uc $t;
		my $tl = lcfirst $t;
		if ($tl =~ m/^pBS/) { $tl = ucfirst $tl; }
		if ($tl =~ m/^cREAM/) { $tl = ucfirst $tl; }
		if ($tl =~ m/^condor/) { $tl = ucfirst $tl; }

		selectType $event $t;
		gen "\t\tcase EDG_WLL_EVENT\_$tu :\n";
		for ($event->getFieldsOrdered) {
			my $f = selectField $event $_;
			my $fn = $f->{name};
			my $ft = $f->{type};

			my $a = "e1->$tl.$fn";
			my $b = "e2->$tl.$fn";

			gen "\t\t\tif (!(".eval($main::compare{C}->{$ft}).")) return \"$fn\";\n";
		}
		gen "\t\tbreak;\n";
	}
@@@}
		default: return "default";
	} /* switch */
	return NULL;
}

static char * compare_states(const edg_wll_JobStat s1, const edg_wll_JobStat s2)
{
	char *tmp1, *tmp2;
	int tmp3;
@@@{
	selectType $status '_common_';
        for (getFieldsOrdered $status) {
                my $f = selectField $status $_;
		my $fn = getName $f;
		my $ft = $f->{type};

		my $a = "s1.$fn";
		my $b = "s2.$fn";

                if ($ft eq 'intlist') {
                        gen "\tif ( (s1.$_ != NULL) && (s2.$_ == NULL) || (s1.$_ == NULL) && (s2.$_ != NULL) )  return \"$fn\";\n";
                }
                elsif ($ft eq 'strlist') {
                }
                elsif ($ft eq 'taglist') {
                }
                elsif ($ft eq 'stslist') {
                }
		else {
			gen "\tif (!(".eval($main::compare{C}->{$ft}).")) return \"$fn\";\n";
		}
        }
@@@}
	return NULL;
}

void XMLParseTest::protoEventTest()
{
	edg_wll_Context	ctx;
	edg_wll_Event	*e1, *e2;
	char		*message, *et, *ed;

	edg_wll_InitContext(&ctx);
	e1 = (edg_wll_Event *) calloc(2,sizeof *e1);
	e1->any.type = EDG_WLL_EVENT_REGJOB;

	e1->regJob.jdl = strdup("very long job = \"blabla\" \\\\ \n hugh\t;");
	e1->regJob.ns = strdup("ns address");
	e1->regJob.jobtype = EDG_WLL_REGJOB_SIMPLE;
	e1->regJob.seed = strdup("1234");

	gettimeofday(&e1->any.timestamp,NULL);
	
	e1->any.host = strdup("some.host");
	e1->any.level = 7;
	e1->any.priority = 0;
	edg_wlc_JobIdParse("https://some.host:1234/x67qr549qc",&e1->any.jobId);
	e1->any.seqcode = EDG_WLL_SEQ_BIGHELPER_INITIAL;
	e1->any.user = strdup("/O=Grid/CN=This User");
	e1->any.source = EDG_WLL_SOURCE_USER_INTERFACE;
	e1->any.src_instance = strdup("UI");

	if (edg_wll_QueryEventsToXML(ctx, e1, &message)) {
		edg_wll_Error(ctx,&et,&ed);
		CPPUNIT_ASSERT_MESSAGE(std::string("QueryEventsToXML():") + et + " " + ed, 0);
	}
	
	CPPUNIT_ASSERT_MESSAGE("message: ", message);

	if (edg_wll_ParseQueryEvents(ctx, message, &e2)) {
		edg_wll_Error(ctx,&et,&ed);
		CPPUNIT_ASSERT_MESSAGE(std::string("ParseQueryEvents():") + et + " " + ed, 0);
	}

	if ((et = compare_events(e1,e2))) {
		CPPUNIT_ASSERT_MESSAGE(std::string("compare_events():") + et, 0);
	}
}

void XMLParseTest::protoStatusTest()
{	
	edg_wll_Context	ctx;
	edg_wll_JobStat	s1, s2;
	char		*message, *et, *ed;
	const struct timeval some_timeval = {14,12};


	edg_wll_InitContext(&ctx);
	edg_wll_InitStatus(&s1);

	s1.state = EDG_WLL_JOB_SUBMITTED;
	edg_wlc_JobIdParse("https://some.host:1234/x67qr549qc",&s1.jobId);
	s1.owner = strdup("/O=Grid/CN=This User");
	s1.jobtype = EDG_WLL_STAT_SIMPLE;
	s1.seed = strdup("4321");
	s1.condorId = strdup("condorId");
	s1.globusId = strdup("globusId");
	s1.localId = strdup("localId");
	s1.jdl = strdup("jdl");
	s1.matched_jdl = strdup("matched_jdl");
	s1.destination = strdup("destination");
	s1.condor_jdl = strdup("condor_jdl");
	s1.rsl = strdup("rsl");
	s1.reason = strdup("reason");
	s1.location = strdup("location");
	s1.ce_node = strdup("ce_node");
	s1.network_server = strdup("network_server");
	s1.subjob_failed = 1;
	s1.done_code = EDG_WLL_STAT_FAILED;
	s1.exit_code = 123;
	s1.resubmitted = 1;
	s1.cancelling = 1;
	s1.cancelReason = strdup("cancelReason");
	s1.cpuTime = 10;
	s1.stateEnterTime = some_timeval;
	s1.lastUpdateTime = some_timeval;
	s1.expectUpdate = 20;
	s1.expectFrom = strdup("expectFrom");
	s1.acl = strdup("acl");

	if (edg_wll_JobStatusToXML(ctx, s1, &message)) {
		edg_wll_Error(ctx,&et,&ed);
		CPPUNIT_ASSERT_MESSAGE(std::string("QueryEventsToXML():") + et + " " + ed, 0);
	}
	
	CPPUNIT_ASSERT_MESSAGE("message: ", message);

	if (edg_wll_ParseJobStat(ctx, message, (long) strlen(message), &s2)) {
		edg_wll_Error(ctx,&et,&ed);
		CPPUNIT_ASSERT_MESSAGE(std::string("ParseJobStat():") + et + " " + ed, 0);
	}

	if ((et = compare_states(s1,s2))) {
		CPPUNIT_ASSERT_MESSAGE(std::string("compare_states():") + et, 0);
	}



}

CPPUNIT_TEST_SUITE_REGISTRATION(XMLParseTest);

int main (int ac,const char *av[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;
	
	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
