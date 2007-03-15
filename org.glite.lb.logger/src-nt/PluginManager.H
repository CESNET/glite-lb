#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H

#include <list>
#include <iostream>

#include "Singleton.H"

class PluginManager: public Singleton<PluginManager> {
	friend class Singleton<PluginManager>;
public:
	
	// base class for plugins
	class Plugin {
	public:
		const char *name;

		Plugin(const char *aname) : name(aname) { 
			PluginManager::instance()->registerPlugin(this, aname); 
		}

		virtual bool initialize() = 0;
		virtual bool cleanup () = 0;
	};

	// add plugin with given name to the list of registered plugins
	void registerPlugin(Plugin *plugin, const char *name) { 
		pluginList.push_front(plugin);
	}

	// remove plugin from the list
	void removePlugin();

	// initialize all plugins on list
	void initialize() {
		for(std::list<Plugin *>::iterator i = pluginList.begin();
		    i != pluginList.end();
		    i++) {
			(*i)->initialize();
		}
		
	}

	// cleanup all plugins on list
	void cleanup() {
		for(std::list<Plugin *>::iterator i = pluginList.begin();
		    i != pluginList.end();
		    i++) {
			(*i)->cleanup();
		}
		
	}
		

protected:
	//  default constructor
	PluginManager() : pluginList()
		{};

private:
	// list of registered plugins
	std::list<Plugin *> pluginList;

};


#endif
