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
