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

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

extern "C" {
#include <pthread.h>
#if 0
#include "glite/wmsutils/tls/ssl_helpers/ssl_inits.h"
#include "glite/wmsutils/tls/ssl_helpers/ssl_pthreads.h"
#endif
#include "glite/security/glite_gss.h"
#include "interlogd.h"
#include "glite/lb/consumer.h"
}

#if defined(IL_NOTIFICATIONS)
#define DEFAULT_PREFIX "/tmp/notif_events"
#define DEFAULT_SOCKET "/tmp/notif_interlogger.sock"
#else
#define DEFAULT_PREFIX "/tmp/dglogd.log"
#define DEFAULT_SOCKET "/tmp/interlogger.sock"
#endif
                                                                                
int TIMEOUT = DEFAULT_TIMEOUT;
                                                                                
#if 0
gss_cred_id_t cred_handle = GSS_C_NO_CREDENTIAL;
pthread_mutex_t cred_handle_lock = PTHREAD_MUTEX_INITIALIZER;
#endif
cred_handle_t *cred_handle = NULL;
pthread_mutex_t cred_handle_lock = PTHREAD_MUTEX_INITIALIZER;
                                                                                
char *file_prefix = DEFAULT_PREFIX;
int bs_only = 0;
int lazy_close = 1;
int default_close_timeout;
size_t max_store_size;
size_t queue_size_low = 0;
size_t queue_size_high = 0;
int parallel = 0;
int killflg = 0;
 
char *cert_file = NULL;
char *key_file  = NULL;
char *CAcert_dir = NULL;
char *log_server = NULL;
char *socket_path = DEFAULT_SOCKET;
 
extern "C" {
	void do_handle_signal() { };
}

int 
main (int ac,const char *av[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;
	
	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
