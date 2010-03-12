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

package org.glite.lb;

/* FIXME: cleanup required */

/**
 * Enum which represents type if sources.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public class Sources {
    public static final int NONE = 0;            /* uninitialized value */
    public static final int USER_INTERFACE = 1;
    public static final int NETWORK_SERVER = 2;
    public static final int WORKLOAD_MANAGER = 3;
    public static final int BIG_HELPER = 4;
    public static final int JOB_SUBMISSION = 5;
    public static final int LOG_MONITOR = 6;
    public static final int LRMS = 7;
    public static final int APPLICATION = 8;
    public static final int LB_SERVER = 9;
    public static final int CREAM_INTERFACE = 10;
    public static final int CREAM_EXECUTOR = 11;

    public int source;
    private String[] names = {
	"None",
	"UserInterface",
	"NetworkServer",
	"WorkloadManager",
	"BigHelper",
	"JobSubmission",
	"LogMonitor",
	"LRMS",
	"Application",
	"LBServer",
	"CREAMInterface",
	"CREAMExecutor",
    };
    
    public Sources() {
	this.source = 0;
    }

    public static void check(int source) {
	if (source < 1 || source > 11) 
		throw new IllegalArgumentException("lb.Source");
    }
    
    public Sources(int source) {
	check(source);
	this.source = source;
    }

    public String toString() {
	return names[source];
    }
}
