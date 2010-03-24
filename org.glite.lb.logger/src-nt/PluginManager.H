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

		virtual ~Plugin();
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
