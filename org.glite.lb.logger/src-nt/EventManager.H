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

#ifndef _EVENT_MANAGER_H
#define _EVENT_MANAGER_H

#include <list>


// interface

/**
 * Base class of event hierarchy.
 */
class Event {
public:
    virtual ~Event() {}
};


/**
 * Base class for event handler address.
 */
class EventHandler {
public:
    // constants for event handler return codes
    static const int NOT_HANDLED = 0;
    static const int HANDLED = 1;
    static const int HANDLED_FINAL = 2;
    static const int HANDLED_NEW = 3;

    virtual int handleEvent(Event* &e) { return NOT_HANDLED; }
    virtual ~EventHandler() {}
};


/**
 * Holds addres of event handler, ie. pointer to member function of T 
 * that takes E* & as an argument.
 */
template<class T, class E>
class TypedEventHandler: public EventHandler {
public:
    typedef int (T::*handlerType)(E* &);
    
    TypedEventHandler(T *handler, handlerType method) 
	: m_handler(handler), m_handleEvent(method) {
    }

    virtual int handleEvent(Event* &e) {
	E *ne = dynamic_cast<E*>(e);
	int result = EventHandler::NOT_HANDLED;
	if(ne) {
	    result = (m_handler->*m_handleEvent)(ne);
	    if((result == EventHandler::HANDLED_NEW) && 
	       !(ne == e)) {
		delete e;
		e = ne;
	    }
	}
	return result;
    }
    
private:
    T *m_handler;
    handlerType m_handleEvent;
};


class EventManager {
public:
    
    int postEvent(Event* &event);
    
    template<class T, class E>
    EventHandler& registerHandler(T *handler, int (T::*method)(E* &)) {
	EventHandler *h = new TypedEventHandler<T,E>(handler, method);
	addHandler(h);
	return *h;
    }
    
    template<class T>
    EventHandler& registerHandler(T *handler) {
	return registerHandler(handler, &T::handleEvent);
    }

private:
    std::list<EventHandler*> handlers;

    void addHandler(EventHandler*);
    void removeHandler(EventHandler *);
};


#endif
