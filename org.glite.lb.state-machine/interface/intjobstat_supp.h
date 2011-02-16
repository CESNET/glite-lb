#include "glite/lb/intjobstat.h"

#ifndef GLITE_LB_INTJOBSTAT_SUPP_H
#define GLITE_LB_INTJOBSTAT_SUPP_H

char *enc_intJobStat(char *old, intJobStat* stat);
intJobStat* dec_intJobStat(char *in, char **rest);
edg_wll_JobStat* intJobStat_to_JobStat(intJobStat* intStat);
char* intJobStat_getLastSeqcode(intJobStat* intStat);
void intJobStat_nullLastSeqcode(intJobStat* intStat);

#endif
