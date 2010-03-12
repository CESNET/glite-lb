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

#ifndef GLITE_JP_CONTEXT_H
#define GLITE_JP_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

int glite_jp_init_context(glite_jp_context_t *);
void glite_jp_free_context(glite_jp_context_t);
void glite_jp_free_query_rec(glite_jp_query_rec_t *);

char *glite_jp_peer_name(glite_jp_context_t);
char *glite_jp_error_chain(glite_jp_context_t);

int glite_jp_stack_error(glite_jp_context_t, const glite_jp_error_t *);
int glite_jp_clear_error(glite_jp_context_t); 

int glite_jp_add_deferred(glite_jp_context_t,int (*)(glite_jp_context_t,void *),void *);
int glite_jp_run_deferred(glite_jp_context_t);


#ifdef __cplusplus
};
#endif

#endif /* GLITE_JP_CONTEXT_H */
