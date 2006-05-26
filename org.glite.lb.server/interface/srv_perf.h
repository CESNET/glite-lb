enum lb_srv_perf_sink {
	GLITE_LB_SINK_NONE = 0,
	GLITE_LB_SINK_PARSE,
	GLITE_LB_SINK_STORE,
	GLITE_LB_SINK_STATE,
	GLITE_LB_SINK_SEND,
};


extern enum lb_srv_perf_sink sink_mode;
