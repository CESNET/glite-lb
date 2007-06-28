#ifndef _EVENT_MANAGER_H
#define _EVENT_MANAGER_H

#include <list>


// interface

class Event {
};


template <class THandler, class TEvent>
class EventHandler {
public:
	typedef int (THandler::*EventHandlerType)(TEvent* &);

	

private:
	std::list<EventHandlerType> m_handlers;
};


class EventManager {
public:
	// constants for event handler return codes
	static const int NOT_HANDLED = 0;
	static const int HANDLED = 1;
	static const int HANDLED_FINAL = 2;
	static const int HANDLED_NEW = 3;

	int postEvent(Event* &event);
	
	template<class T>
	registerHandler(, int priority);

private:
	std::list<
};



// implementation

#endif
