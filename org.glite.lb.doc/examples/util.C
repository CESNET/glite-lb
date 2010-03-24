/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners/ for details on the copyright holders.

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
	typedef vector<pair<Event::Attr, Event::AttrType>> AttrListType;
	
	cout << "Event name: " << event->name() << endl;
	AttrListType attr_list = event->getAttrs(); //* \label{l:getattrs}
	for(AttrListType::iterator i = attr_list.begin();
	    i != attr_list.end();
	    i++) {
		Event::Attr attr = attr_list[i].first;
		
		cout << Event::getAttrName(attr) << " = "; 
		switch(attr_list[i].second) {
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
	typedef vector<pair<JobStatus:Attr, JobStatus::AttrType>> AttrListType;

	cout << "Job status: " << status->name << endl;

	AttrListType attr_list = status->getAttrs(); //* \label{l:jgetattrs}
	for(AttrListType::iterator i = attr_list.begin();
	    i != attr_list.end();
	    i++) {
		JobStatus::Attr attr = attr_list[i].first;
		cout << JobStatus::getAttrName(attr) << " = ";
		switch(attr_list[i].second) {

		case INT_T:
			cout << status->getValInt(attr) << endl;
			break;
			
		case STRING_T: 
			cout << status->getValInt(attr) << endl;
			break;

		case TIMEVAL_T:
			cout << status->getValTime(attr).tv_sec << endl;
			break;

		case BOOL_T:
			cout << status->getValBool(attr).tv_sec << endl;
			break;

		case JOBID_T:
			cout << status->getValJobid(attr).toString() << endl;
			break;
			
		case INTLIST_T:
			vector<int> list = status->getValIntList(attr);
			for(vector<int>::iterator i = list.begin();
			    i != list.end();
			    i++) {
				cout << list[i] << " ";
			}
			cout << endl;
			break;
			
		case STRLIST_T:
			vector<string> list = status->getValStringList(attr);
			for(vector<string>::iterator i = list.begin();
			    i != list.end();
			    i++) {
				cout << list[i] << " ";
			}
			cout << endl;
			break;

		case TAGLIST_T: /**< List of user tags. */
			vector<pair<string, string>> list = status->getValTagList(attr);
			for(vector<pair<string,string>>::iterator i = list.begin();
			    i != list.end();
			    i++) {
				cout << list[i].first << "=" << list[i].second << " ";
			}
			cout << endl;
			break;

		case STSLIST_T:  /**< List of states. */
			vector<JobStatus> list = status->getValJobStatusList(attr);
			for(vector<JobStatus>::iterator i = list.begin();
			    i != list.end();
			    i++) {
				// recursion
				dumpState(&list[i]);
			}
			cout << endl;
			break;
			
		default:
			cout << "attribute type not supported" << endl;
			break;

		}
	}
}
/*end status*/
