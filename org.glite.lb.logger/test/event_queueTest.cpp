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

#include <cppunit/extensions/HelperMacros.h>
 
#include "IlTestBase.h"

extern "C" {
struct event_queue_msg {
  struct server_msg *msg;
  struct event_queue_msg *prev;
};
}

#include <string>
using namespace std;

class event_queueTest: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( event_queueTest );
	CPPUNIT_TEST( testEventQueueCreate );
	CPPUNIT_TEST( testEventQueueInsert );
	CPPUNIT_TEST( testEventQueueGet );
	CPPUNIT_TEST( testEventQueueRemove );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
		server = strdup("localhost:8080");
		eq = event_queue_create(server);
		free(server);
	}

	void tearDown() {
		struct event_queue_msg *mp;
		struct server_msg *m;

		for(mp = eq->head; mp != NULL; ) {
			struct event_queue_msg *mq;

			server_msg_free(mp->msg);
			mq = mp;
			mp = mp->prev;
			free(mq);
		}
		eq->head = NULL;
		event_queue_free(eq);
	}

	void testEventQueueCreate() {
		CPPUNIT_ASSERT( eq != NULL );
		CPPUNIT_ASSERT_EQUAL( string(eq->dest_name), string("localhost") );
		CPPUNIT_ASSERT_EQUAL( eq->dest_port, 8081 );
		CPPUNIT_ASSERT( eq->tail == NULL );
		CPPUNIT_ASSERT( eq->head == NULL );
		CPPUNIT_ASSERT( eq->tail_ems == NULL );
		CPPUNIT_ASSERT( eq->mark_this == NULL );
		CPPUNIT_ASSERT( eq->mark_prev == NULL );
		CPPUNIT_ASSERT( eq->thread_id == 0 );
		CPPUNIT_ASSERT( eq->flushing == 0 );
		CPPUNIT_ASSERT( eq->flush_result == 0 );
	}

	void testEventQueueInsert() {
		struct event_queue_msg *mp;
		struct server_msg *m;

		doSomeInserts();
		mp = eq->head;
		m = mp->msg;
		CPPUNIT_ASSERT_EQUAL( string(m->job_id_s), string("2") );
		CPPUNIT_ASSERT_EQUAL( mp, eq->tail_ems );
		mp = mp->prev;
		m = mp->msg;
		CPPUNIT_ASSERT_EQUAL( string(m->job_id_s), string("1") );
		mp = mp->prev;
		m = mp->msg;
		CPPUNIT_ASSERT_EQUAL( string(m->job_id_s), string("3") );
		CPPUNIT_ASSERT_EQUAL( mp, eq->tail );
		CPPUNIT_ASSERT( mp->prev == NULL );
	}

	void testEventQueueGet() {
		struct event_queue_msg *mp;
		struct server_msg *m,sm;
		int ret;

		doSomeInserts();
		mp = eq->head;
		eq->head = mp->prev;
		eq->tail_ems = NULL;
		server_msg_free(mp->msg);
		free(mp);
		ret = event_queue_get(eq, &m);
		CPPUNIT_ASSERT( ret == 0 );
		CPPUNIT_ASSERT( eq->mark_this == eq->head );
		CPPUNIT_ASSERT( eq->mark_prev == NULL );
		CPPUNIT_ASSERT_EQUAL( string("1"), string(m->job_id_s) );
		sm = IlTestBase::smsg;
		sm.job_id_s = "4";
		sm.receipt_to = 1;
		ret = event_queue_insert(eq, &sm);
		CPPUNIT_ASSERT( ret == 0 );
		CPPUNIT_ASSERT( eq->mark_prev == eq->head );
		CPPUNIT_ASSERT( eq->mark_this == eq->head->prev );
		ret = event_queue_insert(eq, &sm);
		CPPUNIT_ASSERT( ret == 0 );
		CPPUNIT_ASSERT( eq->mark_prev == eq->head->prev );
		CPPUNIT_ASSERT( eq->mark_this == eq->head->prev->prev );
	}

	void testEventQueueRemove() {
		struct event_queue_msg *mp;
		struct server_msg *m,sm;
		int ret;

		doSomeInserts();
		ret = event_queue_get(eq, &m);
		mp = eq->mark_this->prev;
		sm = IlTestBase::smsg;
		sm.job_id_s = "4";
		sm.receipt_to = 1;
		event_queue_insert(eq, &sm);
		ret = event_queue_remove(eq);
		CPPUNIT_ASSERT( eq->head->prev == mp );
		CPPUNIT_ASSERT( eq->mark_this == NULL );
		CPPUNIT_ASSERT( eq->mark_prev == NULL );
	}

protected:
	char *server;
	struct event_queue *eq;

	void doSomeInserts() {
		struct server_msg m = IlTestBase::smsg;

		m.job_id_s = "1";
		event_queue_insert(eq, &m);
		m.receipt_to = 1;
		m.job_id_s = "2";
		event_queue_insert(eq, &m);
		m.job_id_s = "3";
		m.receipt_to = 0;
		event_queue_insert(eq, &m);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( event_queueTest );
