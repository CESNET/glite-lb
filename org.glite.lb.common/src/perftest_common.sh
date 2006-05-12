#!/bin/bash

run_test()
{
    for file in small_job big_job small_dag big_dag
    do
	perftest_logjobs --file=$file --dst=$dest --num=$num_jobs
    done
}

