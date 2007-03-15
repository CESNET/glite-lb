#ifndef _EVENT_MANAGER_H
#define _EVENT_MANAGER_H


class EventManager {
public:
	// type for return code of event handler
	typedef enum {
		NOT_HANDLED,  // the event was not handled at all
		HANDLED,      // the event was handled succesfully
		HANDLED_FINAL // the event was handled, 
		// no other handlers should be called
	} eventstatus_t;

	
	static EventManager* getEventManager() { return &theEventManager; };

	class Event {
	public:
	};

	template<class T>
	class EventHandler {
	public:

		virtual eventstatus_t handleEvent(T *&e);

		eventstatus_t dispatchEvent(Event *&e) {
			T *event = dynamic_cast<T*>(e);
			
			if(event) 
				return(handleEvent(event));
			else
				return(NOT_HANDLED);
		}
	};


	void postEvent(Event &);

	template<class T>
	bool registerHandler(EventHandler<T> *);

	template<class T>
	bool registerHandlerFirst(EventHandler<T> *);

private:
	
	// the event manager
	static EventManager theEventManager;

	// private default constructor for singleton instance
	EventManager() 
		{};

};


#endif
