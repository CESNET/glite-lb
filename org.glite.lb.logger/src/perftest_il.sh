#!/bin/bash

numjobs=$1

# XXX - there must be better way to find stage
STAGEDIR=/home/michal/shared/egee/jra1-head/stage
. $STAGEDIR/sbin/perftest_common.sh

DEBUG=${DEBUG:-0}
# CONSUMER_ARGS=
# PERFTEST_COMPONENT=
# COMPONENT_ARGS=
#LOGJOBS_ARGS="" 

check_test_files || exit 1

PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-interlogd-perf-empty
CONSUMER_ARGS="-d -v"

echo -e "\tsmall_job \t big_job \t small_dag \t big_dag"
run_test il $numjobs
j=0
while [[ $j -lt 4 ]]
do
    echo -e -n "\t ${PERFTEST_THROUGHPUT[$j]}"	
    j=$((j+1))
done
echo ""
#j=0
#while [[ $j -lt 4 ]]
#do
#    echo -e -n "\t (${PERFTEST_EV_THROUGHPUT[$j]})"	
#    j=$((j+1))
#done
#echo ""


#
#    dst=il
#
## i)1)
#
#    glite_lb_interlogd_perf_noparse --nosend
#    run_test()
#
#    glite_lb_interlogd_perf_nosync --nosend
#    run_test()
#
#    glite_lb_interlogd_perf_norecover --nosend
#    run_test()
#
#    glite_lb_interlogd_perf --nosend
#    run_test()
#
## ii)1)
#
#glite_lb_bkserverd_perf_empty
#
#    glite_lb_interlogd_perf_noparse
#    run_test()
#
#    glite_lb_interlogd_perf_nosync
#    run_test()
#    
#    glite_lb_interlogd_perf_norecover
#    run_test()
#
#    glite_lb_interlogd_perf_lazy
#    run_test()
#
#    glite_lb_interlogd_perf
#    run_test()
