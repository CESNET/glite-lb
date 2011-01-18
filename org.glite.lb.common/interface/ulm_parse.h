#ifndef GLITE_LB_ULM_PARSE_H
#define GLITE_LB_ULM_PARSE_H

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


#include <sys/time.h>  	/* for ULCconvertDate */
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*========= DATA =====================================================*/

#define ULM_DATE_STRING_LENGTH	21
#define ULM_FIELDS_MAX		100 /* max number of fields */
#define ULM_PARSE_OK		0
#define ULM_PARSE_ERROR		-1

#define ULM_EQ	'='
#define ULM_QM	'"'
#define ULM_BS	'\\'
#define ULM_SP	' '
#define ULM_TB	'\t'
#define ULM_LF	'\n'

typedef char *LogLine;

typedef struct _edg_wll_ULMFields {
  LogLine raw;
  int    *names;
  int    *vals;
  int     num;
} edg_wll_ULMFields , *p_edg_wll_ULMFields;

/*========= FUNCTIONS ================================================*/

#define EDG_WLL_ULM_CLEAR_FIELDS(x) { if (x) { if (x->vals) free(x->vals); if (x->names) free(x->names); x->num=0; } }

extern p_edg_wll_ULMFields edg_wll_ULMNewParseTable( LogLine );
extern void 	edg_wll_ULMFreeParseTable(p_edg_wll_ULMFields );

extern int 	edg_wll_ULMProcessParseTable( p_edg_wll_ULMFields );
extern char *	edg_wll_ULMGetNameAt( p_edg_wll_ULMFields, int );
extern char *	edg_wll_ULMGetValueAt ( p_edg_wll_ULMFields, int );

extern double 	edg_wll_ULMDateToDouble( const char *s );
void 		edg_wll_ULMDateToTimeval( const char *s, struct timeval *tv );
extern int 	edg_wll_ULMTimevalToDate( long sec, long usec, char *dstr );

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_ULM_PARSE_H */
