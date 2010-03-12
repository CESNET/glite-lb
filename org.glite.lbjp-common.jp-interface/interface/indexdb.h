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

#ifndef GLITE_JP_INDEXDB_H
#define GLITE_JP_INDEXDB_H

/**
 * \file indexdb.h
 * \brief Helper functions for accessing Job Provenance Index Server database.
 */

#include "glite/lbu/db.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef GLITE_JP_INDEX_COMPILE

/**
 * For generating table names.
 */
#define GLITE_JP_INDEXDB_TABLE_ATTR_PREFIX "attr_"

#endif


/**
 * Returns internal id from attribute name.
 *
 * The attribute id is used in some places in the database schema. Because of the future possible changes in the schema, you should rather use other functions to get SQL commands (table names and where conditions).
 *
 * \param[in] name	attribute name
 * \return		attribute id string
 */
char *glite_jp_indexdb_attr2id(const char *name);

/**
 * Get parts of the SQL SELECT command to the given attribute:
 *
 *   SELECT column, full_value_column FROM table WHERE where;
 *
 * This is quick version requiring additional information about indexing of the attribute.
 */
int glite_jp_indexdb_attr2select_from_index(const char *name, int indexed, char **column, char **full_value_column, char **table, char **where);

/**
 * Get parts of the SQL SELECT command to the given attribute:
 *
 *   SELECT column, full_value_column FROM table WHERE where;
 *
 * This is the most portable way - it will peep to the DB for information about indexing.
 */
int glite_jp_indexdb_attr2select_from_db(const char *name, glite_lbu_DBContext dbctx, char **column, char **full_value_column, char **table, char **where);

#ifdef __cplusplus
};
#endif


#endif /* GLITE_JP_INDEXDB_H */
