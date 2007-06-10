#ifndef __GLITE_LB_SRV_PERF_H__
#define __GLITE_LB_SRV_PERF_H__

#ident "$Header$"

enum lb_srv_perf_sink {
	GLITE_LB_SINK_NONE = 0,	// standard behaviour, no sinking

	GLITE_LB_SINK_PARSE,	// skip everything, only read message and
				// generate reply 
				// skips decode_il_msg, db_store 

	GLITE_LB_SINK_STORE,	// do no store data to DB, only decode_il_msg
				// and edg_wll_ParseEvent
				// skips edg_wll_LockJob, edg_wll_StoreEvent 
				// edg_wll_UpdateACL, edg_wll_StepIntState,
				// edg_wll_EventSendProxy, edg_wll_NotifMatch

	GLITE_LB_SINK_STATE,	// do not perform state computation, only save 
				// event to DB
				// skips edg_wll_StepIntState
				// edg_wll_EventSendProxy, edg_wll_NotifMatch

	GLITE_LB_SINK_SEND,
};


extern enum lb_srv_perf_sink sink_mode;

#endif /* __GLITE_LB_SRV_PERF_H__ */
