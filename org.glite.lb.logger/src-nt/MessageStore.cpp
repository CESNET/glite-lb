#include <pthread.h>
#include <sys/time.h>
#include <sstream>

#include "MessageStore.H"

pthread_mutex_t MessageStore::ID::counterLock = PTHREAD_MUTEX_INITIALIZER;
unsigned MessageStore::ID::counter = 0;

MessageStore::ID::ID(){
	time_t t;
	time(&t);
	pthread_mutex_lock(&counterLock);
	counter++;
	id = ((unsigned long long) counter << 32) + t;
	pthread_mutex_unlock(&counterLock);
}

std::string MessageStore::ID::toString() const{
	std::ostringstream oss;
	oss << id;
	return oss.str();
}

