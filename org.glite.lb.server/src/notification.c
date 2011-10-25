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


#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "glite/jobid/strmd5.h"
#include "glite/lbu/trio.h"
#include "glite/lb/context-int.h"
#include "glite/lb/xml_parse.h"
#include "glite/lbu/log.h"

#include "il_notification.h"
#include "query.h"
#include "db_supp.h"
#include "index.h"
#include "lb_xml_parse.h"
#include "get_events.h"


typedef struct {
	edg_wll_NotifId *nid;
	char *nid_s;
	char *xml_conds;
	glite_lbu_Statement stmt;
	int count;
} notif_stream_t;


static char *get_user(edg_wll_Context ctx, int create, char **subj);
static int check_notif_request(edg_wll_Context, const edg_wll_NotifId, char **, char **);
static int split_cond_list(edg_wll_Context, edg_wll_QueryRec const * const *,
						edg_wll_QueryRec ***, char ***);
static int update_notif(edg_wll_Context, const edg_wll_NotifId,
						const char *, const char *, const char *);

static int get_indexed_cols(edg_wll_Context,char const *,edg_wll_QueryRec **,char **);
static void adjust_validity(edg_wll_Context,time_t *);
static int drop_notif_request(edg_wll_Context, const edg_wll_NotifId);
static int check_notif_age(edg_wll_Context, const edg_wll_NotifId);
static int notif_streaming(edg_wll_Context);


int edg_wll_NotifNewServer(
	edg_wll_Context					ctx,
	edg_wll_QueryRec const * const *conditions,
	int	flags,
	char const					   *address_override,
	const edg_wll_NotifId 			nid,
	time_t						   *valid)
{
	int					i;
	char			   *q			= NULL,
					   *nid_s		= NULL,
					   *time_s		= NULL,
					   *addr_s		= NULL,
					   *xml_conds	= NULL,
					   *owner		= NULL,
					   *subj		= NULL,
					  **jobs		= NULL;
	edg_wll_QueryRec  **nconds		= NULL;
	char		*add_index = NULL;
	notif_stream_t	*arg = NULL;
	int npref, okpref;
	char   *msgpref;


	/*	Format notification ID
	 */
	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) )
		goto cleanup;

	/*	Get notification owner
	 */
	if ( !(owner = get_user(ctx, 1, &subj)) )
		goto cleanup;

	/*	Format conditions
	 *	- separate all jobids
	 *	- format new condition list without jobids
	 */
	if ( split_cond_list(ctx, conditions, &nconds, &jobs) )
		goto cleanup;


	/*
	 *	encode new cond. list into a XML string
	 */
	if ( edg_wll_JobQueryRecToXML(ctx, (edg_wll_QueryRec const * const *) nconds, &xml_conds) )
	{
		/*	XXX: edg_wll_JobQueryRecToXML() do not set errors in context!
		 *			can't get propper error number :(
		 */
		edg_wll_SetError(ctx, errno, "Can't encode data into xml");
		goto cleanup;
	}

	/*	Format time of validity
	 */

	if (*valid == 0) 
		*valid = time(NULL) + ctx->notifDuration;	
	adjust_validity(ctx,valid);

	glite_lbu_TimeToStr(*valid, &time_s);
	if ( !time_s )
	{
		edg_wll_SetError(ctx, errno, NULL);
		goto cleanup;
	}

	/*	Format the address
	 */
	if ( address_override )
	{
		char   *aux;

		if ( !(aux = strrchr(address_override, ':')) )
		{
			edg_wll_SetError(ctx, EINVAL, "Addres override not in format host:port");
			goto cleanup;
		}
		if ( !strncmp(address_override, "x-msg", 5)) {
			npref = 0; okpref = 0;
			while (ctx->msg_prefixes[npref]) {
				asprintf(&msgpref, "x-msg://%s", ctx->msg_prefixes[npref++]);
				if ( !strncmp(address_override, msgpref, strlen(msgpref))) {
					okpref = 1;
					free(msgpref);
					break;
				}
				free(msgpref);
			}				
			if (!okpref) {
				char *prefmsg, *prefmsg2;
				asprintf(&prefmsg,"This site requires that all topic names start with prefix %s", ctx->msg_prefixes[0]);
				npref = 1;
				while (ctx->msg_prefixes[npref]) {
					prefmsg2 = prefmsg;
					asprintf(&prefmsg,"%s or %s", prefmsg2, ctx->msg_prefixes[npref++]);
					free(prefmsg2);
				}
	
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_DEBUG, "Rejecting notification with disallowed prefix (%s)", address_override);
				edg_wll_SetError(ctx, EINVAL, prefmsg);
				free(prefmsg);
				goto cleanup;
			}
		}

		if ( !strncmp(address_override, "0.0.0.0", aux-address_override) || 
		     !strncmp(address_override, "[::]", aux-address_override) ||
		     !strncmp(address_override, "::", aux-address_override) )
			trio_asprintf(&addr_s, "%s:%s", ctx->connections->serverConnection->peerName, aux+1);
	}

	do {
		if (edg_wll_Transaction(ctx) != 0) goto cleanup;

		/*	Format DB insert statement
		 */
		trio_asprintf(&q,
					"insert into notif_registrations(notifid,destination,valid,userid,conditions,flags) "
					"values ('%|Ss','%|Ss',%s,'%|Ss', '<and>%|Ss</and>', '%d')",
					nid_s, addr_s? addr_s: address_override, time_s, owner, xml_conds, flags);

		glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG,
			q);
		if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 )
			goto rollback;

		if (get_indexed_cols(ctx,nid_s,nconds,&add_index) ||
			(add_index && edg_wll_ExecSQL(ctx,add_index,NULL) < 0)
		) goto rollback;


		if (jobs) for ( i = 0; jobs[i]; i++ )
		{
			free(q);
			trio_asprintf(&q,
					"insert into notif_jobs(notifid,jobid) values ('%|Ss','%|Ss')",
					nid_s, jobs[i]);
			glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, 
				LOG_PRIORITY_DEBUG, q);
			if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 )
				goto rollback;
		}
		else {
			free(q);
			trio_asprintf(&q,"insert into notif_jobs(notifid,jobid) values ('%|Ss','%|Ss')",
					nid_s,NOTIF_ALL_JOBS);
			glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB,
				LOG_PRIORITY_DEBUG, q);
			if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 ) goto rollback;

		}

rollback:
		free(q); q= NULL;
		free(add_index); add_index = NULL;

	} while (edg_wll_TransNeedRetry(ctx));

	// set-up the streaming
	if ((flags & EDG_WLL_NOTIF_BOOTSTRAP)) {
		char *tmp;

		arg = calloc(1, sizeof(*arg));

		if ( edg_wll_JobQueryRecToXML(ctx, conditions, &tmp) )
		{
			edg_wll_SetError(ctx, errno, "Can't encode data into xml");
			goto cleanup;
		}
		asprintf(&arg->xml_conds, "<and>%s</and>", tmp);
		free(tmp);
		arg->nid = edg_wll_NotifIdDup(nid);
		arg->nid_s = strdup(nid_s);

		ctx->processRequest_cb = notif_streaming;
		ctx->processRequestUserData = arg;

		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[%d] streaming set-up, notif '%s', initial destination '%s'", getpid(), arg->nid_s, addr_s ? : address_override);
		arg = NULL;
	}


cleanup:
	if ( q ) free(q);
	if ( nid_s ) free(nid_s);
	if ( time_s ) free(time_s);
	if ( addr_s ) free(addr_s);
	if ( xml_conds ) free(xml_conds);
	if ( owner ) free(owner);
	if ( jobs )
	{
		for ( i = 0; jobs[i]; i++ )
			free(jobs[i]);
		free(jobs);
	}
	if ( nconds ) free(nconds);
	free(add_index);
	free(arg);
	free(subj);

	return edg_wll_Error(ctx, NULL, NULL);
}


int edg_wll_NotifBindServer(
	edg_wll_Context					ctx,
	const edg_wll_NotifId			nid,
	const char					   *address_override,
	time_t						   *valid)
{
	char	*time_s = NULL,
		*addr_s = NULL;
	int npref, okpref;
	char   *msgpref;


	if ( !address_override )
	{
		edg_wll_SetError(ctx, EINVAL, "Address parameter not given");
		goto err;
	}
	
	do {
		if (edg_wll_Transaction(ctx) != 0) goto err;

		if ( check_notif_request(ctx, nid, NULL, NULL) )
			goto rollback;

		if ( check_notif_age(ctx, nid) ) {
			drop_notif_request(ctx, nid);
			goto rollback;
		}

		/*	Format time of validity
		 */

		if (*valid == 0) 
			*valid = time(NULL) + ctx->notifDuration;	
		adjust_validity(ctx,valid);

		glite_lbu_TimeToStr(*valid, &time_s);
		if ( !time_s )
		{
			edg_wll_SetError(ctx, errno, "Formating validity time");
			goto rollback;
		}

		/*	Format the address
		 */
		if ( address_override )
		{
			char   *aux;

			if ( !(aux = strrchr(address_override, ':')) )
			{
				edg_wll_SetError(ctx, EINVAL, "Addres override not in format host:port");
				goto rollback;
			}
			if ( !strncmp(address_override, "x-msg", 5)) {

				npref = 0; okpref = 0;
				while (ctx->msg_prefixes[npref]) {
					asprintf(&msgpref, "x-msg://%s", ctx->msg_prefixes[npref++]);
					if ( !strncmp(address_override, msgpref, strlen(msgpref))) {
						okpref = 1;
						free(msgpref);
						break;
					}
					free(msgpref);
				}				
				if (!okpref) {
					char *prefmsg, *prefmsg2;
					asprintf(&prefmsg,"This site requires that all topic names start with prefix %s", ctx->msg_prefixes[0]);
					npref = 1;
					while (ctx->msg_prefixes[npref]) {
						prefmsg2 = prefmsg;
						asprintf(&prefmsg,"%s or %s", prefmsg2, ctx->msg_prefixes[npref++]);
						free(prefmsg2);
					}

					glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_DEBUG, "Rejecting notification with disallowed prefix (%s)", address_override);
					edg_wll_SetError(ctx, EINVAL, prefmsg);
					free(prefmsg);
					goto rollback;
				}
			}
			if ( !strncmp(address_override, "0.0.0.0", aux-address_override) ||
			     !strncmp(address_override, "[::]", aux-address_override) ||
			     !strncmp(address_override, "::", aux-address_override) )
				trio_asprintf(&addr_s, "%s:%s", ctx->connections->serverConnection->peerName, aux+1);
		}


		update_notif(ctx, nid, NULL, addr_s? addr_s: address_override, (const char *)(time_s));

rollback:
	free(time_s); time_s = NULL;
	free(addr_s); addr_s = NULL;

	} while (edg_wll_TransNeedRetry(ctx));

err:
	return edg_wll_Error(ctx, NULL, NULL);
}


int edg_wll_NotifChangeServer(
	edg_wll_Context					ctx,
	const edg_wll_NotifId			nid,
	edg_wll_QueryRec const * const *conditions,
	edg_wll_NotifChangeOp			op)
{
	int					i;
	char			   *q			= NULL,
					   *nid_s		= NULL,
					   *xml_conds	= NULL,
					  **jobs		= NULL;
	edg_wll_QueryRec  **nconds		= NULL;


	/*	Format notification ID
	 */
	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) ) {
		edg_wll_SetError(ctx, EINVAL, "Malformed notification ID");
		goto err;
	}

	do {
		if (edg_wll_Transaction(ctx) != 0) goto err;

		if ( check_notif_request(ctx, nid, NULL, NULL) )
			goto rollback;

		if ( check_notif_age(ctx, nid) ) {
			drop_notif_request(ctx, nid);
			goto rollback;
		}

		switch ( op )
		{
		case EDG_WLL_NOTIF_REPLACE:
			/*	Format conditions
			 *	- separate all jobids
			 *	- format new condition list without jobids
			 */
			if ( split_cond_list(ctx, conditions, &nconds, &jobs) )
				goto rollback;

			/*
			 *	encode new cond. list into a XML string
			 */
			if ( edg_wll_JobQueryRecToXML(ctx, (edg_wll_QueryRec const * const *) nconds, &xml_conds) )
			{
				/*	XXX: edg_wll_JobQueryRecToXML() do not set errors in context!
				 *			can't get propper error number :(
				 */
				edg_wll_SetError(ctx, errno, "Can't encode data into xml");
				goto rollback;
			}

			/*	Format DB insert statement
			 */
			if ( update_notif(ctx, nid, xml_conds, NULL, NULL) )
				goto rollback;

			if ( jobs )
			{
				/*	Format DB insert statement
				 */
				trio_asprintf(&q, "delete from  notif_jobs where notifid='%|Ss'", nid_s);
				glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB,
					LOG_PRIORITY_DEBUG, q);
				if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 )
					goto rollback;

				for ( i = 0; jobs[i]; i++ )
				{
					free(q);
					trio_asprintf(&q,
							"insert into notif_jobs(notifid,jobid) values ('%|Ss','%|Ss')",
							nid_s, jobs[i]);
					glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);
					if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 )
					{
						/*	XXX: Remove uncoplete registration?
						 *		 Which error has to be returned?
						 */
						free(q);
						trio_asprintf(&q, "delete from notif_jobs where notifid='%|Ss'", nid_s);
						glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);
						edg_wll_ExecSQL(ctx, q, NULL);
						free(q);
						trio_asprintf(&q,"delete from notif_registrations where notifid='%|Ss'", nid_s);
						glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);
						edg_wll_ExecSQL(ctx, q, NULL);
						goto rollback;
					}
				}
			}
			break;

		case EDG_WLL_NOTIF_ADD:
			break;
		case EDG_WLL_NOTIF_REMOVE:
			break;
		default:
			break;
		}

rollback:
		free(q); q = NULL;
		free(xml_conds); xml_conds = NULL;
		free(nid_s); nid_s = NULL;
		if ( jobs ) {
			for ( i = 0; jobs[i]; i++ )
				free(jobs[i]);
			free(jobs); jobs = NULL;
		}
		free(nconds); nconds = NULL;

	} while (edg_wll_TransNeedRetry(ctx));

err:
	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_NotifRefreshServer(
	edg_wll_Context					ctx,
	const edg_wll_NotifId			nid,
	time_t						   *valid)
{
	char	   *time_s = NULL, *dest = NULL;

	do {
		if (edg_wll_Transaction(ctx) != 0) goto err;		

		if ( check_notif_request(ctx, nid, NULL, &dest) )
			goto rollback;

		if ( check_notif_age(ctx, nid) ) {
			drop_notif_request(ctx, nid);
			goto rollback;
		}

		/*	Format time of validity
		 */

		if (*valid == 0) 
			*valid = time(NULL) + ctx->notifDuration;	
		adjust_validity(ctx,valid);

		glite_lbu_TimeToStr(*valid, &time_s);
		if ( !time_s )
		{
			edg_wll_SetError(ctx, errno, "Formating validity time");
			goto rollback;
		}

		update_notif(ctx, nid, NULL, dest, time_s);

rollback:
		free(time_s); time_s = NULL;
		free(dest); dest = NULL;

	} while (edg_wll_TransNeedRetry(ctx));

err:
	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_NotifDropServer(
	edg_wll_Context					ctx,
	edg_wll_NotifId				   nid)
{
	do {
		if (edg_wll_Transaction(ctx) != 0) goto err;

		if ( check_notif_request(ctx, nid, NULL, NULL) )
			goto rollback;

		if ( drop_notif_request(ctx, nid) )
			goto rollback;

rollback:
		;
	} while (edg_wll_TransNeedRetry(ctx));

err:
	return edg_wll_Error(ctx, NULL, NULL);
}


static char *get_user(edg_wll_Context ctx, int create, char **subj)
{
	glite_lbu_Statement	stmt	= NULL;
	char		   *userid	= NULL,
				   *q		= NULL;
	int				ret;
	char	*can_peername = NULL;

	if ( !ctx->peerName )
	{
		edg_wll_SetError(ctx, EPERM, "Annonymous access not allowed");
		goto cleanup;
	}
	can_peername = edg_wll_gss_normalize_subj(ctx->peerName, 0);
	if (subj) *subj = strdup(can_peername);
	trio_asprintf(&q, "select userid from users where cert_subj='%|Ss'", can_peername);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);
	if ( edg_wll_ExecSQL(ctx, q, &stmt) < 0 )
		goto cleanup;

	/*	returned value:
	 *	0		no user find - continue only when 'create' parameter is set
	 *	>0		user found - return selected value
	 *	<0		SQL error
	 */
	if ( ((ret = edg_wll_FetchRow(ctx, stmt, 1, NULL, &userid)) != 0) || !create )
		goto cleanup;

	if ( !(userid = strdup(strmd5(ctx->peerName, NULL))) )
	{
		edg_wll_SetError(ctx, errno, "Creating user ID");
		goto cleanup;
	}
	free(q);
	trio_asprintf(&q, "insert into users(userid,cert_subj) values ('%|Ss','%|Ss')",
			userid, can_peername);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);
	if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 )
	{
		if ( edg_wll_Error(ctx,NULL,NULL) != EEXIST )
		{
			free(userid);
			userid = NULL;
		}
		else
			edg_wll_ResetError(ctx);
	}

cleanup:
	if ( q ) free(q);
	if ( stmt ) glite_lbu_FreeStmt(&stmt);
	free(can_peername);

	return userid;
}


static int check_notif_request(
	edg_wll_Context				ctx,
	const edg_wll_NotifId		nid,
	char					  **owner,
	char	**destination)
{
	char	   *nid_s = NULL,
			   *stmt = NULL, *user, *dest = NULL;
	int			ret;
	glite_lbu_Statement	s = NULL;


	/* XXX: rewrite select below in order to handle cert_subj format changes */
	if ( !(user = get_user(ctx, 0, NULL)) )
	{
		if ( !edg_wll_Error(ctx, NULL, NULL) )
			edg_wll_SetError(ctx, EPERM, "Unknown user");

		return edg_wll_Error(ctx, NULL, NULL);
	}

	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) ) {
		edg_wll_SetError(ctx, EINVAL, "Malformed notification ID");
		goto cleanup;
	}

	trio_asprintf(&stmt,
				"select destination from notif_registrations "
				"where notifid='%|Ss' and userid='%|Ss' FOR UPDATE",
				nid_s, user);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if ( (ret = edg_wll_ExecSQL(ctx, stmt, &s)) < 0 )
		goto cleanup;
	if ( ret == 0 )
	{
		free(stmt);
		trio_asprintf(&stmt,
					"select notifid from notif_registrations where notifid='%|Ss'", nid_s);
		glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, 
			stmt);
		ret = edg_wll_ExecSQL(ctx, stmt, NULL);
		if ( ret == 0 )
			edg_wll_SetError(ctx, ENOENT, "Unknown notification ID");
		else if ( ret > 0 )
			edg_wll_SetError(ctx, EPERM, "Only owner could access the notification");
		goto cleanup;
	}
	else edg_wll_FetchRow(ctx,s,1,NULL,&dest);

cleanup:
	if ( !edg_wll_Error(ctx, NULL, NULL)) {
		if (owner) *owner = user; else free(user);
		if (destination) *destination = dest; else free(dest);
	}

	if ( nid_s ) free(nid_s);
	if ( stmt ) free(stmt);
 	glite_lbu_FreeStmt(&s);

	return edg_wll_Error(ctx, NULL, NULL);
}


	/*	Format conditions
	 *	- first of all separate all jobids
	 *	- then format new condition list without jobids and encode it into an XML string
	 */
static int split_cond_list(
	edg_wll_Context						ctx,
	edg_wll_QueryRec const * const	   *conditions,
	edg_wll_QueryRec				 ***nconds_out,
	char							 ***jobs_out)
{
	edg_wll_QueryRec  **nconds = NULL;
	char			  **jobs = NULL;
	int					i, j, jobs_ct, nconds_ct;


	if ( !conditions || !conditions[0] ) {
		if (ctx->noAuth) nconds_ct = jobs_ct = 0;
		else return edg_wll_SetError(ctx, EINVAL, "Empty condition list");
	}
	else for ( nconds_ct = jobs_ct = i = 0; conditions[i]; i++ ) {
		if ( conditions[i][0].attr && conditions[i][0].attr != EDG_WLL_QUERY_ATTR_JOBID )
			nconds_ct++;
		for ( j = 0; conditions[i][j].attr; j++ )
			if ( conditions[i][j].attr == EDG_WLL_QUERY_ATTR_JOBID )
				jobs_ct++;
	}

	if ( jobs_out && jobs_ct )
		if ( !(jobs = calloc(jobs_ct+1, sizeof(char *))) )
		{
			edg_wll_SetError(ctx, errno, NULL);
			goto cleanup;
		}

	if ( nconds_out && nconds_ct )
		if ( !(nconds = calloc(nconds_ct+1, sizeof(edg_wll_QueryRec *))) )
		{
			edg_wll_SetError(ctx, errno, NULL);
			goto cleanup;
		}

	if ( jobs ) for ( jobs_ct = i = 0; conditions[i]; i++ )
		for ( j = 0; conditions[i][j].attr; j++ )
			if ( conditions[i][j].attr == EDG_WLL_QUERY_ATTR_JOBID )
				if ( !(jobs[jobs_ct++] = edg_wlc_JobIdGetUnique(conditions[i][j].value.j)) )
				{
					edg_wll_SetError(ctx, errno, NULL);
					goto cleanup;
				}

	if ( nconds ) for ( nconds_ct = i = 0; conditions[i]; i++ )
		if ( conditions[i][0].attr && conditions[i][0].attr != EDG_WLL_QUERY_ATTR_JOBID )
			/*	!!! DO NOT DEALLOCATE this arrays (it is not neccessary to allocate new
			 *	mem - it's used only once and only for xml parsing
			 */
			nconds[nconds_ct++] = (edg_wll_QueryRec  *) (conditions[i]);

	if ( jobs_out ) { *jobs_out = jobs; jobs = NULL; }
	if ( nconds_out ) { *nconds_out = nconds; nconds = NULL; }

cleanup:
	if ( nconds ) free(nconds);
	if ( jobs )
	{
		for ( i = 0; jobs[i]; i++ )
			free(jobs[i]);
		free(jobs);
	}

	return edg_wll_Error(ctx, NULL, NULL);
}


static int update_notif(
	edg_wll_Context					ctx,
	const edg_wll_NotifId			nid,
	const char					   *conds,
	const char					   *dest,
	const char					   *valid)
{
	char	*nid_s = NULL, *host = NULL,
		*stmt = NULL, *aux = NULL;
	int	ret = 0;


	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) )
	{
		edg_wll_SetError(ctx, EINVAL, "Malformed notification ID");
		goto cleanup;
	}

	/*	Format SQL update string
	 *	(Only the owner could update the notification registration)
	 */
	if ( !(stmt = strdup("update notif_registrations set")) )
	{
		edg_wll_SetError(ctx, errno, "updating notification records");
		goto cleanup;
	}
	if ( dest )
	{
		trio_asprintf(&aux, "%s destination='%|Ss'", stmt, dest);
		free(stmt);
		stmt = aux;
	}
	if ( valid )
	{
		trio_asprintf(&aux, "%s %svalid=%s", stmt, dest? ",": "", valid);
		free(stmt);
		stmt = aux;
	}
	if ( conds )
	{
		trio_asprintf(&aux, "%s %sconditions='<and>%|Ss</and>'",
						stmt, (dest||valid)? ",": "", conds);
		free(stmt);
		stmt = aux;
	}
	trio_asprintf(&aux, "%s where notifid='%|Ss'", stmt, nid_s);
	free(stmt);
	stmt = aux;
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if ( (ret = edg_wll_ExecSQL(ctx, stmt, NULL)) < 0 )
		goto cleanup;
	if ( ret == 0 )
	{
		free(stmt);
		trio_asprintf(&stmt,
				"select notifid from notif_registrations where notifid='%|Ss'", nid_s);
		glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
		ret = edg_wll_ExecSQL(ctx, stmt, NULL);
		if ( ret == 0 )
			edg_wll_SetError(ctx, ENOENT, "Unknown notification ID");
		/*
		 *	XXX: Be happy?
		 *	May be: Rows matched: 1  Changed: 0  Warnings: 0 :-)
		else if ( ret > 0 )
			edg_wll_SetError(ctx, EPERM, "Updating notification records");
		 */
	}

	if ( dest || valid) {
		char	*v = strdup(valid),*v2 = strchr(v+1,'\'');
		int	expires;
		
		*v2 = 0;
		expires = glite_lbu_StrToTime(v+1);
/*
 		printf("edg_wll_NotifChangeIL(ctx, %s, %s, %d)\n",
				nid_s? nid_s: "nid", host, port);
*/
		if ( edg_wll_NotifChangeIL(ctx, nid, dest, expires) ) {
			char *errt, *errd;

			edg_wll_Error(ctx, &errt, &errd);
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "edg_wll_NotifChangeIL(): %s (%s)", errt, errd);
			free(errt);
			free(errd);
		}
		free(v);
	}


cleanup:
	if ( nid_s ) free(nid_s);
	if ( stmt ) free(stmt);
	if ( host ) free(host);

	return edg_wll_Error(ctx, NULL, NULL);
}

static int get_indexed_cols(edg_wll_Context ctx,char const *notif,edg_wll_QueryRec **conds,char **update_out)
{
	int	i,j;
	edg_wll_IColumnRec	* notif_cols = ctx->notif_index_cols;
	char	*cols = NULL,*aux;

	for (i=0; conds && conds[i]; i++) {
		for (j=0; notif_cols[j].qrec.attr &&
			(notif_cols[j].qrec.attr != conds[i]->attr || 
				(notif_cols[j].qrec.attr == EDG_WLL_QUERY_ATTR_JDL_ATTR && 
					(!conds[i]->attr_id.tag || strcmp(notif_cols[j].qrec.attr_id.tag,conds[i]->attr_id.tag))
				)
			); j++);
		if (notif_cols[j].qrec.attr) {
			if (conds[i][1].attr && conds[i][0].op != EDG_WLL_QUERY_OP_EQUAL) {
				char	buf[1000];
				sprintf(buf,"%s: indexed, only one and only `equals' condition supported",
						notif_cols[j].colname);

				return edg_wll_SetError(ctx,EINVAL,buf);
			}
			trio_asprintf(&aux,"%s%c %s = '%|Ss'",
					cols ? cols : "",
					cols ? ',': ' ',
					notif_cols[j].colname,conds[i]->value.c
			);
			free(cols);
			cols = aux; 

		}
	}
	if (cols) trio_asprintf(&aux,"update notif_registrations set %s where notifid = '%s'",
					cols,notif);
	else aux = NULL;

	free(cols);
	*update_out = aux;
	return edg_wll_ResetError(ctx);
}

static void adjust_validity(edg_wll_Context ctx,time_t *valid)
{
	time_t	now;

	time(&now);

	if (*valid <= 0) {
		*valid = now + ctx->notifDuration;
	}

	if (ctx->peerProxyValidity && ctx->peerProxyValidity < *valid)
		*valid = ctx->peerProxyValidity;

	if (*valid - now > ctx->notifDurationMax) 
		*valid = now + ctx->notifDurationMax;
}

static int drop_notif_request(edg_wll_Context ctx, const edg_wll_NotifId nid) {
	char	   *nid_s = NULL,
		   *stmt = NULL,
		   *errDesc;
	int	    errCode;

	errCode = edg_wll_Error(ctx, NULL, &errDesc);	

	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) ) {
		edg_wll_SetError(ctx, EINVAL, "Malformed notification ID");
		goto rollback;
	}

	trio_asprintf(&stmt, "delete from notif_registrations where notifid='%|Ss'", nid_s);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	if ( edg_wll_ExecSQL(ctx, stmt, NULL) < 0 )
		goto rollback;
	free(stmt);
	trio_asprintf(&stmt, "delete from notif_jobs where notifid='%|Ss'", nid_s);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	if ( edg_wll_ExecSQL(ctx, stmt, NULL) < 0 ) 
		goto rollback;
	edg_wll_NotifCancelRegId(ctx, nid);
	if (edg_wll_Error(ctx, NULL, NULL) == ECONNREFUSED) {
		/* Let notification erase from DB, 
		 * on notif-IL side it will be autopurged later anyway */

		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
			"[%d] edg_wll_NotifDropServer() - NotifID found and " \
			"dropped, however, connection to notif-IL was " \
			" refused (notif-IL not running?)", getpid());

		edg_wll_ResetError(ctx);
	}

rollback:
	free(nid_s); nid_s = NULL;
	free(stmt); stmt = NULL;
	/* take precedence to previous error */
	if (errCode || errDesc) {
		edg_wll_SetError(ctx, errCode, errDesc);
		free(errDesc);
	}

	return edg_wll_Error(ctx, NULL, NULL);
}

static int check_notif_age(edg_wll_Context ctx, const edg_wll_NotifId nid) {
	time_t now = time(NULL);
	char *time_s = NULL,
	     *nid_s = NULL,
	     *q = NULL;
	int ret;

	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) ) {
		edg_wll_SetError(ctx, EINVAL, "Malformed notification ID");
		goto cleanup;
	}

	glite_lbu_TimeToStr(now, &time_s);
	if ( !time_s )
	{
		edg_wll_SetError(ctx, errno, NULL);
		goto cleanup;
	}

	trio_asprintf(&q, "select notifid from notif_registrations WHERE notifid='%|Ss' AND valid < %s", nid_s, time_s);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, q);
	if ( (ret = edg_wll_ExecSQL(ctx, q, NULL)) < 0 )
		goto cleanup;

	if (ret > 0)
		edg_wll_SetError(ctx, EINVAL, "Notification expired.");

cleanup:
	free(q);
	free(nid_s);
	free(time_s);
	return edg_wll_Error(ctx, NULL, NULL);
}

// refresh notification info from DB
static int notif_streaming_refresh(edg_wll_Context ctx, glite_lbu_Statement stmt, const char *nid_s, char **row, size_t n) {
	int ret;

	if (glite_lbu_ExecPreparedStmt(stmt, 1, GLITE_LBU_DB_TYPE_CHAR, nid_s) == -1
	 || (ret = glite_lbu_FetchRow(stmt, n, NULL, row)) == -1) {
		glite_common_log(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_FATAL, "[%d] NOTIFY stream: %s, refresh DB query failed", getpid(), nid_s);
		return edg_wll_SetErrorDB(ctx);
	}

	if (ret == 0) {
		return edg_wll_SetError(ctx, ENOENT, "notification not registered");
	}

	return 0;
}

static int notif_streaming_event_cb(edg_wll_Context ctx, glite_jobid_t jobid, edg_wll_JobStat *stat, void *arg) {
	char			   *dest;
	const char *owner;
	int flags;
	time_t expire;
	int authz_flags;
	size_t i;
	char *ju;
	char *row[4] = {};
	notif_stream_t *lctx = (notif_stream_t *)arg;

	if (!stat || !jobid) {
	// last job mark
		goto cleanup;
	}

	if (notif_streaming_refresh(ctx, lctx->stmt, lctx->nid_s, row, sizeof(row)/sizeof(row[0])) != 0) {
		if (edg_wll_Error(ctx, NULL, NULL) == ENOENT) {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "[%d] NOTIFY stream: %s, bootstrap interrupted (notification expired?)", getpid(), lctx->nid_s);
		}
		goto cleanup;
	}

	expire = glite_lbu_DBToTime(ctx->dbctx, row[1]);
	flags = atoi(row[2]);
	owner = row[3];

	if (!edg_wll_NotifCheckACL(ctx, (const edg_wll_JobStat *)stat, owner, &authz_flags)) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[%d] NOTIFY stream: %s, authorization failed, job %s", getpid(), lctx->nid_s, ju = glite_jobid_getUnique(stat->jobId));
		free(ju); ju = NULL;
		edg_wll_ResetError(ctx);
		goto cleanup;
	}

	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[%d] NOTIFY stream: %s, job %s, destination '%s'", getpid(), lctx->nid_s, ju = glite_jobid_getUnique(stat->jobId), row[0]);
	free(ju); ju = NULL;

	dest = strdup(row[0]);
	/*XXX: ??? copied from notif_match.c */ free(ctx->p_instance); ctx->p_instance = strdup("");
	if (edg_wll_NotifJobStatus(ctx, lctx->nid, dest, owner, flags, authz_flags, expire, (const edg_wll_JobStat)(*stat)) != 0) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "[%d] NOTIFY stream: %s, error", getpid(), lctx->nid_s);
		goto cleanup;
	}
	lctx->count++;

cleanup:
	for (i = 0; i < sizeof(row)/sizeof(row[0]); i++) free(row[i]);
	glite_jobid_free(jobid);
	edg_wll_FreeStatus(stat);
	return edg_wll_Error(ctx, NULL, NULL);
}

static int notif_streaming(edg_wll_Context ctx) {
	notif_stream_t *arg = (notif_stream_t *)ctx->processRequestUserData;
	edg_wlc_JobId *jobs = NULL;
	edg_wll_JobStat *states = NULL;
	size_t i;
	char *row[4];
	int flags;
	edg_wll_QueryRec **qs = NULL, *q = NULL;
	char *err = NULL, *desc = NULL;

	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[%d] NOTIFY stream: %s, START", getpid(), arg->nid_s);

	if (glite_lbu_PrepareStmt(ctx->dbctx, "SELECT destination, valid, flags, cert_subj FROM notif_registrations nr, users u WHERE nr.userid = u.userid AND notifid = ?", &arg->stmt) != 0) {
		edg_wll_SetErrorDB(ctx);
		goto quit;
	}
	if (notif_streaming_refresh(ctx, arg->stmt, arg->nid_s, row, sizeof(row)/sizeof(row[0])) != 0) goto quit;
	flags = atoi(row[2]);
	for (i = 0; i < sizeof(row)/sizeof(row[0]); i++) free(row[i]);

	if (parseJobQueryRec(ctx, arg->xml_conds, strlen(arg->xml_conds), &qs) != 0) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
			"[%d] NOTIFY stream: %s, parseJobQueryRec failed", getpid(), arg->nid_s);
		goto quit;
	}

	// perform the streaming
	if (edg_wll_QueryJobsServerStream(ctx, (edg_wll_QueryRec const **)qs, flags, notif_streaming_event_cb, arg) != 0)
		goto quit;

	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[%d] NOTIFY stream: %s, END, %d jobs sent", getpid(), arg->nid_s, arg->count);

quit:
	if (edg_wll_Error(ctx, &err, &desc))
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "[%d] NOTIFY stream: %s, %s: %s", getpid(), arg->nid_s, err, desc);
	free(err);
	free(desc);

	ctx->processRequest_cb = NULL;
	ctx->processRequestUserData = NULL;
	edg_wll_NotifIdFree(arg->nid);
	free(arg->nid_s);
	free(arg->xml_conds);
	glite_lbu_FreeStmt(&arg->stmt);
	free(arg);

	if (qs) {
		for (i = 0; qs[i]; i++) {
			for (q = qs[i]; q->attr; q++) {
				edg_wll_QueryRecFree(q);
			}
			free(qs[i]);
		}
		free(qs);
	}
	if (jobs) {
		for (i = 0; jobs[i]; i++) glite_jobid_free(jobs[i]);
		free(jobs);
	}
	if (states) {
		for (i = 0; states[i].jobId; i++) edg_wll_FreeStatus(states+i);
		free(states);
	}

	return edg_wll_Error(ctx, NULL, NULL);
}
