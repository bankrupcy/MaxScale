add_library(mariadbmon_utils SHARED mariadbmon_utils.cc)
target_link_libraries(mariadbmon_utils maxtest)
set_target_properties(mariadbmon_utils PROPERTIES VERSION "1.0.0" LINK_FLAGS -Wl,-z,defs)
install(TARGETS mariadbmon_utils DESTINATION system-test/mariadbmonitor)
add_dependencies(mariadbmon_utils maxtest)

# Two-server multimaster topology
add_test_executable_ex(NAME mariadbmon_multimaster_simple SOURCE mariadbmon_multimaster_simple.cc
        CONFIG mariadbmon_multimaster_simple.cnf VMS repl_backend LABELS mysqlmon)

# MySQL Monitor with Multi-master configurations
add_test_executable_ex(NAME mysqlmon_multimaster SOURCE mysqlmon_multimaster.cc
        CONFIG mysqlmon_multimaster.cnf VMS repl_backend LABELS mysqlmon)

# MySQL Monitor with Multi-master configurations (assume_unique_hostnames=OFF)
add_test_executable_ex(NAME mysqlmon_multimaster_serverid ORIG_NAME mysqlmon_multimaster
        CONFIG mysqlmon_multimaster_serverid.cnf VMS repl_backend LABELS mysqlmon)

# Check that standalone master is allowed. Also, MXS-4301: Allow [mAxScalE]-section
add_test_executable_ex(NAME mysqlmon_detect_standalone_master SOURCE mysqlmon_detect_standalone_master.cc
        CONFIG mysqlmon_detect_standalone_master.cnf VMS repl_backend LABELS mysqlmon)

# MySQL Monitor Failover (automatic) Test
add_test_executable_ex(NAME mysqlmon_failover_auto SOURCE mysqlmon_failover_auto.cc
        CONFIG mysqlmon_failover_auto.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

# MySQL Monitor Failover (manual) Test
add_test_executable_ex(NAME mysqlmon_failover_manual SOURCE mysqlmon_failover_manual.cc
        CONFIG mysqlmon_failover_manual.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

# MySQL Monitor manual failover with many valid candidates
add_test_executable_ex(NAME mysqlmon_failover_manual2_4 SOURCE mysqlmon_failover_manual2.cc
        CONFIG mysqlmon_failover_manual2_4.cnf VMS repl_backend LABELS mysqlmon)
add_test_executable_ex(NAME mysqlmon_failover_manual2_3 SOURCE mysqlmon_failover_manual2.cc
        CONFIG mysqlmon_failover_manual2_3.cnf VMS repl_backend LABELS mysqlmon)
add_test_executable_ex(NAME mysqlmon_failover_manual2_2 SOURCE mysqlmon_failover_manual2.cc
        CONFIG mysqlmon_failover_manual2_2.cnf VMS repl_backend LABELS mysqlmon)

# MySQL Monitor switchover
add_test_executable_ex(NAME mysqlmon_switchover SOURCE mysqlmon_switchover.cc CONFIG mysqlmon_switchover.cnf
        VMS repl_backend LABELS mysqlmon switchover)

# MXS-4743 switchover-force works even with high replication lag
add_test_executable_ex(NAME mariadbmon_switchover_force SOURCE mariadbmon_switchover_force.cc
        CONFIG mariadbmon_switchover_force.cnf
        VMS repl_backend LABELS mysqlmon switchover)

# MySQL Monitor switchover with bad master
add_test_executable_ex(NAME mysqlmon_switchover_bad_master SOURCE mysqlmon_switchover_bad_master.cc
        CONFIG mysqlmon_switchover_bad_master.cnf VMS repl_backend LABELS mysqlmon switchover)

# MySQL Monitor manual failover with no valid slaves, uses config of mysqlmon_failover_auto. Also MXS-2652.
add_test_executable_ex(NAME mysqlmon_failover_no_slaves SOURCE mysqlmon_failover_no_slaves.cc
        CONFIG mysqlmon_failover_auto.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

# MySQL Monitor Rejoin (good) Test
add_test_executable_ex(NAME mysqlmon_rejoin_good SOURCE mysqlmon_rejoin_good.cc
        CONFIG mysqlmon_rejoin_good.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

add_test_executable_ex(NAME mysqlmon_rejoin_bad SOURCE mysqlmon_rejoin_bad.cc
        CONFIG mysqlmon_rejoin_bad.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

# MySQL Monitor Rejoin (bad2) Test, use template for Rejoin (good)
add_test_executable_ex(NAME mysqlmon_rejoin_bad2 SOURCE mysqlmon_rejoin_bad2.cc
        CONFIG mysqlmon_rejoin_bad.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

# MySQL Monitor Rejoin tests
add_test_executable_ex(NAME mysqlmon_rejoin_manual SOURCE mysqlmon_rejoin_manual.cc
        CONFIG mysqlmon_rejoin_manual.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)
add_test_executable_ex(NAME mysqlmon_rejoin_manual2 SOURCE mysqlmon_rejoin_manual2.cc
        CONFIG mysqlmon_rejoin_manual.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

# MySQL Monitor rolling master
add_test_executable_ex(NAME mysqlmon_failover_rolling_master SOURCE mysqlmon_failover_rolling_master.cc
        CONFIG mysqlmon_failover_rolling_master.cnf VMS repl_backend LABELS mysqlmon)

# MySQL Monitor rejoin old slave
add_test_executable_ex(NAME mysqlmon_failover_rejoin_old_slave SOURCE mysqlmon_failover_rejoin_old_slave.cc
        CONFIG mysqlmon_failover_rejoin_old_slave.cnf VMS repl_backend LABELS mysqlmon)

# MySQL Monitor rolling restart slaves
add_test_executable_ex(NAME mysqlmon_failover_rolling_restart_slaves SOURCE mysqlmon_failover_rolling_restart_slaves.cc
        CONFIG mysqlmon_failover_rolling_restart_slaves.cnf VMS repl_backend LABELS mysqlmon)

# MySQL Monitor failover stress
add_test_executable_ex(NAME mysqlmon_failover_stress SOURCE mysqlmon_failover_stress.cc
        CONFIG mysqlmon_failover_stress.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)
# MySQL Monitor failover stress with semisync
add_test_executable_ex(NAME mysqlmon_failover_stress_ss SOURCE mysqlmon_failover_stress_ss.cc
        CONFIG mysqlmon_failover_stress.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

# MySQL Monitor switchover stress
add_test_executable_ex(NAME mysqlmon_switchover_stress SOURCE mysqlmon_switchover_stress.cc
        CONFIG mysqlmon_switchover_stress.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

# Check monitoring and failover with an external master
add_test_executable_ex(NAME mysqlmon_external_master SOURCE mysqlmon_external_master.cc
        CONFIG mysqlmon_external_master.cnf VMS repl_backend LABELS mysqlmon)

# Check failover, switchover and rejoin with scheduled server events
# Also, MXS-3158
add_test_executable_ex(NAME mysqlmon_fail_switch_events SOURCE mysqlmon_fail_switch_events.cc
        CONFIG mysqlmon_rejoin_good.cnf VMS repl_backend LABELS mysqlmon)

# MySQL Monitor reset-replication test, use template for Rejoin (good)
add_test_executable_ex(NAME mysqlmon_reset_replication SOURCE mysqlmon_reset_replication.cc
        CONFIG mysqlmon_rejoin_good.cnf VMS repl_backend LABELS mysqlmon)

# MySQL Monitor low disk switchover
add_test_executable_ex(NAME mysqlmon_switchover_auto SOURCE mysqlmon_switchover_auto.cc
        CONFIG mysqlmon_switchover_auto.cnf VMS repl_backend LABELS mysqlmon)

# MySQL Monitor series of failovers and rejoins
add_test_executable_ex(NAME mysqlmon_failover_readonly SOURCE mysqlmon_failover_readonly.cc
        CONFIG mysqlmon_failover_readonly.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

# MXS-1493: Use replication heartbeat in mysqlmon https://jira.mariadb.org/browse/MXS-1493
add_test_executable_ex(NAME verify_master_failure SOURCE verify_master_failure.cc
        CONFIG verify_master_failure.cnf VMS repl_backend LABELS mysqlmon)

# MariaDB-Monitor enforce_simple_topology
# Also MXS-3324 switchover when autocommit is off on backends.
add_test_executable_ex(NAME mysqlmon_enforce_simple SOURCE mysqlmon_enforce_simple.cc
        CONFIG mysqlmon_enforce_simple.cnf VMS repl_backend LABELS mysqlmon)

add_test_executable_ex(NAME mariadbmon_enforce_writable_master SOURCE mariadbmon_enforce_writable_master.cc
        CONFIG mariadbmon_enforce_writable_master.cnf VMS repl_backend LABELS mysqlmon)

# MariaDB-Monitor multimaster failover/switchover
add_test_executable_ex(NAME mysqlmon_fail_switch_multi SOURCE mysqlmon_fail_switch_multi.cc
        CONFIG mysqlmon_fail_switch_multi.cnf VMS repl_backend LABELS mysqlmon LIBS mariadbmon_utils)

# MXS-3018 MariaDB-Monitor master_conditions and slave_conditions
add_test_executable_ex(NAME mariadbmon_master_slave_conditions SOURCE master_slave_conditions.cc
        CONFIG master_slave_conditions.cnf VMS repl_backend LABELS mysqlmon)

add_test_executable_ex(NAME mariadbmon_rebuild_server SOURCE mariadbmon_rebuild_server.cc
        CONFIG mariadbmon_rebuild_server.cnf VMS repl_backend LABELS mysqlmon)

add_test_executable_ex(NAME mariadbmon_prom_dem_files SOURCE mariadbmon_prom_dem_files.cc
        CONFIG mariadbmon_prom_dem_files.cnf VMS repl_backend LABELS mysqlmon)

add_test_executable_ex(NAME mariadbmon_switchover_kill_super SOURCE mariadbmon_switchover_kill_super.cc
        CONFIG mariadbmon_switchover_kill_super.cnf VMS repl_backend LABELS mysqlmon)

add_test_executable_ex(NAME mariadbmon_repl_custom_opts SOURCE mariadbmon_repl_custom_opts.cc
        CONFIG mariadbmon_repl_custom_opts.cnf VMS repl_backend LABELS mysqlmon)

# Slave with broken replication does not cause new master selection or cannot be selected as master.
# MXS-4798, MXS-4841
add_test_executable_ex(NAME mariadbmon_broken_repl_master SOURCE mariadbmon_broken_repl_master.cc
        CONFIG mariadbmon_broken_repl_master.cnf VMS repl_backend LABELS mysqlmon)

# Two MaxScales with cooperative monitoring. Does not really break replication but mark as such anyway
# to move test exec to a separate MV.
add_test_executable_ex(NAME cooperative_monitoring SOURCE cooperative_monitoring.cc
        CONFIG cooperative_monitoring.cnf1 cooperative_monitoring.cnf2 VMS repl_backend second_maxscale
        LABELS mysqlmon BREAKS_REPL)
