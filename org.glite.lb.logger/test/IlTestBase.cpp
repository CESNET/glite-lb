#include "IlTestBase.h"

#include <string.h>

const char *IlTestBase::msg = "DATE=20040831150159.702224 HOST=\"some.host\" PROG=edg-wms LVL=USAGE DG.PRIORITY=0 DG.SOURCE=\"UserInterface\" DG.SRC_INSTANCE=\"\" DG.EVNT=\"RegJob\" DG.JOBID=\"https://some.host:1234/x67qr549qc\" DG.SEQCODE=\"UI=2:NS=0:WM=0:BH=1:JSS=0:LM=0:LRMS=0:APP=0\" DG.USER=\"/C=CZ/O=Cesnet/CN=Michal Vocu\" DG.REGJOB.JDL=\"\" DG.REGJOB.NS=\"ns address\" DG.REGJOB.PARENT=\"\" DG.REGJOB.JOBTYPE=\"SIMPLE\" DG.REGJOB.NSUBJOBS=\"0\" DG.REGJOB.SEED=\"\"";

const char *IlTestBase::msg_enc = "             429\n6 michal\n415 DATE=20040831150159.702224 HOST=\"some.host\" PROG=edg-wms LVL=USAGE DG.PRIORITY=0 DG.SOURCE=\"UserInterface\" DG.SRC_INSTANCE=\"\" DG.EVNT=\"RegJob\" DG.JOBID=\"https://some.host:1234/x67qr549qc\" DG.SEQCODE=\"UI=2:NS=0:WM=0:BH=1:JSS=0:LM=0:LRMS=0:APP=0\" DG.USER=\"/C=CZ/O=Cesnet/CN=Michal Vocu\" DG.REGJOB.JDL=\"\" DG.REGJOB.NS=\"ns address\" DG.REGJOB.PARENT=\"\" DG.REGJOB.JOBTYPE=\"SIMPLE\" DG.REGJOB.NSUBJOBS=\"0\" DG.REGJOB.SEED=\"\"\n";

const struct server_msg IlTestBase::smsg = {
	"https://some.host:1234/x67qr549qc",
	(char*)IlTestBase::msg_enc,
	strlen(IlTestBase::msg_enc),
	strlen(IlTestBase::msg) + 1,
	NULL
};
