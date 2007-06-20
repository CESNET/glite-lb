#ifndef _EVENT_MANAGER_H
#define _EVENT_MANAGER_H

#include <list>

#include "Singleton.H"

// interface

// type for return code of event handler
typedef enum {
	NOT_HANDLED,  // the event was not handled at all
	HANDLED,      // the event was handled succesfully
	HANDLED_FINAL // the event was handled, 
	// no other handlers should be called
} eventstatus_t;


class Event {
};


template<class T>
class EventHandler {
public:
	virtual eventstatus_t handleEvent(T *&e);
};


template <class T>
class EventManagerForType: public Singleton< EventManagerForType<T> > {
public:

protected:
	void registerHandler(EventHandler<T> *);
	void registerHandlerFirst(EventHandler<T> *);

private:

};


class EventManager: public Singleton<EventManager> {
public:
	eventstatus_t postEvent(Event *);

	template<class T>
	void registerHandler(EventHandler<T> *h) {
		EventManagerForType<T>::instance()->registerHandler(h);
	}

	template<class T>
	void registerHandlerFirst(EventHandler<T> *) {
		EventManagerForType<T>::instance()->registerHandlerFirst(h);
	}

protected:
	EventManager() 
		{}

	virtual ~EventManager()
		{}

private:
	
};




// implementation

#endif
