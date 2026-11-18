#include <stdio.h>
#include <stdlib.h>
#include "priority_p.h"

static int ensure_chart_capacity(GanttBlock **chartPtr, int *capacityPtr, int size) {
    if (size >= *capacityPtr) {
        if (*capacityPtr > 10000000) {
            return 0;
        }
        int newCapacity = (*capacityPtr) * 2;
        GanttBlock *temp = (GanttBlock *)realloc(*chartPtr, newCapacity * sizeof(GanttBlock));
        if (temp == NULL) {
            return 0;
        }
        *chartPtr = temp;
        *capacityPtr = newCapacity;
    }
    return 1;
}

ResultSummary execute_priority_p(Process processes[], int n) {
    if (processes == NULL || n <= 0) {
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    Process *p = (Process *)malloc(n * sizeof(Process));
    if (p == NULL) {
        printf("Memory allocation failed for Priority (P) copy!\n");
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    for (int i = 0; i < n; i++) p[i] = processes[i];

    int chartCapacity = 1000;
    GanttBlock *chart = (GanttBlock *)malloc(chartCapacity * sizeof(GanttBlock));
    if (chart == NULL) {
        printf("Memory allocation failed for Gantt chart!\n");
        free(p);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    int chartSize = 0;
    
    int completedCount = 0;
    int currentTime = 0;
    int prevPid = -2;
    
    while (completedCount < n) {
        int selectedProcess = -1;
        int minPriority = 999999999;
        
        for (int i = 0; i < n; i++) {
            if (p[i].arrivalTime <= currentTime && p[i].remainingTime > 0) {
                if (p[i].priority < minPriority) {
                    minPriority = p[i].priority;
                    selectedProcess = i;
                } else if (p[i].priority == minPriority) {
                    if (p[i].arrivalTime < p[selectedProcess].arrivalTime) {
                        selectedProcess = i;
                    } else if (p[i].arrivalTime == p[selectedProcess].arrivalTime) {
                        if (p[i].pid < p[selectedProcess].pid) {
                            selectedProcess = i;
                        }
                    }
                }
            }
        }
        
        if (selectedProcess == -1) {
            int nextArrival = 999999999;
            for(int i = 0; i < n; i++) {
                if(p[i].remainingTime > 0 && p[i].arrivalTime < nextArrival) {
                    nextArrival = p[i].arrivalTime;
                }
            }
            if (nextArrival == 999999999) {
                free(p);
                free(chart);
                return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
            }
            if(prevPid != -1) {
                if (!ensure_chart_capacity(&chart, &chartCapacity, chartSize)) {
                    printf("Memory reallocation failed for Gantt chart!\n");
                    free(p);
                    free(chart);
                    return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
                }
                chart[chartSize].pid = -1;
                chart[chartSize].startTime = currentTime;
                chart[chartSize].endTime = nextArrival;
                chartSize++;
                prevPid = -1;
            } else {
                chart[chartSize - 1].endTime = nextArrival;
            }
            currentTime = nextArrival;
        } else {
            int i = selectedProcess;
            if (p[i].firstStartTime == -1) {
                p[i].firstStartTime = currentTime;
                p[i].responseTime = p[i].firstStartTime - p[i].arrivalTime;
            }
            
            if (prevPid != p[i].pid) {
                if (!ensure_chart_capacity(&chart, &chartCapacity, chartSize)) {
                    printf("Memory reallocation failed for Gantt chart!\n");
                    free(p);
                    free(chart);
                    return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
                }
                chart[chartSize].pid = p[i].pid;
                chart[chartSize].startTime = currentTime;
                chart[chartSize].endTime = currentTime + 1;
                chartSize++;
                prevPid = p[i].pid;
            } else {
                chart[chartSize - 1].endTime = currentTime + 1;
            }
            
            p[i].remainingTime--;
            currentTime++;
            
            if (p[i].remainingTime == 0) {
                p[i].completionTime = currentTime;
                p[i].turnaroundTime = p[i].completionTime - p[i].arrivalTime;
                p[i].waitingTime = p[i].turnaroundTime - p[i].burstTime;
                completedCount++;
            }
        }
    }
    
    ResultSummary res = print_and_calculate_metrics("Priority (Preemptive)", p, n, chart, chartSize);
    
    free(p);
    free(chart);
    return res;
}
