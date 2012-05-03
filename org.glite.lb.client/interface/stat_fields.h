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

#ifdef __cplusplus
extern "C" {
#endif

char * glite_lb_parse_stat_fields(const char *,void **);
void glite_lb_print_stat_fields(void **,edg_wll_JobStat *);
void glite_lb_dump_stat_fields(void);

#include <time.h>
#include <stdio.h>

static char *TimeToStr(time_t t) 
{
	struct tm   *tm = gmtime(&t);
	static	char	tbuf[1000];

	sprintf(tbuf,"'%4d-%02d-%02d %02d:%02d:%02d UTC'",
			tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour,tm->tm_min,tm->tm_sec);

	return tbuf;
}


#ifdef __cplusplus
}
#endif
