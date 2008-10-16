#ifdef __cplusplus
extern "C" {
#endif

char * glite_lb_parse_stat_fields(const char *,void **);
void glite_lb_print_stat_fields(void **,edg_wll_JobStat *);
void glite_lb_dump_stat_fields(void);

#include <time.h>

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
