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
 * \return 		new (allocated) escaped string (is is empty "" if str is NULL)
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

if (str == NULL) return strdup("");
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
 * \return		new (allocated) unescaped string or NULL (if str NULL or empty)
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
if (size == 0) return NULL;

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

char *glite_lbu_EscapeJSON(const char *in) {
	const char * tmp_in;
	char	*out = NULL;
	int	i,j,cnt;

	if (!in) return NULL;

	for (cnt = 0, tmp_in = in; *tmp_in; tmp_in++) {
		switch (*tmp_in) {
		case '"':
		case '\\':
		case '/':
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
			cnt++;
			break;
		default:
			if ((unsigned char)*tmp_in < 0x20) cnt += 6;
			break;
		}
	}

	out = malloc(strlen(in)+1+cnt);

	for (i=j=0; in[i]; i++) {
		switch (in[i]) {
		case '"':
		case '\\':
		case '/':
			out[j++] = '\\';
			out[j++] = in[i];
			break;
		case '\b':
			out[j++] = '\\';
			out[j++] = 'b';
			break;
		case '\f':
			out[j++] = '\\';
			out[j++] = 'f';
			break;
		case '\n':
			out[j++] = '\\';
			out[j++] = 'n';
			break;
		case '\r':
			out[j++] = '\\';
			out[j++] = 'r';
			break;
		case '\t':
			out[j++] = '\\';
			out[j++] = 't';
			break;
		default:
			if ((unsigned char)in[i] < 0x20) {
				snprintf(out + j, 7, "\\u%04x", in[i]);
				j += 6;
			} else
				out[j++] = in[i];
			break;
		}
	}
	out[j] = 0;

	return out;
}

char *glite_lbu_UnescapeURL(const char *in) {
	char *out;
	char *spec = in;
	char *reserved;
	unsigned int val;

	if(!in) return NULL;
	out = (char*)calloc(strlen(in)+1, sizeof(char));

	strncpy(out, spec, strcspn(spec, "%")); // Copy the first part of the string up to the first '%'

	while ((spec = strchr(spec, '%'))) {
		if(sscanf(spec+1, "%02X", &val)) { //Treat as percent-escaped hex if the format is right
			asprintf(&reserved,"%c", val);
			strcat(out, reserved);
			free(reserved);
			spec = spec + 3;
		}
		else spec = spec + 1; //Otherwise just skip the '%' sign. This should not happen. This increment is here just to prevent cycle lockups in case of browser malfunction.
		strncpy(out+strlen(out), spec, strcspn(spec, "%"));
	}

	return(out);
}

