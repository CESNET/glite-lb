#!/bin/bash

run_test()
{
    for file in small_job big_job small_dag big_dag
    do
	perftest_logjobs --test=$test_name --file=$file --dst=$dest --num=$num_jobs
    done
}

print_results()
{
    total=`echo "scale=9; $PERFTEST_END_TIMESTAMP - $PERFTEST_BEGIN_TIMESTAMP" | bc`
    echo -e "Total time for PERFTEST_NUM_JOBS: \t$total"
    echo -e -n "Event throughput: \t"
    echo "scale=9; $PERFTEST_NUM_JOBS * $PERFTEST_JOB_SIZE / $total" |bc
    echo -e -n "Job throughput: \t"
    echo "scale=9; $PERFTEST_NUM_JOBS / $total" |bc
}