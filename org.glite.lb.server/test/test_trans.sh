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
# Simple script to test United Server Proxy behaviour
# - for internal purposes only
# - test should correspond to tests described at 
#   http://egee.cesnet.cz/mediawiki/index.php/LB_and_JP_cleanup#unify_lb.proxy_.2B_server

#!/bin/bash

QUERY_SOCK=/tmp/lb_proxy_serve.sock


cisti() {
	echo "DB cleanup..."

	mysql -u lbserver -e "delete from users;" lbserver20trans
	mysql -u lbserver -e "delete from status_tags;" lbserver20trans
	mysql -u lbserver -e "delete from states;" lbserver20trans
	mysql -u lbserver -e "delete from short_fields;" lbserver20trans
	mysql -u lbserver -e "delete from jobs;" lbserver20trans
	mysql -u lbserver -e "delete from events;" lbserver20trans
	mysql -u lbserver -e "delete from server_state;" lbserver20trans
	mysql -u lbserver -e "delete from notif_registrations;" lbserver20trans
	mysql -u lbserver -e "delete from notif_jobs;" lbserver20trans
	mysql -u lbserver -e "delete from long_fields;" lbserver20trans
	mysql -u lbserver -e "delete from acls;" lbserver20trans

	echo "done."
}

registruj() {

	echo "Registering...."
	OUT=`org.glite.lb.client/build/job_reg -x -m scientific.civ.zcu.cz:7846 -s application|grep JOBID`
	eval $OUT
	ID1=$EDG_JOBID
	OUT=`org.glite.lb.client/build/job_reg -m scientific.civ.zcu.cz:7846 -s application|grep JOBID`
	eval $OUT
	ID2=$EDG_JOBID
	OUT=`org.glite.lb.client/build/job_reg -x -m skurut68-2.cesnet.cz:9000 -s application|grep JOBID`
	eval $OUT
	ID3=$EDG_JOBID
	echo "done."
}

registruj_kolekce() {

	echo "Registering...."
	OUT=`org.glite.lb.client/build/job_reg -x -C -n 1 -m scientific.civ.zcu.cz:7846 -s application|grep JOBID`
	eval $OUT
	ID1=$EDG_WL_COLLECTION_JOBID
	ID1_SUB=$EDG_WL_SUB_JOBID
	OUT=`org.glite.lb.client/build/job_reg -C -n 1 -m scientific.civ.zcu.cz:7846 -s application|grep JOBID`
	eval $OUT
	ID2=$EDG_WL_COLLECTION_JOBID
	ID2_SUB=$EDG_WL_SUB_JOBID
	OUT=`org.glite.lb.client/build/job_reg -x -C -n 1 -m skurut68-2.cesnet.cz:9000 -s application|grep JOBID`
	eval $OUT
	ID3=$EDG_WL_COLLECTION_JOBID
	ID3_SUB=$EDG_WL_SUB_JOBID
	OUT=`org.glite.lb.client/build/job_reg -x -C -S -n 1 -m scientific.civ.zcu.cz:7846 -s application|grep JOBID`
	eval $OUT
	ID4=$EDG_WL_COLLECTION_JOBID
	ID4_SUB=$EDG_WL_SUB_JOBID
	OUT=`org.glite.lb.client/build/job_reg -C -S -n 1 -m scientific.civ.zcu.cz:7846 -s application|grep JOBID`
	eval $OUT
	ID5=$EDG_WL_COLLECTION_JOBID
	ID5_SUB=$EDG_WL_SUB_JOBID
	OUT=`org.glite.lb.client/build/job_reg -x -C -S -n 1 -m skurut68-2.cesnet.cz:9000 -s application|grep JOBID`
	eval $OUT
	ID6=$EDG_WL_COLLECTION_JOBID
	ID6_SUB=$EDG_WL_SUB_JOBID
	echo "done."

}

vypis() {
	mysql -u lbserver -e "select dg_jobid,proxy,server from jobs" lbserver20trans
}

vypis_kolekci() {
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$1'" lbserver20trans
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$2'" lbserver20trans|grep http
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$3'" lbserver20trans|grep http
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$4'" lbserver20trans|grep http
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$5'" lbserver20trans|grep http
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$6'" lbserver20trans|grep http
	shift 6
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$1'" lbserver20trans|grep http
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$2'" lbserver20trans|grep http
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$3'" lbserver20trans|grep http
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$4'" lbserver20trans|grep http
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$5'" lbserver20trans|grep http
	mysql -u lbserver -e "select dg_jobid, proxy,server from jobs where dg_jobid='$6'" lbserver20trans|grep http
}

do_stavu_cleared() {
	echo "Transfering jobs to cleared state..."
	stage/examples/glite-lb-cleared.sh -x -j $1 2>/dev/null
	stage/examples/glite-lb-cleared.sh -j $2 2>/dev/null  
	stage/examples/glite-lb-cleared.sh -x -j $3 2>/dev/null
	echo "done."
}

check_states() {
	echo
	echo "State of job $1"
	stage/examples/glite-lb-job_status  -x $QUERY_SOCK  $1 | grep "state : "
	stage/examples/glite-lb-job_status $1 |grep "state : "
	echo "State of job $2"
	stage/examples/glite-lb-job_status  -x $QUERY_SOCK  $2 | grep "state : "
	stage/examples/glite-lb-job_status $2 |grep "state : "
	echo "State of job $3"
	stage/examples/glite-lb-job_status  -x $QUERY_SOCK  $3 | grep "state : "
	stage/examples/glite-lb-job_status $3 |grep "state : "
}

test1() {
	echo
	echo "==================== test 1 ============================="

	cisti;
	registruj;
	vypis;
	echo job1=$ID1
	echo job2=$ID2
	echo job3=$ID3
}

test2() {
	echo
	echo "==================== test 2 ============================="

        cisti;
        registruj;
        vypis;
        echo job1=$ID1
        echo job2=$ID2
        echo job3=$ID3

	do_stavu_cleared $ID1 $ID2 $ID3
	vypis;
}

test3() {
	echo
	echo "==================== test 3 ============================="

	cisti;
	registruj;
	vypis;
	echo job1=$ID1
	echo job2=$ID2
	echo job3=$ID3

	sleep 2;
	stage/bin/glite-lb-purge --cleared 1s --aborted 1s --cancelled 1s --other 1s -l -m scientific.civ.zcu.cz:7846
	vypis;
}

test4() {
	echo
	echo "==================== test 4 ============================="

	cisti;
	registruj;
	vypis;
	echo job1=$ID1
	echo job2=$ID2
	echo job3=$ID3
	
	check_states $ID1 $ID2 $ID3
}

test5() {
	echo
	echo "==================== test 5 ============================="

	cisti;
	registruj_kolekce;
	vypis_kolekci  $ID1 $ID1_SUB $ID2 $ID2_SUB  $ID3 $ID3_SUB $ID4 $ID4_SUB $ID5 $ID5_SUB $ID6 $ID6_SUB 
}



####################################################

test1;
test2;
test3;
test4;
test5;
