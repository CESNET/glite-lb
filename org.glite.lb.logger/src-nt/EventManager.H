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
