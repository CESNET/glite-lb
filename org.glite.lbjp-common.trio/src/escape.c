#ident "$Header$"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "escape.h"

#define ULM_QM  '"'
#define ULM_BS  '\\'
#define ULM_LF  '\n'

/*
 *----------------------------------------------------------------------
 * 
 * \fn char *glite_lbu_EscapeULM(const char *str)
 * \param str		a string to escape
 * \return 		new (allocated) escaped string
 * \brief in given string escape all ULM_QM, ULM_BS and ULM_LF by ULM_BS
 *
 * Calls: malloc, strlen
 *
 * Algorithm: array lookup
 *             - the new string will be allocated
 *
 *----------------------------------------------------------------------
 */

char *glite_lbu_EscapeULM(const char *str)
{
unsigned int i,j;
size_t size;
char *ret;

if (str == NULL) return NULL;
if ((size = strlen(str)) == 0) return strdup("");

ret = (char*) malloc(1+2*size*sizeof(char));

j = 0;
for (i=0; i<size; i++) {
	if ((str[i] != ULM_BS) && (str[i] != ULM_QM) && (str[i] != ULM_LF)) {
		ret[j] = str[i];
		j++;
	}
	else {
		ret[j] = ULM_BS;
		if (str[i] == ULM_LF) {
			ret[j+1] = 'n';
		}
		else {
			ret[j+1] = str[i];
		}
		j += 2;
	}
} /* for */

ret[j] = 0;

return ret;
}

/*
 *----------------------------------------------------------------------
 * 
 * \fn char *glite_lbu_UnescapeULM(const char *str)
 * \param str		a string to unescape
 * \return		new (allocated) unescaped string
 * \brief in given string unescape all escaped ULM_QM, ULM_BS and ULM_LF
 *
 * Calls: malloc, strlen
 *
 * Algorithm: array lookup
 *               - the new string will be allocated
 *
 *----------------------------------------------------------------------
 */

char *glite_lbu_UnescapeULM(const char *str)
{
unsigned int i,j;
size_t size;
char *ret;

if (str == NULL) return NULL;

size  = strlen(str);
ret = (char*) malloc(1+size*sizeof(char));

/*
j = 0;
for (i=0; i<size; i++) {
	if ( (str[i] != ULM_BS) || 
	   ((str[i] == ULM_BS) && ((str[i+1] != ULM_BS) && (str[i+1] != ULM_QM) && (str[i+1] != 'n'))) )
	{
		if (str[i] == ULM_LF) { ret[j] = 'n'; }
		else { ret[j] = str[i]; }
		j++;
	}
} 
*/
for (i=j=0; i<size; i++,j++)
	if (str[i] == ULM_BS) switch(str[++i]) {
		case 'n': ret[j] = ULM_LF; break;
		default: ret[j] = str[i]; break;
	} else { ret[j] = str[i]; }

ret[j] = '\0';

return ret;
}

static const struct {
	const char c,*e;
} xml_etab[] = {
	{ '<',"lt" },
	{ '>',"gt" },
	{ '&',"amp" },
	{ '"',"quot" },
	{ '\'',"apos" },
	{ 0, NULL }
};

#define XML_ESCAPE_SET	"<>&\"'"

char *glite_lbu_EscapeXML(const char *in)
{
        const char* tmp_in;
	char	*out;
	int	cnt,i,j,k;

	if (!in) return NULL;

	for (cnt = 0, tmp_in = in; *tmp_in != '\0'; ++tmp_in) {
	       	if (strchr(XML_ESCAPE_SET, *tmp_in) ||
			(*tmp_in & 0x7f) < 0x20 /* control character */ ||
			(*tmp_in == '%')) cnt++;
        }

	out = malloc(strlen(in)+1+cnt*5);

	for (i=j=0; in[i]; i++) {
		for (k=0; xml_etab[k].c && xml_etab[k].c != in[i]; k++);
		if (xml_etab[k].c) {
			int	l;

			out[j++] = '&';
			memcpy(out+j,xml_etab[k].e,l=strlen(xml_etab[k].e));
			j += l;
			out[j++] = ';';
		} else if ((in[i] & 0x7f) < 0x20 || in[i] == '%') {
			sprintf(out+j, "%%%02x", (unsigned char)in[i]);
			j+=3;
		} else {
			out[j++] = in[i];
		}
	}
	out[j] = 0;
	return out;
}

char *glite_lbu_UnescapeXML(const char *in)
{
	char	*out;
	int	i,j,k;
	char	xtmp[3];
	unsigned char	origchar;

	if (!in) return NULL;
	out = malloc(strlen(in)+1);

	for (i=j=0; in[i]; j++) if (in[i] == '&') {
		char	*s = strchr(in+i,';');
		if (s) {
			int	l = s-in-i+1;
			for (k=0; xml_etab[k].c && strncasecmp(in+i+1,xml_etab[k].e,l-2); k++);
			if (xml_etab[k].c) {
				out[j] = xml_etab[k].c;
				i += l;
			} else out[j] = in[i++];
		} else out[j] = in[i++];
	} else if (in[i] == '%') {
		if (isxdigit(xtmp[0]=in[i+1]) && isxdigit(xtmp[1]=in[i+2])) {
			xtmp[2] = '\0';
			origchar = (unsigned char) strtol(xtmp, NULL, 16);
			if ((origchar & 0x7f) < 0x20 || origchar == '%') {
				out[j] = origchar;
				i += 3;
			} else out[j] = in[i++];
		} else out[j] = in[i++];
	} else {
		out[j] = in[i++];
	}
	out[j] = 0;
	return out;
}

char *glite_lbu_EscapeSQL(const char *in)
{
        const char* tmp_in;
	char	*out = NULL;
	int	i,j,cnt;

	if (!in) return NULL;

	for (cnt = 0, tmp_in = in; (tmp_in = strchr(tmp_in,'\'')) != NULL; ++tmp_in) {
          ++cnt;
        }
	for (tmp_in = in; (tmp_in = strchr(tmp_in,'\\')) != NULL; ++tmp_in) {
          ++cnt;
        }

	out = malloc(strlen(in)+1+cnt);

	for (i=j=0; in[i]; i++) {
		if (in[i] == '\\') out[j++] = '\\';
		if (in[i] == '\'') out[j++] = '\'';
		out[j++] = in[i];
	}
	out[j] = 0;

	return out;
}
