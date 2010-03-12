#ifndef GLITE_LB_SRV_PERF_H
#define GLITE_LB_SRV_PERF_H

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

#endif /* GLITE_LB_SRV_PERF_H */
