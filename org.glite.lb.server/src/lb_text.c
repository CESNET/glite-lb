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


#include "lb_text.h"
#include "lb_proto.h"
#include "cond_dump.h"
#include "server_state.h"
#include "authz_policy.h"

#include "glite/lb/context-int.h"
#include "glite/lb/xml_conversions.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/db.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

int edg_wll_QueryToText(edg_wll_Context ctx UNUSED_VAR, edg_wll_Event *eventsOut UNUSED_VAR, char **message UNUSED_VAR)
{
/* not implemented yet */
	return -1;
}

#define TR(name,type,field) \
{ \
	int l; \
        if (field) \
		l = asprintf(&a,"%s=" type "\n", \
			name, field); \
	else \
		l = asprintf(&a,"%s=\n", name); \
	b = realloc(b, sizeof(*b)*(pomL+l+1)); \
	strcpy(b+pomL, a); \
	pomL += l; \
	free(a); a=NULL; \
}

#define TRS(name,type,field) \
{ \
        int l; \
        if (field) \
                l = asprintf(&a,"%s=" type "", \
                        name, field); \
        else \
                l = asprintf(&a,"%s=\n", name); \
        b = realloc(b, sizeof(*b)*(pomL+l+1)); \
        strcpy(b+pomL, a); \
        pomL += l; \
        free(a); a=NULL; \
}

#define TRA(type,field) \
{ \
        int l; \
        if (field) \
                l = asprintf(&a,"," type "", \
                        field); \
        else \
                l = asprintf(&a,"\n"); \
        b = realloc(b, sizeof(*b)*(pomL+l+1)); \
        strcpy(b+pomL, a); \
        pomL += l; \
        free(a); a=NULL; \
}

int edg_wll_UserNotifsToText(edg_wll_Context ctx, char **notifids, char **message){
	char *a = NULL, *b;
        int i = 0;
        b = strdup("");

        while(notifids[i]){
                if (i == 0)
                        asprintf(&a, "%s", notifids[i]);
                else
                        asprintf(&a, "%s,%s", b, notifids[i]);
                free(b);
                b = a;
                i++;
        }
	if (b)
		asprintf(&a, "User_notifications=%s\n", b);

	*message = a;

	return 0;
}

char *edg_wll_ErrorToText(edg_wll_Context ctx,int code)
{
	char	*out,*et,*ed;
	char	*msg = edg_wll_HTTPErrorMessage(code);
	edg_wll_ErrorCode	e;

	e = edg_wll_Error(ctx,&et,&ed);
	asprintf(&out,"Error=%s\n"
		"Code=%d\n"
		"Description=%s (%s)\n"
		,msg,e,et,ed);

	free(et); free(ed);
	return out;
}

