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

