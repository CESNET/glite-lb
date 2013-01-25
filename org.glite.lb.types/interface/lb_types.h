#ifndef GLITE_LB_TYPES_H
#define GLITE_LB_TYPES_H

/**
 * \file lb_types.h
 * \brief L&B API common types and related definitions
 */
 

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



#ifdef __cplusplus
extern "C" {
#endif

/*!
 *
 * Pair tag = value.
 */
typedef struct _edg_wll_TagValue {
        char *  tag;    /**< User-specified information tag */
        char *  value;  /**< Value assigned to user-specified information tag */
} edg_wll_TagValue;


/**
 * Free allocated edg_wll_TagValue * list
 * \param list IN: list to free
 */
void edg_wll_FreeTagList(edg_wll_TagValue *list);

/**
 * Deep copy the tag list
 */
edg_wll_TagValue *edg_wll_CopyTagList(edg_wll_TagValue *src);

/**
 * Function for parsing name=value tag lists
 */
int edg_wll_TagListParse(const char *src, edg_wll_TagValue **list);

/**
 * Comparing tag list values
 */
int edg_wll_TagListCompare(edg_wll_TagValue *a, edg_wll_TagValue *b);

/**
 *  Function for stringifying name=value tag lists
 */
char * edg_wll_TagListToString(edg_wll_TagValue *list);


#ifdef __cplusplus
}
#endif

#endif
