#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>


#include <glite/lb/producer.h>
#include "events_parse.h"

class EventParseTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(EventParseTest);
	CPPUNIT_TEST(regJob);
/*
@@@{
        for my $e ($event->getTypesOrdered) {
                my $u = lcfirst $e;
		if ($u =~ m/^pBS/) { $u = ucfirst $u; }
                my $c = getTypeComment $event $e;
                gen "\tCPPUNIT_TEST($u);\n";
        }
@@@}
*/
	CPPUNIT_TEST_SUITE_END();

public:
	void regJob();
/*
@@@{
        for my $e ($event->getTypesOrdered) {
                my $u = lcfirst $e;
		if ($u =~ m/^pBS/) { $u = ucfirst $u; }
                my $c = getTypeComment $event $e;
                gen "\tvoid $u();\n";
        }
@@@}
*/

};

void EventParseTest::regJob()
{
	edg_wll_Context	ctx;
	edg_wll_Event	*e1,*e2;
	char	*line,*et,*ed;

	edg_wll_InitContext(&ctx);
	e1 = edg_wll_InitEvent(EDG_WLL_EVENT_REGJOB);

	e1->regJob.jdl = strdup("very long job = \"blabla\" \\\\ \n hugh\t;");
	e1->regJob.ns = strdup("ns address");
	e1->regJob.jobtype = EDG_WLL_REGJOB_SIMPLE;
	e1->regJob.seed = strdup("");

	gettimeofday(&e1->any.timestamp,NULL);
	
	e1->any.host = strdup("some.host");
	e1->any.level = 7;
	e1->any.priority = 0;
	edg_wlc_JobIdParse("https://some.host:1234/x67qr549qc",&e1->any.jobId);
	e1->any.seqcode = EDG_WLL_SEQ_BIGHELPER_INITIAL;
	e1->any.user = strdup("/O=Grid/CN=This User");
	e1->any.source = EDG_WLL_SOURCE_USER_INTERFACE;
	e1->any.src_instance = strdup("");

	line = edg_wll_UnparseEvent(ctx,e1);
	std::cerr << line << std::endl;
	
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
}

/*
@@@{
        for my $e ($event->getTypesOrdered) {
                my $l = lcfirst $e;
		if ($l =~ m/^pBS/) { $l = ucfirst $l; }
                my $u = uc $e;
                my $c = getTypeComment $event $e;
                gen "
void EventParseTest::$l(){
	edg_wll_Context ctx;
	edg_wll_Event   *e1,*e2;
	char    *line,*et,*ed;

	edg_wll_InitContext(&ctx);
	e1 = edg_wll_InitEvent(EDG_WLL_EVENT_$u);
";

		selectType $event '_common_';
		for ($event->getFieldsOrdered) {
			my $f = selectField $event $_;
			my $fn = getName $f;
			my $value = getDefaultTestValue $f;
			if (!$f->{codes}) {
				if (!$f->hasAlias('ULM')) {
					my $fu = uc $fn;
					gen "\te1->any.$fn = $value;\n";
#					gen "\tif (". $f->isNULL("e1->any.$fn") .") MISSING(EDG_WLL\_COMMON\_$fu)\n";
				} else {
					my $fa = $f->getName('ULM');
					my $fu = uc $fa;
#					gen "\tif (". $f->isNULL("e1->any.$fn") .") MISSING(ULM\_$fu)\n";
				}
			}
		}
		gen '
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
}

';

        }
@@@}
*/

CPPUNIT_TEST_SUITE_REGISTRATION( EventParseTest );

#include "test_main.cpp"
