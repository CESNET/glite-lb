#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/*========= DATA =====================================================*/

#include "glite/lb/ulm_parse.h"

/*========= FUNCTIONS ================================================*/

/* -- Internal function prototype -- */
void edg_wll_ULMSplitDate( const char *s,
                      unsigned int *year,
                      unsigned int *mon,
                      unsigned int *day,
                      unsigned int *hour,
                      unsigned int *min,
                      unsigned int *sec,
                      unsigned long *usec );
int edg_wll_ULMisalphaext( int c);

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_ULMNewParseTable -- Allocate memory for new parse table
 *
 * Calls: malloc, strdup
 *
 * Algorithm:
 *
 *----------------------------------------------------------------------
 */
p_edg_wll_ULMFields edg_wll_ULMNewParseTable(LogLine logline)
{
  p_edg_wll_ULMFields this = (p_edg_wll_ULMFields) calloc(1,sizeof(edg_wll_ULMFields));
  LogLine ln = logline;

  /* Strip leading spaces */
  for ( ; *ln && isblank(*ln); ln++ );

  this->names = NULL;
  this->vals = NULL;
  this->num = 0;
  this->raw = strdup(ln);

  return this;
}

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_ULMFreeParseTable -- Free memory allocated for parse table
 *
 * Calls: free
 *
 * Algorithm:
 *
 *----------------------------------------------------------------------
 */
void edg_wll_ULMFreeParseTable(p_edg_wll_ULMFields this)
{   
   EDG_WLL_ULM_CLEAR_FIELDS(this);
   if (this->raw) free(this->raw);
   if (this) free(this);
}


/*
 *----------------------------------------------------------------------
 *
 * edg_wll_ULMProcessParseTable -- Break UML message into fields.
 *
 * Calls: strchr, strrchr
 *
 * Algorithm: 
 *	LogLine is of the ULM form as described in draft-abela-ulm-05.
 *	- the ULM_EQ symbol separates name,value pairs 
 *	- quote marks must be taken into account 
 *	1. count ULM_EQ and ULM_SP symbols,
 *	2. allocate the integer arrays 'names' and 'vals' which hold 
 *	   the indices of subsequent name and value strings,
 *	3. the original raw data will be peppered with \0's so that raw[index] 
 *	   will give the proper string if index is in names[] or vals[]
 * 
 *      The actual algorithm to get these indices is a simple array lookup.
 *
 *	The following illegal formats are caught:
 *		1. no name=value pairs
 *		2. space or tab next to delimiter
 *		3. logline starts or ends with delimiter
 *		4. no spaces between successive delimiters
 *		5. illegal character after ending ULM_QM
 *
 *----------------------------------------------------------------------
 */
int edg_wll_ULMProcessParseTable(p_edg_wll_ULMFields this)
{
  char *func = "edg_wll_ULMProcessParseTable";
  char *eq;
  int  i,j;
  int  eqCnt,qmCnt,spCnt;
  int  iArrayEQ[ULM_FIELDS_MAX];
  int  iArraySP[ULM_FIELDS_MAX];
  size_t size;

  if ( (this == NULL) || (this->raw == NULL) || (*this->raw == '\0')) {
    fprintf(stderr,"%s: PARSE ERROR: Nothing to parse.\n",func);
    return ULM_PARSE_ERROR;                                     
  }

  /* Init data */
  EDG_WLL_ULM_CLEAR_FIELDS(this);

  for (i=0; i<ULM_FIELDS_MAX; i++) { 
    iArrayEQ[i] = 0;
    iArraySP[i] = 0;
  }

  i = j = 0;
  qmCnt = eqCnt = spCnt = 0;

  size = strlen(this->raw);

  /* Count number of ULM_EQ and ULM_SP 
   * and replace all ULM_LF by nul characters */
  for (i=0; i< (int)size; i++) {
    switch (this->raw[i]) {
      case ULM_SP :
      case ULM_TB :
         if (qmCnt == 0) { iArraySP[spCnt] = i; spCnt++; }
         break;
      case ULM_EQ :
	 if (i==0) {
		fprintf(stderr,"%s: PARSE ERROR: '%c' at the beginning of log line.\n", func, ULM_EQ);
		return ULM_PARSE_ERROR;
	 }
         if (qmCnt == 0) { 
           if (isblank(this->raw[i-1]) || (!edg_wll_ULMisalphaext(this->raw[i-1]))) {
             fprintf(stderr,"%s: PARSE ERROR: Disallowed character ('%c') or space before delimiter '%c'.\n",
                     func,this->raw[i-1],ULM_EQ);
             return ULM_PARSE_ERROR;
           }
           if (isblank(this->raw[i+1]) || ((!edg_wll_ULMisalphaext(this->raw[i-1])) && (this->raw[i+1] != ULM_QM ))) {
             fprintf(stderr,"%s: PARSE ERROR: Disallowed character ('%c') or space after delimiter '%c'.\n",
                     func,this->raw[i+1],ULM_EQ);
             return ULM_PARSE_ERROR;
           }
           iArrayEQ[eqCnt] = i; 
           eqCnt++; 
         }
         break;
      case ULM_LF :
         if (qmCnt == 0) { this->raw[i] = '\0'; }
	 break;
      case ULM_QM :
         if (this->raw[i-1] != ULM_BS) {
           if (qmCnt == 0) qmCnt++;
           else qmCnt--;
         }              
         if ((qmCnt == 0) && (!isspace(this->raw[i+1]) && (this->raw[i+1] != '\0'))) {
             fprintf(stderr,"%s: PARSE ERROR: Disallowed character ('%c') after ending '%c'at i=%d size=%lu char=%d.\n",
                     func,this->raw[i+1],ULM_QM,i,(unsigned long)size,this->raw[i+1]);
             for (j=0; j<=i; j++) fputc(this->raw[j],stderr);
             fputc(ULM_LF,stderr);
             return ULM_PARSE_ERROR;
         }
         break;
      default :
         break;
    } /* switch */
  } /* for */

  if (eqCnt == 0) {
    fprintf(stderr,"%s: PARSE ERROR: No '%c' in line \"%s\"\n",func,ULM_EQ,this->raw);
    return ULM_PARSE_ERROR;                    
  }

  if (this->raw[0] == ULM_EQ) {
    fprintf(stderr,"%s: PARSE ERROR: Need at least 1 letter for the first field name.\n",func);
    return ULM_PARSE_ERROR;                    
  }

  if (qmCnt != 0) {
    fprintf(stderr,"%s: PARSE ERROR: Last quoted value did not finish.\n",func);
    return ULM_PARSE_ERROR;                    
  }

  /* Allocate names, vals arrays */
  this->names = (int *) malloc(eqCnt*sizeof(int));
  this->vals =  (int *) malloc(eqCnt*sizeof(int));

  this->names[0] = (int)(0);
  this->vals[0]  = (int)(iArrayEQ[0] + 1);

  /* 
   * Main loop 
   */
  for (i=1; i<eqCnt; i++) {
    eq = &this->raw[iArrayEQ[i]]; 
    j = 1;
    while (edg_wll_ULMisalphaext(*(eq-j))) {
      j++;
    }
    if (isblank(*(eq-j))) {
       this->names[i] = (int)(iArrayEQ[i] - j + 1);
       this->vals[i]  = (int)(iArrayEQ[i] + 1);
    }
    else {
       fprintf(stderr,"%s: PARSE ERROR: Disallowed character '%c' for field name \
(e.g. no space between successive delimiters).\n",func,*(eq-j));
       return ULM_PARSE_ERROR; 
    }
  } /* for */

  /* Replace delimiters and intervening whitespace by nul characters */
  for (i=0; i<eqCnt; i++) this->raw[iArrayEQ[i]] = '\0';   
  for (i=0; i<spCnt; i++) this->raw[iArraySP[i]] = '\0';

  this->num = eqCnt;

  /* Debug dump of parsed fields */
//  for( i=0; i<eqCnt; i++ ) fprintf(stdout,"field[%d]: %s=%s\n",i,this->raw+this->names[i],this->raw+this->vals[i]);

  return ULM_PARSE_OK;
}

/*
 *---------------------------------------------------------------------------
 * edg_wll_ULMisalphaext - test if the character is an ALPHAEXT as described in 
 *              draft-abela-ulm-05
 *---------------------------------------------------------------------------
 */
int edg_wll_ULMisalphaext( int c) {

  return (isalnum(c) || (c == '.') || (c == '-') || c == '_');
}

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_ULMGetNameAt -- Get name at index.
 *
 * Calls: 
 *
 * Algorithm: array lookup
 *
 *----------------------------------------------------------------------
 */
char *edg_wll_ULMGetNameAt( p_edg_wll_ULMFields this, int index )
{
  if ( index < 0 || index > this->num )
    return NULL;
  return (char *)&(this->raw[this->names[index]]);
}

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_ULMGetValueAt -- Get value at index
 *
 * Calls: 
 *
 * Algorithm: array lookup
 *
 *----------------------------------------------------------------------
 */
char *edg_wll_ULMGetValueAt( p_edg_wll_ULMFields this, int index ) 
{
  if ( index < 0 || index > this->num )
    return NULL;
  return (char *)&(this->raw[this->vals[index]]);
}

/*
 *---------------------------------------------------------------------------
 * edg_wll_ULMDateToDouble -- Calculate date in sec. since 1/1/1970 from string.
 *                Algorithm borrowed from linux kernel source code,
 *                i.e. Linus Torvalds, who in turn credits it to Gauss.
 *
 * PRE: Input is properly formatted, non-null, need _not_ be null-terminated.
 * IN : String in format YYYYMMDDHHmmss.uuuuuu
 *        YYYY  = 4 digit year
 *        MM    = 2 digit month (1..12)
 *        DD    = 2 digit day of month (1..31)
 *        HH    = 2 digit hour of day ( 0..23 )
 *        mm    = 2 digit minute of hour ( 0..59 )
 *        ss    = 2 digit second of minute ( 0..59 )
 *        uuuuuu= 1..6 digit microsecond of second ( 0..999999 )
 * OUT: Number of seconds, and fraction accurate to at most microseconds,
 *      elapsed since 1/1/1970 (GMT).
 *
 * edg_wll_ULMDateToTimeval -- the same, except it returns timeval
 *---------------------------------------------------------------------------
 */
double edg_wll_ULMDateToDouble( const char *s )
{
  unsigned int year, mon, day, hour, min, sec=0;
  unsigned long usec=0L;

  edg_wll_ULMSplitDate( s, &year, &mon, &day, &hour, &min, &sec, &usec );

  if (0 >= (int) (mon -= 2)) {    /* 1..12 -> 11,12,1..10 */
    mon += 12;      /* Puts Feb last since it has leap day */
    year -= 1;
  }
  return (double)
    ( ( ( (
           (unsigned long)(year/4 - year/100 + year/400 + 367*mon/12 + day) +
           year*365 - 719499
           )*24 + hour /* now have hours */
          )*60 + min /* now have minutes */
        )*60 + sec /* seconds */
      ) + (double)( usec / 1E6 ); /* microseconds */
}

void edg_wll_ULMDateToTimeval( const char *s, struct timeval *tv )
{
  unsigned int year, mon, day, hour, min, sec=0;
  unsigned long usec=0L;

  edg_wll_ULMSplitDate( s, &year, &mon, &day, &hour, &min, &sec, &usec );

  if (0 >= (int) (mon -= 2)) {    /* 1..12 -> 11,12,1..10 */
    mon += 12;      /* Puts Feb last since it has leap day */
    year -= 1;
  }
  tv->tv_sec = (long)
    ( ( ( (
           (unsigned long)(year/4 - year/100 + year/400 + 367*mon/12 + day) +
           year*365 - 719499
           )*24 + hour /* now have hours */
          )*60 + min /* now have minutes */
        )*60 + sec /* seconds */
      );
  tv->tv_usec = usec; /* microseconds */
}

/*
 *---------------------------------------------------------------------------
 * edg_wll_ULMSplitDate -- Efficiently break date into parsed component parts.
 *---------------------------------------------------------------------------
 */
#define DIG(x) ((int)((x)-'0'))
void edg_wll_ULMSplitDate( const char *s,
                      unsigned int *year,
                      unsigned int *mon,
                      unsigned int *day,
                      unsigned int *hour,
                      unsigned int *min,
                      unsigned int *sec,
                      unsigned long *usec )
{
   *year = DIG(s[0]) * 1000 + DIG(s[1]) * 100 + DIG(s[2]) * 10 + DIG(s[3]);
   *mon  = DIG(s[4]) * 10 + DIG(s[5]);
   *day  = DIG(s[6]) * 10 + DIG(s[7]);
   *hour = DIG(s[8]) * 10 + DIG(s[9]);
   *min  = DIG(s[10]) * 10 + DIG(s[11]);
   *sec  = DIG(s[12]) * 10 + DIG(s[13]);
   if ( s[14] == '.' ) *usec = atol(s+15);
}
#undef DIG

/*
 *---------------------------------------------------------------------------
 * edg_wll_ULMTimevalToDate -- Take a seconds, microseconds argument and convert it
 *                   to a date string that edg_wll_ULMDateToDouble could parse.
 *
 * CALL: gmtime, strftime
 *
 * PRE : seconds, usec >= 0 , usec < 1000000 (checked)
 *       date string has DATE_STRING_LENGTH+1 bytes allocated (not checked)
 * IN  : seconds, useconds
 * OUT : date string 'dstr'.
 * RTRN: 0=OK, other=FAILURE
 * POST: This is the inverse of edg_wll_ULMDateToDouble, i.e.
 *         edg_wll_ULMDateToDouble( edg_wll_ULMTimevalToDate( sec, usec ) ) = sec.usec
 *---------------------------------------------------------------------------
 */
int edg_wll_ULMTimevalToDate( long sec, long usec, char *dstr )
{
  char *func = "edg_wll_ULMTimevalToDate";
  struct tm tms;
  struct tm *tp;
  int        len;

  if ( sec < 0 || usec < 0 || usec > 999999 )
    return 1;

  tp = gmtime_r( (const time_t *) &sec, &tms );
  if ( tp == NULL )
    return 1;

  len = strftime( dstr,
                  ULM_DATE_STRING_LENGTH+1-7,
                  "%Y%m%d%H%M%S",
                  tp );
  if ( len != ULM_DATE_STRING_LENGTH-7 ) {
    fprintf(stderr,"%s: bad strftime() return value: %d\n",func, len);
    return 1;
  }

  sprintf( dstr + ULM_DATE_STRING_LENGTH-7, ".%06ld", usec );

  return 0;
}

