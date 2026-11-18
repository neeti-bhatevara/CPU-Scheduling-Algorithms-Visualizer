#include <stdio.h>
#include <stdlib.h>
#include "fcfs.h"

ResultSummary execute_fcfs(Process processes[], int n) {
    if (processes == NULL || n <= 0) {
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    // 1. Create a local copy so we don't modify the original array
    Process *p = (Process *)malloc(n * sizeof(Process));
    if (p == NULL) {
        printf("Memory allocation failed for FCFS copy!\n");
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    for (int i = 0; i < n; i++) {
        p[i] = processes[i];
    }
    
    // 2. Sort processes by arrivalTime (Bubble Sort for simplicity)
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (p[j].arrivalTime > p[j + 1].arrivalTime || 
               (p[j].arrivalTime == p[j + 1].arrivalTime && p[j].pid > p[j + 1].pid)) {
                Process temp = p[j];
                p[j] = p[j + 1];
                p[j + 1] = temp;
            }
        }
    }
    
    // Arrays for Gantt chart tracking
    // Max blocks could be 2*n (if every process has an idle gap before it)
    GanttBlock *chart = (GanttBlock *)malloc(2 * n * sizeof(GanttBlock));
    if (chart == NULL) {
        printf("Memory allocation failed for Gantt chart!\n");
        free(p);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    int chartSize = 0;
    
    int currentTime = 0;
    
    // 3. Simulate Execution
    for (int i = 0; i < n; i++) {
        if (currentTime < p[i].arrivalTime) {
            // CPU is idle
            chart[chartSize].pid = -1;
            chart[chartSize].startTime = currentTime;
            chart[chartSize].endTime = p[i].arrivalTime;
            chartSize++;
            currentTime = p[i].arrivalTime;
        }
        
        // Process Execution
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
    }
    
    ResultSummary res = print_and_calculate_metrics("FCFS", p, n, chart, chartSize);
    
    free(p);
    free(chart);
    return res;
}
