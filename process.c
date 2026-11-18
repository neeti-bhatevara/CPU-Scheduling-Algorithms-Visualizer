#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"

void init_process(Process *p, int id, int arrival, int burst, int priority) {
    p->pid = id;
    p->arrivalTime = arrival;
    p->burstTime = burst;
    p->remainingTime = burst;
    p->priority = priority;
    p->completionTime = 0;
    p->turnaroundTime = 0;
    p->waitingTime = 0;
    p->responseTime = 0;
    p->firstStartTime = -1;
}

int read_line(const char *prompt, char *buffer, size_t bufferSize) {
    for (;;) {
        if (prompt != NULL) {
            printf("%s", prompt);
        }
        if (fgets(buffer, (int)bufferSize, stdin) == NULL) {
            return 0;
        }

        size_t length = strlen(buffer);
        if (length > 0 && buffer[length - 1] == '\n') {
            buffer[length - 1] = '\0';
            if (length > 1 && buffer[length - 2] == '\r') {
                buffer[length - 2] = '\0';
            }
            return 1;
        }
        if (feof(stdin)) {
            return 1;
        }
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
        printf("Input is too long. Please try again.\n");
    }
}

int read_integer(const char *prompt, int minimum, int maximum, int *value) {
    char line[128];
    for (;;) {
        char *end = NULL;
        long parsed;

        if (!read_line(prompt, line, sizeof(line))) {
            return 0;
        }

        errno = 0;
        parsed = strtol(line, &end, 10);
        while (end != NULL && isspace((unsigned char)*end)) {
            end++;
        }
        if (line[0] != '\0' && end != line && end != NULL && *end == '\0' &&
            errno != ERANGE && parsed >= minimum && parsed <= maximum) {
            *value = (int)parsed;
            return 1;
        }
        printf("Invalid input. Enter a whole number from %d to %d.\n",
               minimum, maximum);
    }
}

int input_processes(Process processes[], int n) {
    int totalBurst = 0;

    for (int i = 0; i < n; i++) {
        int arrival;
        int burst;
        int priority;

        printf("\n--- Process P%d ---\n", i + 1);
        if (!read_integer("Arrival time (0-1000000): ", 0, 1000000, &arrival)) {
            return 0;
        }
        for (;;) {
            if (!read_integer("Burst time (1-100000): ", 1, 100000, &burst)) {
                return 0;
            }
            if (totalBurst + burst <= 100000) {
                break;
            }
            printf("Total burst time cannot exceed 100000 time units.\n");
        }
        totalBurst += burst;
        if (!read_integer("Priority (1-1000000; lower number is higher priority): ",
                          1, 1000000, &priority)) {
            return 0;
        }
        init_process(&processes[i], i + 1, arrival, burst, priority);
    }
    return 1;
}

void display_processes(Process processes[], int n) {
    printf("\n%-6s %-9s %-7s %-9s %-10s %-11s %-11s %-8s %-9s %-11s\n",
           "PID", "Arrival", "Burst", "Priority", "Remaining", "Completion",
           "Turnaround", "Waiting", "Response", "First start");
    for (int i = 0; i < n; i++) {
        Process p = processes[i];
        printf("%-6d %-9d %-7d %-9d %-10d %-11d %-11d %-8d %-9d %-11d\n",
               p.pid, p.arrivalTime, p.burstTime, p.priority, p.remainingTime,
               p.completionTime, p.turnaroundTime, p.waitingTime,
               p.responseTime, p.firstStartTime);
    }
}

static int g_json_output_mode = 0;

void set_json_output_mode(int enable) {
    g_json_output_mode = enable;
}

int get_json_output_mode(void) {
    return g_json_output_mode;
}

ResultSummary print_and_calculate_metrics(const char *algoName,
                                          Process processes[], int n,
                                          GanttBlock chart[], int chartSize) {
    ResultSummary summary = {0};
    snprintf(summary.algoName, sizeof(summary.algoName), "%s", algoName);
    if (n <= 0 || chartSize <= 0) {
        return summary;
    }

    double totalWaiting = 0;
    double totalTurnaround = 0;
    double totalResponse = 0;
    for (int i = 0; i < n; i++) {
        totalWaiting += processes[i].waitingTime;
        totalTurnaround += processes[i].turnaroundTime;
        totalResponse += processes[i].responseTime;
    }
    summary.avgWT = (float)(totalWaiting / n);
    summary.avgTAT = (float)(totalTurnaround / n);
    summary.avgRT = (float)(totalResponse / n);

    int totalTime = chart[chartSize - 1].endTime - chart[0].startTime;
    int idleTime = 0;
    for (int i = 0; i < chartSize; i++) {
        if (chart[i].pid < 0) {
            idleTime += chart[i].endTime - chart[i].startTime;
        }
    }
    int busyTime = totalTime - idleTime;
    if (totalTime > 0) {
        summary.cpuUtilization = (float)busyTime * 100.0f / totalTime;
        summary.throughput = (float)n / totalTime;
    }

    int previousPid = -1;
    for (int i = 0; i < chartSize; i++) {
        if (chart[i].pid >= 0) {
            if (previousPid >= 0 && previousPid != chart[i].pid) {
                summary.contextSwitches++;
            }
            previousPid = chart[i].pid;
        }
    }

    if (g_json_output_mode) {
        printf("{\n");
        printf("  \"algorithm\": \"%s\",\n", algoName);
        printf("  \"metrics\": {\n");
        printf("    \"avgWT\": %.2f,\n", summary.avgWT);
        printf("    \"avgTAT\": %.2f,\n", summary.avgTAT);
        printf("    \"avgRT\": %.2f,\n", summary.avgRT);
        printf("    \"cpuUtilization\": %.2f,\n", summary.cpuUtilization);
        printf("    \"throughput\": %.4f,\n", summary.throughput);
        printf("    \"contextSwitches\": %d\n", summary.contextSwitches);
        printf("  },\n");
        printf("  \"chart\": [\n");
        for (int i = 0; i < chartSize; i++) {
            printf("    { \"pid\": %d, \"startTime\": %d, \"endTime\": %d }%s\n",
                   chart[i].pid, chart[i].startTime, chart[i].endTime,
                   (i == chartSize - 1) ? "" : ",");
        }
        printf("  ],\n");
        printf("  \"processes\": [\n");
        for (int i = 0; i < n; i++) {
            printf("    { \"pid\": %d, \"arrivalTime\": %d, \"burstTime\": %d, \"priority\": %d, \"completionTime\": %d, \"turnaroundTime\": %d, \"waitingTime\": %d, \"responseTime\": %d }%s\n",
                   processes[i].pid, processes[i].arrivalTime, processes[i].burstTime, processes[i].priority,
                   processes[i].completionTime, processes[i].turnaroundTime, processes[i].waitingTime, processes[i].responseTime,
                   (i == n - 1) ? "" : ",");
        }
        printf("  ]\n");
        printf("}\n");
        return summary;
    }

    printf("\n--- %s Gantt Chart ---\n", algoName);
    for (int rowStart = 0; rowStart < chartSize; rowStart += 10) {
        int rowEnd = rowStart + 10 < chartSize ? rowStart + 10 : chartSize;
        for (int i = rowStart; i < rowEnd; i++) {
            printf("+---------");
        }
        printf("+\n|");
        for (int i = rowStart; i < rowEnd; i++) {
            char label[16];
            if (chart[i].pid < 0) {
                snprintf(label, sizeof(label), "IDLE");
            } else {
                snprintf(label, sizeof(label), "P%d", chart[i].pid);
            }
            printf(" %-7s |", label);
        }
        printf("\n");
        for (int i = rowStart; i < rowEnd; i++) {
            printf("+---------");
        }
        printf("+\n");
        printf("%-10d", chart[rowStart].startTime);
        for (int i = rowStart; i < rowEnd; i++) {
            printf("%-10d", chart[i].endTime);
        }
        printf("\n\n");
    }

    printf("--- %s Process Results ---\n", algoName);
    display_processes(processes, n);

    printf("\nAverage waiting time: %.2f\n", summary.avgWT);
    printf("Average turnaround time: %.2f\n", summary.avgTAT);
    printf("Average response time: %.2f\n", summary.avgRT);
    printf("CPU utilization: %.2f%%\n", summary.cpuUtilization);
    printf("Throughput: %.4f processes/time unit\n", summary.throughput);
    printf("Process-to-process context switches: %d\n\n", summary.contextSwitches);
    return summary;
}

static int valid_workload(const Process *processes, int count) {
    int totalBurst = 0;
    if (count < 1 || count > 100) {
        return 0;
    }
    for (int i = 0; i < count; i++) {
        if (processes[i].pid <= 0 || processes[i].arrivalTime < 0 ||
            processes[i].arrivalTime > 1000000 || processes[i].burstTime <= 0 ||
            processes[i].priority <= 0 || processes[i].priority > 1000000 ||
            totalBurst > 100000 - processes[i].burstTime) {
            return 0;
        }
        totalBurst += processes[i].burstTime;
        for (int j = 0; j < i; j++) {
            if (processes[i].pid == processes[j].pid) {
                return 0;
            }
        }
    }
    return 1;
}

int import_processes_csv(const char *path, Process **processes, int *count,
                         char *error, size_t errorSize) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        snprintf(error, errorSize, "Could not open '%s'.", path);
        return 0;
    }

    Process *loaded = NULL;
    int loadedCount = 0;
    int capacity = 0;
    char line[256];
    int lineNumber = 0;
    int success = 1;

    if (fgets(line, sizeof(line), file) == NULL) {
        snprintf(error, errorSize, "The CSV file is empty.");
        success = 0;
    } else {
        lineNumber++;
        if (strcmp(line, "pid,arrival,burst,priority\n") != 0 &&
            strcmp(line, "pid,arrival,burst,priority\r\n") != 0 &&
            strcmp(line, "pid,arrival,burst,priority") != 0) {
            snprintf(error, errorSize,
                     "Expected header: pid,arrival,burst,priority");
            success = 0;
        }
    }

    while (success && fgets(line, sizeof(line), file) != NULL) {
        lineNumber++;
        size_t lineLength = strlen(line);
        if (lineLength > 0 && line[lineLength - 1] != '\n' && !feof(file)) {
            snprintf(error, errorSize, "CSV line %d is too long.", lineNumber);
            success = 0;
            break;
        }
        int pid;
        int arrival;
        int burst;
        int priority;
        char extra;
        int fields = sscanf(line, " %d , %d , %d , %d %c",
                            &pid, &arrival, &burst, &priority, &extra);
        if (fields != 4 || pid <= 0 || arrival < 0 || arrival > 1000000 ||
            burst <= 0 || burst > 100000 || priority <= 0 ||
            priority > 1000000 || loadedCount >= 100) {
            snprintf(error, errorSize, "Invalid process data on CSV line %d.",
                     lineNumber);
            success = 0;
            break;
        }

        if (loadedCount == capacity) {
            int nextCapacity = capacity == 0 ? 8 : capacity * 2;
            Process *resized = realloc(loaded,
                                       (size_t)nextCapacity * sizeof(*loaded));
            if (resized == NULL) {
                snprintf(error, errorSize, "Not enough memory to load the CSV.");
                success = 0;
                break;
            }
            loaded = resized;
            capacity = nextCapacity;
        }
        init_process(&loaded[loadedCount], pid, arrival, burst, priority);
        loadedCount++;
    }

    if (ferror(file)) {
        snprintf(error, errorSize, "Error reading '%s'.", path);
        success = 0;
    }
    if (fclose(file) != 0 && success) {
        snprintf(error, errorSize, "Error closing '%s'.", path);
        success = 0;
    }
    if (success && !valid_workload(loaded, loadedCount)) {
        snprintf(error, errorSize,
                 "CSV must contain 1-100 unique processes; total burst time must not exceed 100000.");
        success = 0;
    }
    if (!success) {
        free(loaded);
        return 0;
    }

    *processes = loaded;
    *count = loadedCount;
    return 1;
}

int export_processes_csv(const char *path, const Process processes[], int count,
                         char *error, size_t errorSize) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        snprintf(error, errorSize, "Could not open '%s' for writing.", path);
        return 0;
    }

    int success = fprintf(file, "pid,arrival,burst,priority\n") >= 0;
    for (int i = 0; success && i < count; i++) {
        success = fprintf(file, "%d,%d,%d,%d\n", processes[i].pid,
                          processes[i].arrivalTime, processes[i].burstTime,
                          processes[i].priority) >= 0;
    }
    if (fclose(file) != 0) {
        success = 0;
    }
    if (!success) {
        snprintf(error, errorSize, "Could not write all process data to '%s'.",
                 path);
    }
    return success;
}

int export_results_csv(const char *path, const ResultSummary results[],
                       int count, char *error, size_t errorSize) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        snprintf(error, errorSize, "Could not open '%s' for writing.", path);
        return 0;
    }

    int success = fprintf(file,
                          "algorithm,average_waiting,average_turnaround,average_response,cpu_utilization_percent,throughput,context_switches\n") >= 0;
    for (int i = 0; success && i < count; i++) {
        success = fprintf(file, "\"%s\",%.2f,%.2f,%.2f,%.2f,%.4f,%d\n",
                          results[i].algoName, results[i].avgWT,
                          results[i].avgTAT, results[i].avgRT,
                          results[i].cpuUtilization, results[i].throughput,
                          results[i].contextSwitches) >= 0;
    }
    if (fclose(file) != 0) {
        success = 0;
    }
    if (!success) {
        snprintf(error, errorSize, "Could not write all results to '%s'.", path);
    }
    return success;
}
