#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "process.h"
#include "fcfs.h"
#include "sjf.h"
#include "srtf.h"
#include "rr.h"
#include "priority_np.h"
#include "priority_p.h"
#include "hrrn.h"
#include "mlfq.h"

static int failures = 0;

static void expect_near(const char *name, float actual, float expected) {
    if (fabsf(actual - expected) > 0.01f) {
        fprintf(stderr, "FAIL: %s: got %.4f, expected %.4f\n",
                name, actual, expected);
        failures++;
    }
}

static void expect_int(const char *name, int actual, int expected) {
    if (actual != expected) {
        fprintf(stderr, "FAIL: %s: got %d, expected %d\n",
                name, actual, expected);
        failures++;
    }
}

static void check_summary(const char *name, ResultSummary result,
                          float waiting, float turnaround, float response,
                          int switches) {
    char label[96];
    snprintf(label, sizeof(label), "%s average waiting", name);
    expect_near(label, result.avgWT, waiting);
    snprintf(label, sizeof(label), "%s average turnaround", name);
    expect_near(label, result.avgTAT, turnaround);
    snprintf(label, sizeof(label), "%s average response", name);
    expect_near(label, result.avgRT, response);
    snprintf(label, sizeof(label), "%s context switches", name);
    expect_int(label, result.contextSwitches, switches);
    expect_near("CPU utilization", result.cpuUtilization, 100.0f);
    expect_near("throughput", result.throughput, 1.0f / 3.0f);
}

static int test_algorithms(void) {
    Process processes[3];
    init_process(&processes[0], 1, 0, 5, 2);
    init_process(&processes[1], 2, 1, 3, 1);
    init_process(&processes[2], 3, 2, 1, 3);

    check_summary("FCFS", execute_fcfs(processes, 3),
                  10.0f / 3.0f, 19.0f / 3.0f, 10.0f / 3.0f, 2);
    check_summary("SJF", execute_sjf_non_preemptive(processes, 3),
                  8.0f / 3.0f, 17.0f / 3.0f, 8.0f / 3.0f, 2);
    check_summary("SRTF", execute_srtf(processes, 3),
                  5.0f / 3.0f, 14.0f / 3.0f, 0.0f, 4);
    check_summary("Round Robin", execute_rr(processes, 3, 2),
                  10.0f / 3.0f, 19.0f / 3.0f, 1.0f, 5);
    check_summary("Priority NP", execute_priority_np(processes, 3),
                  10.0f / 3.0f, 19.0f / 3.0f, 10.0f / 3.0f, 2);
    check_summary("Priority P", execute_priority_p(processes, 3),
                  3.0f, 6.0f, 2.0f, 3);
    check_summary("HRRN", execute_hrrn(processes, 3),
                  8.0f / 3.0f, 17.0f / 3.0f, 8.0f / 3.0f, 2);
    check_summary("MLFQ", execute_mlfq(processes, 3),
                  10.0f / 3.0f, 19.0f / 3.0f, 1.0f, 4);

    expect_int("input process remains unchanged", processes[0].remainingTime, 5);
    expect_int("input process completion remains unchanged",
               processes[0].completionTime, 0);

    Process idleProcess;
    init_process(&idleProcess, 9, 5, 3, 1);
    ResultSummary idleResult = execute_fcfs(&idleProcess, 1);
    expect_near("idle workload CPU utilization", idleResult.cpuUtilization, 37.5f);
    expect_near("idle workload throughput", idleResult.throughput, 0.125f);
    expect_near("idle workload response", idleResult.avgRT, 0.0f);
    return failures == 0;
}

static int test_csv_round_trip(void) {
    const char *processPath = "build/test_processes.csv";
    const char *resultsPath = "build/test_results.csv";
    const char *invalidPath = "build/test_invalid_processes.csv";
    char error[256];
    Process original[2];
    init_process(&original[0], 4, 0, 7, 2);
    init_process(&original[1], 8, 3, 2, 1);
    if (!export_processes_csv(processPath, original, 2, error, sizeof(error))) {
        fprintf(stderr, "FAIL: process CSV export: %s\n", error);
        failures++;
        return 0;
    }

    Process *loaded = NULL;
    int loadedCount = 0;
    if (!import_processes_csv(processPath, &loaded, &loadedCount,
                              error, sizeof(error))) {
        fprintf(stderr, "FAIL: process CSV import: %s\n", error);
        failures++;
        remove(processPath);
        return 0;
    }
    expect_int("CSV process count", loadedCount, 2);
    expect_int("CSV first PID", loaded[0].pid, 4);
    expect_int("CSV second arrival", loaded[1].arrivalTime, 3);
    expect_int("CSV second burst", loaded[1].burstTime, 2);

    FILE *invalidFile = fopen(invalidPath, "w");
    if (invalidFile == NULL) {
        fprintf(stderr, "FAIL: could not create invalid CSV fixture\n");
        failures++;
    } else {
        fputs("pid,arrival,burst,priority\n1,0,2,1\n1,2,3,2\n", invalidFile);
        fclose(invalidFile);
        Process *unchangedPointer = loaded;
        int unchangedCount = loadedCount;
        if (import_processes_csv(invalidPath, &loaded, &loadedCount,
                                 error, sizeof(error))) {
            fprintf(stderr, "FAIL: duplicate PID CSV should be rejected\n");
            failures++;
            free(loaded);
            loaded = unchangedPointer;
            loadedCount = unchangedCount;
        } else if (loaded != unchangedPointer || loadedCount != unchangedCount) {
            fprintf(stderr, "FAIL: failed CSV import changed loaded data\n");
            failures++;
        }
    }

    ResultSummary comparison[1] = {{"FCFS", 1.0f, 2.0f, 0.5f,
                                    100.0f, 0.25f, 1}};
    if (!export_results_csv(resultsPath, comparison, 1, error, sizeof(error))) {
        fprintf(stderr, "FAIL: result CSV export: %s\n", error);
        failures++;
    }
    free(loaded);
    remove(processPath);
    remove(resultsPath);
    remove(invalidPath);
    return failures == 0;
}

static int test_invalid_inputs(void) {
    ResultSummary res;
    res = execute_fcfs(NULL, 0);
    expect_int("FCFS null context switches", res.contextSwitches, 0);
    res = execute_fcfs(NULL, -5);
    expect_int("FCFS negative count context switches", res.contextSwitches, 0);
    res = execute_sjf_non_preemptive(NULL, 0);
    expect_int("SJF null context switches", res.contextSwitches, 0);
    res = execute_srtf(NULL, 0);
    expect_int("SRTF null context switches", res.contextSwitches, 0);
    res = execute_rr(NULL, 0, 2);
    expect_int("RR zero processes context switches", res.contextSwitches, 0);
    res = execute_rr(NULL, 3, 0);
    expect_int("RR zero quantum context switches", res.contextSwitches, 0);
    res = execute_rr(NULL, 3, -1);
    expect_int("RR negative quantum context switches", res.contextSwitches, 0);
    res = execute_priority_np(NULL, 0);
    expect_int("Priority NP null context switches", res.contextSwitches, 0);
    res = execute_priority_p(NULL, 0);
    expect_int("Priority P null context switches", res.contextSwitches, 0);
    res = execute_hrrn(NULL, 0);
    expect_int("HRRN null context switches", res.contextSwitches, 0);
    res = execute_mlfq(NULL, 0);
    expect_int("MLFQ null context switches", res.contextSwitches, 0);
    res = execute_mlfq(NULL, -1);
    expect_int("MLFQ negative count context switches", res.contextSwitches, 0);
    return failures == 0;
}

static int test_mlfq_features(void) {
    // 1. Test queue demotion across all 3 queues (Q1 -> Q2 -> Q3)
    Process pDemote[2];
    init_process(&pDemote[0], 1, 0, 10, 1);
    init_process(&pDemote[1], 2, 0, 2, 2);
    ResultSummary resDemote = execute_mlfq(pDemote, 2);
    expect_near("MLFQ demotion avg waiting", resDemote.avgWT, 2.0f);
    expect_near("MLFQ demotion avg turnaround", resDemote.avgTAT, 8.0f);
    expect_near("MLFQ demotion avg response", resDemote.avgRT, 1.0f);
    expect_int("MLFQ demotion context switches", resDemote.contextSwitches, 2);
    expect_near("MLFQ demotion CPU util", resDemote.cpuUtilization, 100.0f);

    // 2. Test preemption across queue levels with different arrival times
    Process pPreempt[2];
    init_process(&pPreempt[0], 1, 0, 8, 1);
    init_process(&pPreempt[1], 2, 3, 2, 2);
    ResultSummary resPreempt = execute_mlfq(pPreempt, 2);
    expect_near("MLFQ preemption avg waiting", resPreempt.avgWT, 1.0f);
    expect_near("MLFQ preemption avg turnaround", resPreempt.avgTAT, 6.0f);
    expect_near("MLFQ preemption avg response", resPreempt.avgRT, 0.0f);
    expect_int("MLFQ preemption context switches", resPreempt.contextSwitches, 2);
    expect_near("MLFQ preemption CPU util", resPreempt.cpuUtilization, 100.0f);

    // 3. Test idle periods
    Process pIdle;
    init_process(&pIdle, 5, 5, 3, 1);
    ResultSummary resIdle = execute_mlfq(&pIdle, 1);
    expect_near("MLFQ idle avg waiting", resIdle.avgWT, 0.0f);
    expect_near("MLFQ idle avg turnaround", resIdle.avgTAT, 3.0f);
    expect_near("MLFQ idle avg response", resIdle.avgRT, 0.0f);
    expect_int("MLFQ idle context switches", resIdle.contextSwitches, 0);
    expect_near("MLFQ idle CPU util", resIdle.cpuUtilization, 37.5f);
    expect_near("MLFQ idle throughput", resIdle.throughput, 0.125f);

    return failures == 0;
}

int main(void) {
    test_algorithms();
    test_csv_round_trip();
    test_invalid_inputs();
    test_mlfq_features();
    if (failures != 0) {
        fprintf(stderr, "\n%d test(s) failed.\n", failures);
        return EXIT_FAILURE;
    }
    printf("\nAll CPU scheduling tests passed.\n");
    return EXIT_SUCCESS;
}
