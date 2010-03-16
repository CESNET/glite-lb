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


#include <stdlib.h>

#include "glite/lb/consumer.h"
#include "glite/lb/context-int.h"

#include "lbs_db.h"

edg_wll_ErrorCode edg_wll_Open(edg_wll_Context ctx, char *cs)
{
	return edg_wll_DBConnect(ctx,cs) ? edg_wll_Error(ctx,NULL,NULL) : 0;
}

edg_wll_ErrorCode edg_wll_Close(edg_wll_Context ctx)
{
	return edg_wll_ResetError(ctx);
}
