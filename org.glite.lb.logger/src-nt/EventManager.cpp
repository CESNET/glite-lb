#include "EventManager.H"

int
EventManager::postEvent(Event* &e)
{
  for(std::list<EventHandler*>::iterator i = handlers.begin();
      i != handlers.end();
      i++) {
    (*i)->handleEvent(e);
  }
  return 0;
}

void
EventManager::addHandler(EventHandler *handler)
{
  handlers.push_back(handler);
}

void
EventManager::removeHandler(EventHandler *handler)
{
}
