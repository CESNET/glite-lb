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

#ifndef GLITE_LB_TIMEOUTS_H
#define GLITE_LB_TIMEOUTS_H

/** 
 * default and maximal notif timeout (in seconds)
 */
#define EDG_WLL_NOTIF_TIMEOUT_DEFAULT   120
#define EDG_WLL_NOTIF_TIMEOUT_MAX       1800

/**
 * default and maximal query timeout (in seconds)
 */
#define EDG_WLL_QUERY_TIMEOUT_DEFAULT   120
#define EDG_WLL_QUERY_TIMEOUT_MAX       1800

/**
 * default and maximal logging timeout (in seconds)
 */
#define EDG_WLL_LOG_TIMEOUT_DEFAULT		120
#define EDG_WLL_LOG_TIMEOUT_MAX			300
#define EDG_WLL_LOG_SYNC_TIMEOUT_DEFAULT	120
#define EDG_WLL_LOG_SYNC_TIMEOUT_MAX		600

#endif /* GLITE_LB_TIMEOUTS_H */
