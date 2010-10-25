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

#include "IlTestBase.h"

#include <string.h>

const char *IlTestBase::msg = "DATE=20040831150159.702224 HOST=\"some.host\" PROG=edg-wms LVL=USAGE DG.PRIORITY=0 DG.SOURCE=\"UserInterface\" DG.SRC_INSTANCE=\"\" DG.EVNT=\"RegJob\" DG.JOBID=\"https://some.host:1234/x67qr549qc\" DG.SEQCODE=\"UI=2:NS=0:WM=0:BH=1:JSS=0:LM=0:LRMS=0:APP=0\" DG.USER=\"/C=CZ/O=Cesnet/CN=Michal Vocu\" DG.REGJOB.JDL=\"\" DG.REGJOB.NS=\"ns address\" DG.REGJOB.PARENT=\"\" DG.REGJOB.JOBTYPE=\"SIMPLE\" DG.REGJOB.NSUBJOBS=\"0\" DG.REGJOB.SEED=\"\"";

const char *IlTestBase::msg_enc = "             429\n6 michal\n415 DATE=20040831150159.702224 HOST=\"some.host\" PROG=edg-wms LVL=USAGE DG.PRIORITY=0 DG.SOURCE=\"UserInterface\" DG.SRC_INSTANCE=\"\" DG.EVNT=\"RegJob\" DG.JOBID=\"https://some.host:1234/x67qr549qc\" DG.SEQCODE=\"UI=2:NS=0:WM=0:BH=1:JSS=0:LM=0:LRMS=0:APP=0\" DG.USER=\"/C=CZ/O=Cesnet/CN=Michal Vocu\" DG.REGJOB.JDL=\"\" DG.REGJOB.NS=\"ns address\" DG.REGJOB.PARENT=\"\" DG.REGJOB.JOBTYPE=\"SIMPLE\" DG.REGJOB.NSUBJOBS=\"0\" DG.REGJOB.SEED=\"\"\n";

const struct server_msg IlTestBase::smsg = {
	"https://some.host:1234/x67qr549qc",
	0L,
	(char*)IlTestBase::msg_enc,
	strlen(IlTestBase::msg_enc),
	strlen(IlTestBase::msg) + 1,
	NULL,
	0,
	0L,
	"some.host",
	1234,
	"some.host:1234",
	0
};
