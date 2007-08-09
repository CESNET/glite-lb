#ifndef GLITE_LB_ESCAPE_H
#define GLITE_LB_ESCAPE_H
/*!
 * \file Client/escape.h
 * \brief Prototypes for Client/escape.c
 */

#ident "$Header$"


/*!
 * \fn char *edg_wll_LogEscape(const char *str)
 * \param str           a string to escape
 * \return              new (allocated) escaped string
 * \brief in given string (ULM) escape all ULM_QM, ULM_BS and ULM_LF by ULM_BS
 */

char *edg_wll_LogEscape(const char *);


/*!
 * \fn char *edg_wll_LogUnescape(const char *str)
 * \param str           a string to unescape
 * \return              new (allocated) unescaped string
 * \brief in given string (ULM) unescape all escaped ULM_QM, ULM_BS and ULM_LF
 */

char *edg_wll_LogUnescape(const char *);


/*!
 * \fn char *edg_wll_EscapeXML(const char *str);
 * \param str		a string to escape
 * \return		new (allocated) escaped string
 * \brief in given string (XML) escape all unwanted characters
 */

char *edg_wll_EscapeXML(const char *);


/*!
 * \fn char *edg_wll_UnescapeXML(const char *str)
 * \param str		a string to unescape
 * \return		new (allocated) unescaped string
 * \brief in given string (XML) unescape all escaped characters
 */

char *edg_wll_UnescapeXML(const char *);


/*!
 * \fn char *edg_wll_EscapeSQL(const char *str)
 * \param str		a string to escape
 * \return		new (allocated) escaped string
 * \briefin given string (SQL) escape all unwanted characters
 */

char *edg_wll_EscapeSQL(const char *);

#endif /* GLITE_LB_ESCAPE_H */
