#ifndef __EDG_WORKLOAD_LOGGING_COMMON_ESCAPE_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_ESCAPE_H__
/*!
 * \file escape.h
 */

#ident "$Header$"


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
 * \briefin given string (SQL) escape all unwanted characters
 */

char *glite_lbu_EscapeSQL(const char *);

#endif /* __EDG_WORKLOAD_LOGGING_COMMON_ESCAPE_H__ */
