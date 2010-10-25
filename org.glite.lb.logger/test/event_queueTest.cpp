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
  struct event_queue_msg *next;
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
		eq = event_queue_create(server, NULL);
		threads[0].thread_id = pthread_self();
		threads[0].gss.context = NULL;
		threads[0].jobid = strdup("1");
		threads[1].thread_id = (pthread_t)1;
		threads[1].gss.context = NULL;
		threads[1].jobid = strdup("2");
		eq->thread = threads;
		eq->num_threads = 2;
		free(server);
	}

	void tearDown() {
		struct event_queue_msg *mp;
		struct server_msg *m;

		for(mp = eq->head; mp != eq->head; ) {
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
		CPPUNIT_ASSERT( eq->head == NULL );
		CPPUNIT_ASSERT( eq->tail_ems == NULL );
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
		CPPUNIT_ASSERT_EQUAL( mp, mp->next->prev);
		mp = mp->prev;
		m = mp->msg;
		CPPUNIT_ASSERT_EQUAL( string(m->job_id_s), string("1") );
		CPPUNIT_ASSERT_EQUAL( mp, eq->tail_ems );
		CPPUNIT_ASSERT_EQUAL( mp, mp->next->prev);
		mp = mp->prev;
		m = mp->msg;
		CPPUNIT_ASSERT_EQUAL( string(m->job_id_s), string("3") );
		CPPUNIT_ASSERT_EQUAL( mp, mp->next->prev);
		CPPUNIT_ASSERT_EQUAL( mp->prev, eq->head );
	}

	void testEventQueueGet() {
		struct queue_thread *me = threads + 0;
		struct server_msg *m;
		int ret;

		doSomeInserts();
		ret = event_queue_get(eq, me, &m);
		CPPUNIT_ASSERT( ret == 1 );
		CPPUNIT_ASSERT_EQUAL( string("1"), string(m->job_id_s) );
		CPPUNIT_ASSERT( me->current == eq->head->prev );
		CPPUNIT_ASSERT_EQUAL( string("1"), string(me->jobid) );
		CPPUNIT_ASSERT( m == me->current->msg );
	}

	void testEventQueueRemove() {
		struct server_msg *m;
		struct queue_thread *me = threads + 0;
		int ret;

		doSomeInserts();
		threads[1].jobid = strdup("4");
		ret = event_queue_get(eq, me, &m);
		CPPUNIT_ASSERT(ret == 1);
		ret = event_queue_remove(eq, me);
		/* remain 2 */
		CPPUNIT_ASSERT( eq->head->prev ==  eq->head->next );
		CPPUNIT_ASSERT( eq->head != eq->head->prev );
		CPPUNIT_ASSERT( eq->head == eq->tail_ems );
		ret = event_queue_get(eq, me, &m);
		CPPUNIT_ASSERT( ret == 1);
		ret = event_queue_remove(eq, me);
		/* remain 1 */
		CPPUNIT_ASSERT( ret == 0 );
		CPPUNIT_ASSERT( eq->head != NULL );
		CPPUNIT_ASSERT( eq->head == eq->head->prev );
		CPPUNIT_ASSERT( eq->head == eq->head->next );
		ret = event_queue_get(eq, me, &m);
		CPPUNIT_ASSERT( ret == 1 );
		ret = event_queue_remove(eq, me);
		/* empty queue */
		CPPUNIT_ASSERT( ret == 0);
		CPPUNIT_ASSERT( eq->head == NULL );
	}

protected:
	char *server;
	struct event_queue *eq;
	struct queue_thread threads[2];

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
