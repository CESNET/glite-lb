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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <expat.h>

#include <glite/jobid/cjobid.h>
#include <glite/lb/jobstat.h>

int use_proxy = 0;

void
print_jobs(edg_wll_JobStat *states) 
{
  int		i,j;
  
  for (i=0; states[i].state != EDG_WLL_JOB_UNDEF; i++) {	
    char *id = edg_wlc_JobIdUnparse(states[i].jobId);
    char *st = edg_wll_StatToString(states[i].state);
    
    if (!states[i].parent_job) {
      if (states[i].jobtype == EDG_WLL_STAT_SIMPLE) { 
	printf("      %s .... %s %s\n", id, st, (states[i].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[i].done_code) : "" );
      }
      else if ((states[i].jobtype == EDG_WLL_STAT_DAG) || 
	       (states[i].jobtype == EDG_WLL_STAT_COLLECTION)) {
	printf("%s  %s .... %s %s\n", (states[i].jobtype==EDG_WLL_STAT_DAG)?"DAG ":"COLL",id, st, (states[i].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[i].done_code) : "");
	for (j=0; states[j].state != EDG_WLL_JOB_UNDEF; j++) {
	  if (states[j].parent_job) {
	    char *par_id = edg_wlc_JobIdUnparse(states[j].parent_job);
	    
	    if (!strcmp(id,par_id)) {
	      char *sub_id = edg_wlc_JobIdUnparse(states[j].jobId),
		*sub_st = edg_wll_StatToString(states[j].state);
	      
	      printf(" `-       %s .... %s %s\n", sub_id, sub_st, (states[j].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[j].done_code) : "");
	      free(sub_id);
	      free(sub_st);
	    }
	    free(par_id);
	  }	
	}
      }
    }
    
    free(id);
    free(st);
  }
  
  printf("\nFound %d jobs\n",i);
}



