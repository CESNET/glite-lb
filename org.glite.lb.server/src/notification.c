#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "glite/wms/jobid/strmd5.h"
#include "glite/lb/trio.h"
#include "glite/lb/context-int.h"
#include "glite/lb/xml_parse.h"
#include "glite/lb/notification.h"
#include "lbs_db.h"


static char	   *get_user(edg_wll_Context ctx, int create);
static int		update_notif(edg_wll_Context, const edg_wll_NotifId,
							const char *, const char *, const char *);

int edg_wll_NotifNewServer(
	edg_wll_Context					ctx,
	edg_wll_QueryRec const * const *conditions,
	char const					   *address_override,
	const edg_wll_NotifId 			nid,
	time_t						   *valid)
{
	int					i, j,
						ct,
						new_rows;
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
	 *	- first of all separate all jobids
	 *	- then format new condition list without jobids and encode it into an XML string
	 */
	if ( !conditions || !conditions[0] )
	{
		edg_wll_SetError(ctx, EINVAL, "Empty condition list");
		goto cleanup;
	}
	for ( new_rows = ct = i = 0; conditions[i]; i++ )
	{
		if ( conditions[i][0].attr && conditions[i][0].attr != EDG_WLL_QUERY_ATTR_JOBID )
			new_rows++;
		for ( j = 0; conditions[i][j].attr; j++ )
			if ( conditions[i][j].attr == EDG_WLL_QUERY_ATTR_JOBID )
				ct++;
	}
	if ( !ct )
	{
		edg_wll_SetError(ctx, EINVAL, "Notification has to bind to at least one jobID");
		goto cleanup;
	}
	if (   !(jobs = calloc(ct+1, sizeof(char *)))
		|| !(nconds = calloc(new_rows+1, sizeof(edg_wll_QueryRec *))) )
	{
		edg_wll_SetError(ctx, errno, NULL);
		goto cleanup;
	}
	for ( ct = i = 0; conditions[i]; i++ )
		for ( j = 0; conditions[i][j].attr; j++ )
			if ( conditions[i][j].attr == EDG_WLL_QUERY_ATTR_JOBID )
				if ( !(jobs[ct++] = edg_wlc_JobIdGetUnique(conditions[i][j].value.j)) )
				{
					edg_wll_SetError(ctx, errno, NULL);
					goto cleanup;
				}
	for ( new_rows = i = 0; conditions[i]; i++ )
		if ( conditions[i][0].attr && conditions[i][0].attr != EDG_WLL_QUERY_ATTR_JOBID )
			/*	!!! DO NOT DEALLOCATE this arrays (it is not neccessary to allocate new
			 *	mem - it's used only once and only for xml parsing
			 */
			nconds[new_rows++] = (edg_wll_QueryRec  *) (conditions[i]);
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
	
	if ( !(time_s = strdup(edg_wll_TimeToDB(*valid))) )
	{
		edg_wll_SetError(ctx, errno, NULL);
		goto cleanup;
	}
	time_s[strlen(time_s)-1] = 0;

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
			trio_asprintf(&addr_s, "%s:%s", ctx->connPool[ctx->connToUse].peerName, aux+1);
	}

	/*	Format DB insert statement
	 */
	trio_asprintf(&q,
				"insert into notif_registrations(notifid,destination,valid,userid,conditions) "
				"values ('%|Ss','%|Ss','%|Ss','%|Ss', '<and>%|Ss</and>')",
				nid_s, addr_s? addr_s: address_override, time_s+1, owner, xml_conds);

	if ( edg_wll_ExecStmt(ctx, q, NULL) < 0 )
		goto cleanup;

	for ( i = 0; jobs[i]; i++ )
	{
		free(q);
		trio_asprintf(&q,
				"insert into notif_jobs(notifid,jobid) values ('%|Ss','%|Ss')",
				nid_s, jobs[i]);
		if ( edg_wll_ExecStmt(ctx, q, NULL) < 0 )
		{
			/*	XXX: Remove uncoplete registration?
			 *		 Which error has to be returned?
			 */
			free(q);
			trio_asprintf(&q, "delete from notif_jobs where notifid='%|Ss'", nid_s);
			edg_wll_ExecStmt(ctx, q, NULL);
			free(q);
			trio_asprintf(&q, "delete from notif_registrations where notifid='%|Ss'", nid_s);
			edg_wll_ExecStmt(ctx, q, NULL);
			goto cleanup;
		}
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
	char	   *time_s = NULL;


	if ( !address_override )
	{
		edg_wll_SetError(ctx, EINVAL, "Address parameter not given");
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

	if ( !(time_s = strdup(edg_wll_TimeToDB(*valid))) )
	{
		edg_wll_SetError(ctx, errno, "Formating validity time");
		goto cleanup;
	}
	time_s[strlen(time_s)-1] = 0;

	update_notif(ctx, nid, NULL, address_override, (const char *)(time_s+1));

cleanup:
	if ( time_s ) free(time_s);

	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_NotifChangeServer(
	edg_wll_Context					ctx,
	const edg_wll_NotifId			id,
	edg_wll_QueryRec const * const *conditions,
	edg_wll_NotifChangeOp			op)
{
	return edg_wll_SetError(ctx, EINVAL, "Not yet implemented");
}

int edg_wll_NotifRefreshServer(
	edg_wll_Context					ctx,
	const edg_wll_NotifId			nid,
	time_t						   *valid)
{
	char	   *time_s = NULL;


	/*	Format time of validity
	 */
	*valid = time(NULL);
	if (   ctx->peerProxyValidity
		&& (ctx->peerProxyValidity - *valid) < ctx->notifDuration )
		*valid = ctx->peerProxyValidity;
	else
		*valid += ctx->notifDuration;

	if ( !(time_s = strdup(edg_wll_TimeToDB(*valid))) )
	{
		edg_wll_SetError(ctx, errno, "Formating validity time");
		goto cleanup;
	}
	time_s[strlen(time_s)-1] = 0;

	update_notif(ctx, nid, NULL, NULL, time_s+1);

cleanup:
	if ( time_s ) free(time_s);

	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_NotifDropServer(
	edg_wll_Context					ctx,
	edg_wll_NotifId				   *nid)
{
	char	   *owner = NULL,
			   *nid_s = NULL,
			   *stmt;
	int			ret;


	if ( !(owner = get_user(ctx, 0)) )
	{
		if ( !edg_wll_Error(ctx, NULL, NULL) )
			edg_wll_SetError(ctx, EPERM, "Unknown user");

		return edg_wll_Error(ctx, NULL, NULL);
	}

	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) )
		goto cleanup;

	/* Only the owner could remove the notification registration
	 */
	trio_asprintf(&stmt,
				"delete from notif_registrations where notifid='%|Ss' and userid='%|Ss'",
				nid_s, owner);
	if ( (ret = edg_wll_ExecStmt(ctx, stmt, NULL)) < 0 )
		goto cleanup;
	free(stmt);
	if ( ret == 0 )
	{
		trio_asprintf(&stmt,
				"select notifid from notif_registrations where notifid='%|Ss'", nid_s);
		ret = edg_wll_ExecStmt(ctx, stmt, NULL);
		if ( ret == 0 )
			edg_wll_SetError(ctx, ENOENT, "Unknown notification ID");
		else if ( ret > 0 )
			edg_wll_SetError(ctx, EPERM, NULL);

		goto cleanup;
	}
	trio_asprintf(&stmt, "delete from notif_jobs where notifid='%|Ss'", nid_s);
	edg_wll_ExecStmt(ctx, stmt, NULL);

cleanup:
	if ( owner ) free(owner);
	if ( nid_s ) free(nid_s);
	if ( stmt ) free(stmt);

	return edg_wll_Error(ctx, NULL, NULL);
}

static char *get_user(edg_wll_Context ctx, int create)
{
	edg_wll_Stmt	stmt	= NULL;
	char		   *userid	= NULL,
				   *q		= NULL;
	int				ret;


	if ( !ctx->peerName )
	{
		edg_wll_SetError(ctx, EPERM, "Annonymous notifications not allowed");
		goto cleanup;
	}
	trio_asprintf(&q, "select userid from users where cert_subj='%|Ss'", ctx->peerName);
	if ( edg_wll_ExecStmt(ctx, q, &stmt) < 0 )
		goto cleanup;

	/*	returned value:
	 *	0		no user find - continue only when 'create' parameter is set
	 *	>0		user found - return selected value
	 *	<0		SQL error
	 */
	if ( ((ret = edg_wll_FetchRow(stmt, &userid)) != 0) || !create )
		goto cleanup;

	if ( !(userid = strdup(strmd5(ctx->peerName, NULL))) )
	{
		edg_wll_SetError(ctx, errno, "Creating user ID");
		goto cleanup;
	}
	free(q);
	trio_asprintf(&q, "insert into users(userid,cert_subj) values ('%|Ss','%|Ss')",
			userid, ctx->peerName);
	if ( edg_wll_ExecStmt(ctx, q, NULL) < 0 )
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
	if ( stmt ) edg_wll_FreeStmt(&stmt);

	return userid;
}

static int update_notif(
	edg_wll_Context					ctx,
	const edg_wll_NotifId			nid,
	const char					   *conds,
	const char					   *dest,
	const char					   *valid)
{
	char	   *owner	= NULL,
			   *nid_s	= NULL,
			   *stmt, *aux;
	int			ret;


	if ( !(owner = get_user(ctx, 0)) )
	{
		if ( !edg_wll_Error(ctx, NULL, NULL) )
			edg_wll_SetError(ctx, EPERM, "Unknown user");

		return edg_wll_Error(ctx, NULL, NULL);
	}

	if ( !(nid_s = edg_wll_NotifIdGetUnique(nid)) )
		goto cleanup;

	/*	Format SQL update string
	 *	(Only the owner could update the notification registration)
	 */
	stmt = strdup("update notif_registrations set");
	if ( dest )
	{
		trio_asprintf(&aux, "%s destination='%|Ss'", stmt, dest);
		free(stmt);
		stmt = aux;
	}
	if ( valid )
	{
		trio_asprintf(&aux, "%s %svalid='%|Ss'", stmt, dest? ",": "", valid);
		free(stmt);
		stmt = aux;
	}
	if ( conds )
	{
		trio_asprintf(&aux, "%s %sconditions='%|Ss'",
						stmt, (dest||valid)? ",": "", conds);
		free(stmt);
		stmt = aux;
	}
	trio_asprintf(&aux, "%s where notifid='%|Ss' and userid='%|Ss'",
						stmt, nid_s, owner);
	free(stmt);
	stmt = aux;

	if ( (ret = edg_wll_ExecStmt(ctx, stmt, NULL)) < 0 )
		goto cleanup;
	if ( ret == 0 )
	{
		free(stmt);
		trio_asprintf(&stmt,
				"select notifid from notif_registrations where notifid='%|Ss'", nid_s);
		ret = edg_wll_ExecStmt(ctx, stmt, NULL);
		if ( ret == 0 )
			edg_wll_SetError(ctx, ENOENT, "Unknown notification ID");
		else if ( ret > 0 )
			edg_wll_SetError(ctx, EPERM, "Updating notification records");
	}

cleanup:
	if ( owner ) free(owner);
	if ( nid_s ) free(nid_s);
	if ( stmt ) free(stmt);

	return edg_wll_Error(ctx, NULL, NULL);
}
