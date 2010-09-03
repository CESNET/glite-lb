#ident "$Header$"
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


#include "interlogd.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

struct plugin_list {
	struct il_output_plugin plugin_def;
	struct plugin_list *next;
};

static struct plugin_list *plugins = NULL;

#define DL_RESOLVESYM(var, handle, name, type) \
	dlerror(); \
	var = (type) dlsym(handle, name); \
	if(var == NULL) { \
		snprintf(err, sizeof(err), "plugin_init: error resolving %s: %s", name, dlerror()); \
		set_error(IL_DL, ENOENT, err); \
		return -1; \
	}

int plugin_mgr_init(const char *plugin_name, char *cfg)
{
	char err[256];
	void *dl_handle;
	struct plugin_list *plugin;

	dlerror();
	dl_handle = dlopen(plugin_name, RTLD_LAZY);
	if(dl_handle == NULL) {
		snprintf(err, sizeof(err), "plugin_init: error opening dynamic library: %s", dlerror());
		set_error(IL_SYS, ENOENT, err);
		return -1;
	}
	dlerror();

	plugin = malloc(sizeof(*plugin));
	if(plugin == NULL) {
		set_error(IL_NOMEM, ENOMEM, "plugin_init: error allocating plugin description");
		return -1;
	}

	plugin->next = plugins;
	plugins = plugin;

	DL_RESOLVESYM(plugin->plugin_def.plugin_init, dl_handle, "plugin_init", int(*)(char *));
	DL_RESOLVESYM(plugin->plugin_def.plugin_supports_scheme, dl_handle,  "plugin_supports_scheme", int(*)(const char *));
	DL_RESOLVESYM(plugin->plugin_def.event_queue_connect, dl_handle, "event_queue_connect", int (*)(struct event_queue*));
	DL_RESOLVESYM(plugin->plugin_def.event_queue_send, dl_handle, "event_queue_send", int (*)(struct event_queue *));
	DL_RESOLVESYM(plugin->plugin_def.event_queue_close, dl_handle, "event_queue_close", int (*)(struct event_queue *));

	return (*plugin->plugin_def.plugin_init)(cfg);
}


struct il_output_plugin *
plugin_get(const char *scheme)
{
	struct plugin_list *outp;

	for(outp = plugins; outp != NULL; outp = outp->next) {
		if((outp->plugin_def.plugin_supports_scheme)(scheme)) {
			return &outp->plugin_def;
		}
	}

	return NULL;
}
