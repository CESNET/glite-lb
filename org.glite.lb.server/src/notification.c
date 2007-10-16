#ident "$Header$"

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "glite/jobid/strmd5.h"
#include "glite/lbu/trio.h"
#include "glite/lb/context-int.h"
#include "glite/lb/xml_parse.h"

#include "il_notification.h"
#include "query.h"
#include "db_supp.h"


static char *get_user(edg_wll_Context ctx, int create);
static int check_notif_request(edg_wll_Context, const edg_wll_NotifId, char **);
static int split_cond_list(edg_wll_Context, edg_wll_QueryRec const * const *,
						edg_wll_QueryRec ***, char ***);
static int update_notif(edg_wll_Context, const edg_wll_NotifId,
						const char *, const char *, const char *);


int edg_wll_NotifNewServer(
	edg_wll_Context					ctx,
	edg_wll_QueryRec const * const *conditions,
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
					  **jobs		= NULL;
	edg_wll_QueryRec  **nconds		= NULL;


	/*	Format notification ID
	 */
	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) )
		goto cleanup;

	/*	Get notification owner
	 */
	if ( !(owner = get_user(ctx, 1)) )
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
	*valid = time(NULL);
	if (   ctx->peerProxyValidity
		&& (ctx->peerProxyValidity - *valid) < ctx->notifDuration )
		*valid = ctx->peerProxyValidity;
	else
		*valid += ctx->notifDuration;

	glite_lbu_TimeToDB(*valid, &time_s);
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

		if ( !(aux = strchr(address_override, ':')) )
		{
			edg_wll_SetError(ctx, EINVAL, "Addres overrirde not in format host:port");
			goto cleanup;
		}
		if ( !strncmp(address_override, "0.0.0.0", aux-address_override) )
			trio_asprintf(&addr_s, "%s:%s", ctx->connections->serverConnection->peerName, aux+1);
	}

	/*	Format DB insert statement
	 */
	trio_asprintf(&q,
				"insert into notif_registrations(notifid,destination,valid,userid,conditions) "
				"values ('%|Ss','%|Ss',%s,'%|Ss', '<and>%|Ss</and>')",
				nid_s, addr_s? addr_s: address_override, time_s, owner, xml_conds);

	if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 )
		goto cleanup;

	if (jobs) for ( i = 0; jobs[i]; i++ )
	{
		free(q);
		trio_asprintf(&q,
				"insert into notif_jobs(notifid,jobid) values ('%|Ss','%|Ss')",
				nid_s, jobs[i]);
		if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 )
		{
			/*	XXX: Remove uncoplete registration?
			 *		 Which error has to be returned?
			 */
			free(q);
			trio_asprintf(&q, "delete from notif_jobs where notifid='%|Ss'", nid_s);
			edg_wll_ExecSQL(ctx, q, NULL);
			free(q);
			trio_asprintf(&q, "delete from notif_registrations where notifid='%|Ss'", nid_s);
			edg_wll_ExecSQL(ctx, q, NULL);
			goto cleanup;
		}
	}
	else {
		trio_asprintf(&q,"insert into notif_jobs(notifid,jobid) values ('%|Ss','%|Ss')",
				nid_s,NOTIF_ALL_JOBS);
		if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 ) goto cleanup;

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

	return edg_wll_Error(ctx, NULL, NULL);
}


int edg_wll_NotifBindServer(
	edg_wll_Context					ctx,
	const edg_wll_NotifId			nid,
	const char					   *address_override,
	time_t						   *valid)
{
	char	   *time_s = NULL,
			   *addr_s = NULL;


	if ( !address_override )
	{
		edg_wll_SetError(ctx, EINVAL, "Address parameter not given");
		goto cleanup;
	}

	if ( check_notif_request(ctx, nid, NULL) )
		goto cleanup;

	/*	Format time of validity
	 */
	*valid = time(NULL);
	if (   ctx->peerProxyValidity
		&& (ctx->peerProxyValidity - *valid) < ctx->notifDuration )
		*valid = ctx->peerProxyValidity;
	else
		*valid += ctx->notifDuration;

	glite_lbu_TimeToDB(*valid, &time_s);
	if ( !time_s )
	{
		edg_wll_SetError(ctx, errno, "Formating validity time");
		goto cleanup;
	}

	/*	Format the address
	 */
	if ( address_override )
	{
		char   *aux;

		if ( !(aux = strchr(address_override, ':')) )
		{
			edg_wll_SetError(ctx, EINVAL, "Addres overrirde not in format host:port");
			goto cleanup;
		}
		if ( !strncmp(address_override, "0.0.0.0", aux-address_override) )
			trio_asprintf(&addr_s, "%s:%s", ctx->connections->serverConnection->peerName, aux+1);
	}


	update_notif(ctx, nid, NULL, addr_s? addr_s: address_override, (const char *)(time_s));

cleanup:
	if ( time_s ) free(time_s);
	if ( addr_s ) free(addr_s);

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
	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) )
		goto cleanup;

	if ( check_notif_request(ctx, nid, NULL) )
		goto cleanup;

	switch ( op )
	{
	case EDG_WLL_NOTIF_REPLACE:
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

		/*	Format DB insert statement
		 */
		if ( update_notif(ctx, nid, xml_conds, NULL, NULL) )
			goto cleanup;

		if ( jobs )
		{
			/*	Format DB insert statement
			 */
			trio_asprintf(&q, "delete from  notif_jobs where notifid='%|Ss'", nid_s);
			if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 )
				goto cleanup;

			for ( i = 0; jobs[i]; i++ )
			{
				free(q);
				trio_asprintf(&q,
						"insert into notif_jobs(notifid,jobid) values ('%|Ss','%|Ss')",
						nid_s, jobs[i]);
				if ( edg_wll_ExecSQL(ctx, q, NULL) < 0 )
				{
					/*	XXX: Remove uncoplete registration?
					 *		 Which error has to be returned?
					 */
					free(q);
					trio_asprintf(&q, "delete from notif_jobs where notifid='%|Ss'", nid_s);
					edg_wll_ExecSQL(ctx, q, NULL);
					free(q);
					trio_asprintf(&q,"delete from notif_registrations where notifid='%|Ss'", nid_s);
					edg_wll_ExecSQL(ctx, q, NULL);
					goto cleanup;
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

cleanup:
	if ( q ) free(q);
	if ( xml_conds ) free(xml_conds);
	if ( nid_s ) free(nid_s);
	if ( jobs )
	{
		for ( i = 0; jobs[i]; i++ )
			free(jobs[i]);
		free(jobs);
	}
	if ( nconds ) free(nconds);

	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_NotifRefreshServer(
	edg_wll_Context					ctx,
	const edg_wll_NotifId			nid,
	time_t						   *valid)
{
	char	   *time_s = NULL;


	if ( check_notif_request(ctx, nid, NULL) )
		goto cleanup;

	/*	Format time of validity
	 */
	*valid = time(NULL);
	if (   ctx->peerProxyValidity
		&& (ctx->peerProxyValidity - *valid) < ctx->notifDuration )
		*valid = ctx->peerProxyValidity;
	else
		*valid += ctx->notifDuration;

	glite_lbu_TimeToDB(*valid, &time_s);
	if ( !time_s )
	{
		edg_wll_SetError(ctx, errno, "Formating validity time");
		goto cleanup;
	}

	update_notif(ctx, nid, NULL, NULL, time_s);

cleanup:
	if ( time_s ) free(time_s);

	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_NotifDropServer(
	edg_wll_Context					ctx,
	edg_wll_NotifId				   *nid)
{
	char	   *nid_s = NULL,
			   *stmt = NULL;
	int			ret;


	if ( check_notif_request(ctx, nid, NULL) )
		goto cleanup;

	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) )
		goto cleanup;

	trio_asprintf(&stmt, "delete from notif_registrations where notifid='%|Ss'", nid_s);
	if ( (ret = edg_wll_ExecSQL(ctx, stmt, NULL)) < 0 )
		goto cleanup;
	free(stmt);
	trio_asprintf(&stmt, "delete from notif_jobs where notifid='%|Ss'", nid_s);
	edg_wll_ExecSQL(ctx, stmt, NULL);
	edg_wll_NotifCancelRegId(ctx, nid);

cleanup:
	if ( nid_s ) free(nid_s);
	if ( stmt ) free(stmt);

	return edg_wll_Error(ctx, NULL, NULL);
}


static char *get_user(edg_wll_Context ctx, int create)
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
	trio_asprintf(&q, "select userid from users where cert_subj='%|Ss'", can_peername);
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
	char					  **owner)
{
	char	   *nid_s = NULL,
			   *stmt, *user;
	int			ret;


	/* XXX: rewrite select below in order to handle cert_subj format changes */
	if ( !(user = get_user(ctx, 0)) )
	{
		if ( !edg_wll_Error(ctx, NULL, NULL) )
			edg_wll_SetError(ctx, EPERM, "Unknown user");

		return edg_wll_Error(ctx, NULL, NULL);
	}

	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) )
		goto cleanup;

	trio_asprintf(&stmt,
				"select notifid from notif_registrations "
				"where notifid='%|Ss' and userid='%|Ss'",
				nid_s, user);

	if ( (ret = edg_wll_ExecSQL(ctx, stmt, NULL)) < 0 )
		goto cleanup;
	if ( ret == 0 )
	{
		free(stmt);
		trio_asprintf(&stmt,
					"select notifid from notif_registrations where notifid='%|Ss'", nid_s);
		ret = edg_wll_ExecSQL(ctx, stmt, NULL);
		if ( ret == 0 )
			edg_wll_SetError(ctx, ENOENT, "Unknown notification ID");
		else if ( ret > 0 )
			edg_wll_SetError(ctx, EPERM, "Only owner could access the notification");
	}

cleanup:
	if ( !edg_wll_Error(ctx, NULL, NULL) && owner )
		*owner = user;
	else
		free(user);
	if ( nid_s ) free(nid_s);
	if ( stmt ) free(stmt);

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
	char	   *nid_s = NULL,
			   *host = NULL,
			   *stmt, *aux;
	int			ret, port;


	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) )
		goto cleanup;

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
		host = strchr(dest, ':');
		port = atoi(host+1);
		if ( !(host = strndup(dest, host-dest)) )
		{
			edg_wll_SetError(ctx, errno, "updating notification records");
			goto cleanup;
		}
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

	if ( (ret = edg_wll_ExecSQL(ctx, stmt, NULL)) < 0 )
		goto cleanup;
	if ( ret == 0 )
	{
		free(stmt);
		trio_asprintf(&stmt,
				"select notifid from notif_registrations where notifid='%|Ss'", nid_s);
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

	if ( host ) {
		printf("edg_wll_NotifChangeDestination(ctx, %s, %s, %d)\n",
				nid_s? nid_s: "nid", host, port);
		if ( edg_wll_NotifChangeDestination(ctx, nid, host, port) ) {
			char *errt, *errd;

			edg_wll_Error(ctx, &errt, &errd);
			printf("edg_wll_NotifChangeDestination(): %s (%s)\n", errt, errd);
			free(errt);
			free(errd);
		}
	}


cleanup:
	if ( nid_s ) free(nid_s);
	if ( stmt ) free(stmt);
	if ( host ) free(host);

	return edg_wll_Error(ctx, NULL, NULL);
}
