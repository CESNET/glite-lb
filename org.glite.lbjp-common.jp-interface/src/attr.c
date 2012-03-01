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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "glite/jobid/strmd5.h"
#include "types.h"
#include "attr.h"
#include "type_plugin.h"
#include "context.h"

void glite_jp_attrval_free(glite_jp_attrval_t *a,int f)
{
	free(a->name);
	free(a->value);
	free(a->origin_detail);
	memset(a,0,sizeof *a);
	if (f) free(a);
}

void glite_jp_attrval_copy(glite_jp_attrval_t *dst,const glite_jp_attrval_t *src)
{
	dst->name = strdup(src->name);
	dst->origin = src->origin;
	dst->size = src->size;
	dst->timestamp = src->timestamp;
	dst->origin_detail = src->origin_detail ? 
		strdup(src->origin_detail) : NULL;
	if ((dst->binary = src->binary)) {
		dst->value = malloc(src->size);
		memcpy(dst->value,src->value,src->size);
	}
	else dst->value = strdup(src->value);
}


#define min(x,y) ((x) > (y) ? (y) : (x))

static int fb_cmp(void *ctx,const glite_jp_attrval_t *a,const glite_jp_attrval_t *b,int *result)
{
	if (a->binary != b->binary) return EINVAL;
	if (a->binary) {
		*result = memcmp(a->value,b->value,min(a->size,b->size));
		if (!*result && a->size != b->size) 
			*result = a->size > b->size ? 1 : -1;
	}
	else *result = strcmp(a->value,b->value);
	return 0;
}

/* XXX: depends on specific definition of glite_jp_attr_orig_t */
static char orig_char[] = "ASUF";

/* XXX: don't allocate memory, don't grow more than twice */
static int escape_colon(const char *in, char *out)
{
	int	i,o;

	for (i=o=0; in[i]; i++) switch (in[i]) {
		case ':': out[o++] = '\\'; out[o++] = ':'; break;
		case '\\': out[o++] = '\\'; out[o++] = '\\'; break;
		default: out[o++] = in[i]; break;
	}
	out[o] = 0;
	return o;
}

/* XXX: read until unescaped colon is found 
 *      allocates output */
static char * unescape_colon(const char *in,int *rd)
{
	int	i,o;
	char	*out;

	for (i=o=0; in[i] && in[i] != ':'; i++,o++)
		if (in[i] == '\\') i++;

	out = malloc(o+1);

	for (i=o=0; in[i] && in[i] != ':'; i++)
		if (in[i] == '\\') out[o++] = in[++i]; 
		else out[o++] = in[i];

	out[o] = 0;
	*rd = i;
	return out;
}

static char * fb_to_db_full(void *ctx,const glite_jp_attrval_t *attr)
{
	
	int	vsize = attr->binary ? attr->size * 4/3 + 6 : strlen(attr->value)+1,
		len;

	/* 4x: + \0 + ASUF + BS + %12d */
	char	*db = malloc(19 + (attr->origin_detail ? 2*strlen(attr->origin_detail) : 0) + vsize);

	if (attr->origin < 0 || attr->origin > GLITE_JP_ATTR_ORIG_FILE) {
		free(db); return NULL; 
	}
	len = sprintf(db,"%c:%ld:%c:",attr->binary ? 'B' : 'S',
		attr->timestamp,orig_char[attr->origin]);

	if (attr->origin_detail) len += escape_colon(attr->origin_detail,db+len);
	db[len++] = ':';

	if (attr->binary) {
		vsize = base64_encode(attr->value,attr->size,db+len,vsize-1);
		if (vsize < 0) { free(db); return NULL; }
		db[len+vsize] = 0;
	}
	else strcpy(db+len,attr->value);

	return db;
}

static char * fb_to_db_index(void *ctx,const glite_jp_attrval_t *attr,int len)
{
	char	*s;

/* XXX: binary values not really handled. Though the formal semantics is not broken */
	if (attr->binary) return strdup("XXX"); 

	s = strdup(attr->value);
	if (len < strlen(s)) s[len] = 0;
	return s;
}

static int fb_from_db(void *ctx,const char *str,glite_jp_attrval_t *attr)
{
	int 	p = 2;
	char	*colon,*cp;

	if (str[0] != 'B' && str[0] != 'S') return EINVAL;
	attr->binary = str[0] == 'B';
	cp = attr->value = strdup(str);
	
	colon = strchr(cp+p,':');
	if (!colon) return EINVAL;

	*colon++ = 0;
	attr->timestamp = (time_t) atol(cp+p);
	p = colon-cp;

	for (attr->origin = GLITE_JP_ATTR_ORIG_ANY; orig_char[attr->origin] && orig_char[attr->origin] != cp[p]; attr->origin++);
	if (!orig_char[attr->origin]) return EINVAL;

	p += 2;
	if (cp[p] == ':') attr->origin_detail = NULL;
	else {
		int	r;
		attr->origin_detail = unescape_colon(cp+p,&r);
		p += r;
	}
	if (cp[p++] != ':') return EINVAL;

	if (attr->binary) {
		attr->size = base64_decode(str+p,attr->value,strlen(str));
		if (attr->size < 0) return EINVAL;
	}
	else strcpy(attr->value,str+p);

	return 0;
}

static const char * fb_type_full(void *ctx,const char *attr)
{
	return "mediumblob";
}

static const char * fb_type_index(void *ctx,const char *attr,int len)
{
	static char tbuf[100];
	sprintf(tbuf,"varchar(%d)",len);
	return tbuf;
}



static glite_jp_tplug_data_t fallback_plugin = {
	"",
	NULL,
	fb_cmp,
	fb_to_db_full,
	fb_to_db_index,
	fb_from_db,
	fb_type_full,
	fb_type_index,
};

static glite_jp_tplug_data_t *get_plugin(glite_jp_context_t ctx,const char *aname)
{
	void	**cp = ctx->type_plugins;
	char	*colon,*ns;

	if (!cp) return &fallback_plugin;
	glite_jp_clear_error(ctx);
	ns = strdup(aname);
	colon = strrchr(ns,':');
	if (colon) *colon = 0; else *ns = 0;

	while (*cp) {
		glite_jp_tplug_data_t	*p = *cp;
		if (!strcmp(ns,p->namespace)) {
			free(ns);
			return p;
		}
		cp++;
	}
	free(ns);
	return &fallback_plugin;	/* XXX: is it always desirable? */
}

int glite_jp_attrval_cmp(glite_jp_context_t ctx,const glite_jp_attrval_t *a,const glite_jp_attrval_t *b,int *result)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,a->name);
	glite_jp_error_t	err;

	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	glite_jp_clear_error(ctx);

	if (strcmp(a->name,b->name)) {
		err.code = EINVAL;
		err.desc = "Can't compare different attributes";
		return glite_jp_stack_error(ctx,&err);
	}

	return ap->cmp ? ap->cmp(ap->pctx,a,b,result) : fb_cmp(ap->pctx,a,b,result);
}

char *glite_jp_attrval_to_db_full(glite_jp_context_t ctx,const glite_jp_attrval_t *attr)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr->name);

	glite_jp_clear_error(ctx);
	return ap->to_db_full ? ap->to_db_full(ap->pctx,attr) : fb_to_db_full(ap->pctx,attr);
}

char *glite_jp_attrval_to_db_index(glite_jp_context_t ctx,const glite_jp_attrval_t *attr,int len)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr->name);

	glite_jp_clear_error(ctx);
	return ap->to_db_index ? ap->to_db_index(ap->pctx,attr,len) : fb_to_db_index(ap->pctx,attr,len);
}


int glite_jp_attrval_from_db(glite_jp_context_t ctx,const char *str,glite_jp_attrval_t *attr)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr->name);

	glite_jp_clear_error(ctx);
	return ap->from_db ? ap->from_db(ap->pctx,str,attr) : fb_from_db(ap->pctx,str,attr);
}

const char *glite_jp_attrval_db_type_full(glite_jp_context_t ctx,const char *attr)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr);

	glite_jp_clear_error(ctx);
	return ap->db_type_full ? ap->db_type_full(ap->pctx,attr) : fb_type_full(ap->pctx,attr);
}

const char *glite_jp_attrval_db_type_index(glite_jp_context_t ctx,const char *attr,int len)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr);

	glite_jp_clear_error(ctx);
	return ap->db_type_index ?  ap->db_type_index(ap->pctx,attr,len) : fb_type_index(ap->pctx,attr,len);
}

/* XXX: UNIX time, should be ISO blahblah */
time_t glite_jp_attr2time(const char *a)
{
	long	t;

	sscanf(a,"%ld",&t);
	return t;
}

/* XXX: UNIX time, should be ISO blahblah */
char * glite_jp_time2attr(time_t t)
{
	char	*r;

	asprintf(&r,"%ld",(long) t);
	return r;
}

