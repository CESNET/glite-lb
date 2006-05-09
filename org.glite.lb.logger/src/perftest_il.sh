
    dst=il

# i)1)

    glite_lb_interlogd_perf_noparse_nosend
    run_test()

    glite_lb_interlogd_perf_nosync_nosend
    run_test()

    glite_lb_interlogd_perf_norecover_nosend
    run_test()

    glite_lb_interlogd_perf_nosend
    run_test()

# ii)1)

glite_lb_bkserverd_perf_empty

    glite_lb_interlogd_perf_noparse
    run_test()

    glite_lb_interlogd_perf_nosync
    run_test()
    
    glite_lb_interlogd_perf_norecover
    run_test()

    glite_lb_interlogd_perf_lazy
    run_test()

    glite_lb_interlogd_perf
    run_test()
