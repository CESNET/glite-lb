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
