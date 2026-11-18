#include <stdio.h>
#include <stdlib.h>
#include "sjf.h"

ResultSummary execute_sjf_non_preemptive(Process processes[], int n) {
    if (processes == NULL || n <= 0) {
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    // 1. Create a local copy so we don't modify the original array
    Process *p = (Process *)malloc(n * sizeof(Process));
    if (p == NULL) {
        printf("Memory allocation failed for SJF copy!\n");
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    for (int i = 0; i < n; i++) {
        p[i] = processes[i];
    }
    
    // Arrays for Gantt chart tracking
    GanttBlock *chart = (GanttBlock *)malloc(2 * n * sizeof(GanttBlock));
    if (chart == NULL) {
        printf("Memory allocation failed for Gantt chart!\n");
        free(p);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    int chartSize = 0;
    
    // Array to keep track of completed processes
    int *isCompleted = (int *)calloc(n, sizeof(int));
    if (isCompleted == NULL) {
        printf("Memory allocation failed for completion array!\n");
        free(p);
        free(chart);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    
    int currentTime = 0;
    int completedCount = 0;
    
    // 2. Simulate Execution
    while (completedCount < n) {
        int selectedProcess = -1;
        int minBurst = 999999999;
        
        for (int i = 0; i < n; i++) {
            if (p[i].arrivalTime <= currentTime && !isCompleted[i]) {
                if (p[i].burstTime < minBurst) {
                    minBurst = p[i].burstTime;
                    selectedProcess = i;
                } else if (p[i].burstTime == minBurst) {
                    // Tie breaker 1: Earliest Arrival Time
                    if (p[i].arrivalTime < p[selectedProcess].arrivalTime) {
                        selectedProcess = i;
                    } else if (p[i].arrivalTime == p[selectedProcess].arrivalTime) {
                        // Tie breaker 2: Smallest PID
                        if (p[i].pid < p[selectedProcess].pid) {
                            selectedProcess = i;
                        }
                    }
                }
            }
        }
        
        if (selectedProcess == -1) {
            // CPU is idle. Find the next earliest arrival time among uncompleted
            int nextArrival = 999999999;
            for (int i = 0; i < n; i++) {
                if (!isCompleted[i] && p[i].arrivalTime < nextArrival) {
                    nextArrival = p[i].arrivalTime;
                }
            }
            
            // Record IDLE block
            chart[chartSize].pid = -1;
            chart[chartSize].startTime = currentTime;
            chart[chartSize].endTime = nextArrival;
            chartSize++;
            
            currentTime = nextArrival;
        } else {
            // Process execution
            int i = selectedProcess;
            p[i].firstStartTime = currentTime;
            p[i].responseTime = p[i].firstStartTime - p[i].arrivalTime;
            
            chart[chartSize].pid = p[i].pid;
            chart[chartSize].startTime = currentTime;
            
            currentTime += p[i].burstTime;
            
            chart[chartSize].endTime = currentTime;
            chartSize++;
            
            // Calculate metrics
            p[i].completionTime = currentTime;
            p[i].turnaroundTime = p[i].completionTime - p[i].arrivalTime;
            p[i].waitingTime = p[i].turnaroundTime - p[i].burstTime;
            p[i].remainingTime = 0; // Process finished
            
            isCompleted[i] = 1;
            completedCount++;
        }
    }
    
    ResultSummary res = print_and_calculate_metrics("SJF (Non-Preemptive)", p, n, chart, chartSize);
    
    free(isCompleted);
    free(p);
    free(chart);
    return res;
}
