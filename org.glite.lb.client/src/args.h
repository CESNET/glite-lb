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

#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_ARGS_H__

typedef enum {
    EDG_WLL_ARGS_NONE = 0,
    EDG_WLL_ARGS_BOOL,
    EDG_WLL_ARGS_INT,
    EDG_WLL_ARGS_UINT16,
    EDG_WLL_ARGS_FLOAT,
    EDG_WLL_ARGS_DOUBLE,
    EDG_WLL_ARGS_STRING,
    EDG_WLL_ARGS_HELP,
    EDG_WLL_ARGS_JOBID,
    EDG_WLL_ARGS_NOTIFID,
    EDG_WLL_ARGS_SOURCE,
    EDG_WLL_ARGS_EVENT,
    EDG_WLL_ARGS_OPTIONS,
    EDG_WLL_ARGS_SUBOPTIONS,
    EDG_WLL_ARGS_SELECTSTRING,
    EDG_WLL_ARGS_TIMEVAL,
    EDG_WLL_ARGS_TAGLIST,
} edg_wll_ArgsCode;

typedef struct {
    edg_wll_ArgsCode type;
    const char* oshort;
    const char* olong;
    const char* help;
    void* value;
    int min;
    int max;
} edg_wll_Args;

void edg_wll_ParseArgs(int* argc, char** argv, const edg_wll_Args* parray,
                       const char* help);

#endif /* __EDG_WORKLOAD_LOGGING_CLIENT_ARGS_H__ */
