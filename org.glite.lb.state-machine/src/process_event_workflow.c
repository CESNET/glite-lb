#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <syslog.h>

#include "glite/lb/context-int.h"

#include "glite/lbu/trio.h"

#include "intjobstat.h"
#include "seqcode_aux.h"


int processEvent_Workflow(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	return	RET_OK;
}
