
typedef struct _glite_lb_SrvContext {
	glite_lb_SrvFlesh	*flesh;
} * glite_lb_SrvContext;

typedef struct _glite_lb_SrvFlesh {
	int (*JobIdGetUnique)(char *jobid);

/** compute state from blob, retrive whatever else is required */
	int (*ComputeStateBlob)(glite_lb_SrvContext ctx,glite_lbu_Blob *state_blob,const glite_lb_StateOpts *opts,glite_lb_Status **state_out);

/** retrieve necessary events via bones, compute state */
	int (*ComputeStateFull)(glite_lb_SrvContext ctx,const char *job,
			const glite_lb_StateOpts *opts,
			glite_lb_State **state_out,
			glite_lbu_Blob *blob_out);
	
} glite_lb_SrvFlesh;
