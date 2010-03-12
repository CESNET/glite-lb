#!/bin/sh
#
# Copyright (c) Members of the EGEE Collaboration. 2004-2010.
# See http://www.eu-egee.org/partners for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


# Example script illustrating injection of LB commands into Job Wrapper
# to log sandbox transfer related events

# It can be used as a submitted job to check the functionality, however,
# in this way it interferes with sequence codes logged by JW itself.
# Therefore it should be taken as an example only; the commands from here
# should end up in the generated job wrapper instead.


# register sandbox transfers with LB
# sets environment variables:
# 	GLITE_WMS_SEQUENCE_CODE	updated seq. code for the job itself
#	GLITE_LB_ISB_JOBID
#	GLITE_LB_ISB_SEQUENCE

eval `glite-lb-register_sandbox \
	--jobid $GLITE_WMS_JOBID	\
	--input \
	--from http://users.machine/path/to/sandbox.file \
	--to file://where/it/is/sandbox.file \
	--sequence $GLITE_WMS_SEQUENCE_CODE`


eval `glite-lb-register_sandbox \
	--jobid $GLITE_WMS_JOBID	\
	--output \
	--from file://where/it/is/sandbox.file2 \
	--to http://users.machine/path/to/sandbox.file2 \
	--sequence $GLITE_WMS_SEQUENCE_CODE`

# ISB transfer 
GLITE_LB_ISB_SEQUENCE=`glite-lb-logevent \
	--source LRMS \
	--jobid $GLITE_LB_ISB_JOBID \
	--sequence $GLITE_LB_ISB_SEQUENCE \
	--event FileTransfer \
	--result START`

# it takes looong
sleep 60

GLITE_LB_ISB_SEQUENCE=`glite-lb-logevent \
	--source LRMS \
	--jobid $GLITE_LB_ISB_JOBID \
	--sequence $GLITE_LB_ISB_SEQUENCE \
	--event FileTransfer \
	--result OK`

# or FAIL with --reason "because of bad weather"

# job payload here
sleep 120


GLITE_LB_OSB_SEQUENCE=`glite-lb-logevent \
	--source LRMS \
	--jobid $GLITE_LB_OSB_JOBID \
	--sequence $GLITE_LB_OSB_SEQUENCE \
	--event FileTransfer \
	--result START`

sleep 60

GLITE_LB_OSB_SEQUENCE=`glite-lb-logevent \
	--source LRMS \
	--jobid $GLITE_LB_OSB_JOBID \
	--sequence $GLITE_LB_OSB_SEQUENCE \
	--event FileTransfer \
	--result OK`

