#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>


#include <glite/lb/producer.h>
#include "events_parse.h"

class EventParseTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(EventParseTest);
	CPPUNIT_TEST(regJob);
	CPPUNIT_TEST_SUITE_END();

public:
	void regJob();


};

static char * compare_events(const edg_wll_Event *e1, const edg_wll_Event *e2)
{
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

void EventParseTest::regJob()
{
	edg_wll_Context	ctx;
	edg_wll_Event	*e1,*e2;
	char	*line,*et,*ed;

	edg_wll_InitContext(&ctx);
	e1 = edg_wll_InitEvent(EDG_WLL_EVENT_REGJOB);
	e1->any.type = EDG_WLL_EVENT_REGJOB;

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
		CPPUNIT_ASSERT_MESSAGE(std::string("UnparseEvent():") + et + " " + ed, line);
	}

	if (edg_wll_ParseEvent(ctx,line,&e2)) {
		edg_wll_Error(ctx,&et,&ed);
		CPPUNIT_ASSERT_MESSAGE(std::string("ParseEvent():") + et + " " + ed, 0);
	}

	if ((et = compare_events(e1,e2))) {
		CPPUNIT_ASSERT_MESSAGE(std::string("compare_events():") + et, 0);
	}
}

CPPUNIT_TEST_SUITE_REGISTRATION( EventParseTest );

int main (int ac,const char *av[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;
	
	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
