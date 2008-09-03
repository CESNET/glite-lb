#ifndef GLITE_LB_STATISTICS_H
#define GLITE_LB_STATISTICS_H

#ifdef BUILDING_LB_CLIENT
#include "consumer.h"
#else
#include "glite/lb/consumer.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Count the number of jobs which entered the specified state.
 * \param[in] group 		group of jobs of interest, eg. DESTINATION = something
 * 		(XXX: this is the only query supported right now)
 * \param[in] major 		major code of the state of interest
 * \param[in] minor 		minor state code, eg. DONE_FAILED
 * \param[in,out] from,to 	on input - requested interval of interest
 * 	on output - when the data were available
 * \param[out] rate 		average rate per second in which the jobs enter this state
 * \param[out] res_from,res_to	time resolution of the data (seconds)
 */

int edg_wll_StateRate(
	edg_wll_Context	context,
	const edg_wll_QueryRec	*group,
	edg_wll_JobStatCode	major,
	int			minor,
	time_t	*from, 
	time_t	*to,
	float	*rate,
	int	*res_from,
	int	*res_to
);


/** Compute average time for which jobs stay in the specified state.
 * \see edg_wll_StateRate for description of parameters.
 */

int edg_wll_StateDuration(
	edg_wll_Context	context,
	const edg_wll_QueryRec	*group,
	edg_wll_JobStatCode	major,
	int			minor,
	time_t	*from, 
	time_t	*to,
	float	*duration,
	int	*res_from,
	int	*res_to
);



#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_CONSUMER_H */
