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

#ifndef GLITE_JP_ATTR_H
#define GLITE_JP_ATTR_H

#ifdef __cplusplus
extern "C" {
#endif

void glite_jp_attrval_free(glite_jp_attrval_t *,int);
void glite_jp_attrval_copy(glite_jp_attrval_t *,const glite_jp_attrval_t *);

/* Search through registered type plugins and call appropriate plugin method.
 * See type_plugin.h for detailed description.
 */

int glite_jp_attrval_cmp(glite_jp_context_t ctx,const glite_jp_attrval_t *a,const glite_jp_attrval_t *b,int *result);

char *glite_jp_attrval_to_db_full(glite_jp_context_t ctx,const glite_jp_attrval_t *attr);
char *glite_jp_attrval_to_db_index(glite_jp_context_t ctx,const glite_jp_attrval_t *attr,int len);

int glite_jp_attrval_from_db(glite_jp_context_t ctx,const char *str,glite_jp_attrval_t *attr);
const char *glite_jp_attrval_db_type_full(glite_jp_context_t ctx,const char *attr);
const char *glite_jp_attrval_db_type_index(glite_jp_context_t ctx,const char *attr,int len);

time_t glite_jp_attr2time(const char *);
char * glite_jp_time2attr(time_t);

#ifdef __cplusplus
};
#endif


#endif /* GLITE_JP_ATTR_H */
