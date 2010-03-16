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


#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char *edg_wll_TimeToDB(time_t t)
{
	struct tm	*tm = gmtime(&t);
	char	tbuf[256];

	sprintf(tbuf,"'%4d-%02d-%02d %02d:%02d:%02d'",tm->tm_year+1900,tm->tm_mon+1,
		tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	
	return strdup(tbuf);
}

time_t edg_wll_DBToTime(char *t)
{
	struct tm	tm;

	memset(&tm,0,sizeof(tm));
	setenv("TZ","UTC",1); tzset();
	sscanf(t,"%4d-%02d-%02d %02d:%02d:%02d",
		&tm.tm_year,&tm.tm_mon,&tm.tm_mday,
		&tm.tm_hour,&tm.tm_min,&tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;

	return mktime(&tm);
}
