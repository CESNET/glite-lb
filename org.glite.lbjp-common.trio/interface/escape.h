#ifndef __EDG_WORKLOAD_LOGGING_COMMON_ESCAPE_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_ESCAPE_H__
/*!
 * \file escape.h
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



/*!
 * \fn char *glite_lbu_EscapeULM(const char *str)
 * \param str           a string to escape
 * \return              new (allocated) escaped string
 * \brief in given string (ULM) escape all ULM_QM, ULM_BS and ULM_LF by ULM_BS
 */

char *glite_lbu_EscapeULM(const char *);


/*!
 * \fn char *glite_lbu_UnescapeULM(const char *str)
 * \param str           a string to unescape
 * \return              new (allocated) unescaped string
 * \brief in given string (ULM) unescape all escaped ULM_QM, ULM_BS and ULM_LF
 */

char *glite_lbu_UnescapeULM(const char *);


/*!
 * \fn char *glite_lbu_EscapeXML(const char *str);
 * \param str		a string to escape
 * \return		new (allocated) escaped string
 * \brief in given string (XML) escape all unwanted characters
 */

char *glite_lbu_EscapeXML(const char *);


/*!
 * \fn char *glite_lbu_UnescapeXML(const char *str)
 * \param str		a string to unescape
 * \return		new (allocated) unescaped string
 * \brief in given string (XML) unescape all escaped characters
 */

char *glite_lbu_UnescapeXML(const char *);


/*!
 * \fn char *glite_lbu_EscapeSQL(const char *str)
 * \param str		a string to escape
 * \return		new (allocated) escaped string
 * \brief in given string (SQL) escape all unwanted characters
 */

char *glite_lbu_EscapeSQL(const char *);


/*!
 * \fn char *glite_lbu_EscapeJSON(const char *str)
 * \param str		a string to escape
 * \return		new (allocated) escaped string
 * \brief in given string (JSON) escape all unwanted characters
 */

char *glite_lbu_EscapeJSON(const char *in);

#endif /* __EDG_WORKLOAD_LOGGING_COMMON_ESCAPE_H__ */
