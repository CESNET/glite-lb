#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

extern "C" {
#include <pthread.h>
#include "glite/wmsutils/tls/ssl_helpers/ssl_inits.h"
#include "glite/wmsutils/tls/ssl_helpers/ssl_pthreads.h"
#include "interlogd.h"
#include "glite/lb/consumer.h"
#include "glite/lb/lb_gss.h"
}

#if defined(IL_NOTIFICATIONS)
#define DEFAULT_PREFIX "/tmp/notif_events"
#define DEFAULT_SOCKET "/tmp/notif_interlogger.sock"
#else
#define DEFAULT_PREFIX "/tmp/dglogd.log"
#define DEFAULT_SOCKET "/tmp/interlogger.sock"
#endif
                                                                                
int TIMEOUT = DEFAULT_TIMEOUT;
                                                                                
gss_cred_id_t cred_handle = GSS_C_NO_CREDENTIAL;
pthread_mutex_t cred_handle_lock = PTHREAD_MUTEX_INITIALIZER;
                                                                                
char *file_prefix = DEFAULT_PREFIX;
int bs_only = 0;
 
char *cert_file = NULL;
char *key_file  = NULL;
char *CAcert_dir = NULL;
char *log_server = NULL;
char *socket_path = DEFAULT_SOCKET;
 

int 
main (int ac,const char *av[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;
	
	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
