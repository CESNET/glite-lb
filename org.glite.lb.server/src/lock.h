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

#define edg_wll_LockJob(ctx,job) edg_wll_LockUnlockJob((ctx),(job),-1)
#define edg_wll_UnlockJob(ctx,job) edg_wll_LockUnlockJob((ctx),(job),1)

int edg_wll_LockUnlockJob(const edg_wll_Context,const edg_wlc_JobId,int);
int edg_wll_JobSemaphore(const edg_wll_Context ctx, const edg_wlc_JobId job);
