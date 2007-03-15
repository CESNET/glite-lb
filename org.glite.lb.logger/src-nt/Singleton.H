#ifndef _SINGLETON_H
#define _SINGLETON_H

#include <pthread.h>

#include "Exception.H"

template <class T>
class Singleton {
public:
	// obtain the singleton object
	static T* instance() {
		// XXX - is this really thread safe?
		static pthread_mutex_t instance_lock = PTHREAD_MUTEX_INITIALIZER;

		pthread_mutex_lock(&instance_lock);
		if(theInstance == NULL) {
			theInstance = new T;
		}
		pthread_mutex_unlock(&instance_lock);
		return(theInstance);
	}

protected:
	// prevent other's from messing with the instance
	Singleton()  {}
	Singleton(const Singleton &) {}
	Singleton& operator=(const Singleton &) {}
	~Singleton() {}
	
private:
	static T *theInstance;
};

template<class T>
T *Singleton<T>::theInstance = NULL;


#endif
