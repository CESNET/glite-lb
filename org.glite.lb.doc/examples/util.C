#include "glite/lb/Event.h"
#include "glite/lb/JobStatus.h"

#include <iostream>

using namespace glite::lb;
using namespace std;

/*event*/
void
dumpEvent(Event *event)
{
// list of attribute names and types
	typedef std::vector<std::pair<Event::Attr, Event::AttrType>> AttrListType;
	
	cout << "Event name: " << event->name() << endl;
	AttrListType attr_list = event->getAttrs();
	for(AttrListType::iterator i = attr_list.begin();
	    i != attr_list.end();
	    i++) {
		Event::Attr attr = attr_list[i].first();
		
		cout << Event::getAttrName(attr) << " = ";
		switch(attr_list[i].second()) {
		case Event::INT_T: 
		case Event::PORT_T:
		case Event::LOGSRC_T:
			cout << event->getValInt(attr) << endl;
			break;
			
		case Event::STRING_T:
			cout << event->getValString(attr) << endl;
			break;
			
		case Event::TIMEVAL_T:
			cout << event->getValTime(attr).tv_sec << endl;
			break;
			
		case Event::FLOAT_T:
			cout << event->getValFloat(attr) << endl;
			break;
			
		case Event::DOUBLE_T:
			cout << event->getValDouble(attr) << endl;
			break;
			
		case Event::JOBID_T:
			cout << event->getValJobId(attr).toString() << endl;
			break;
			
		default:
			cout << "attribute type not supported" << endl;
			break;
		}
	}
}
/*end event*/


/*status*/
void dumpState(JobStatus *status) 
{
	
}
/*end status*/
