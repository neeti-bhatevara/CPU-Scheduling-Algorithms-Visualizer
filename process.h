#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>

typedef struct {
    int pid;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int priority;
    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
    int firstStartTime; // -1 indicates not started yet
} Process;

typedef struct {
    int pid; // -1 for IDLE
    int startTime;
    int endTime;
} GanttBlock;

typedef struct {
    char algoName[64];
    float avgWT;
    float avgTAT;
    float avgRT;
    float cpuUtilization;
    float throughput;
    int contextSwitches;
} ResultSummary;

// Function prototypes
void init_process(Process *p, int id, int arrival, int burst, int priority);
int read_line(const char *prompt, char *buffer, size_t bufferSize);
int read_integer(const char *prompt, int minimum, int maximum, int *value);
int input_processes(Process processes[], int n);
void display_processes(Process processes[], int n);
ResultSummary print_and_calculate_metrics(const char* algoName, Process processes[], int n, GanttBlock chart[], int chartSize);
int import_processes_csv(const char *path, Process **processes, int *count, char *error, size_t errorSize);
int export_processes_csv(const char *path, const Process processes[], int count, char *error, size_t errorSize);
int export_results_csv(const char *path, const ResultSummary results[], int count, char *error, size_t errorSize);
void set_json_output_mode(int enable);
int get_json_output_mode(void);

#endif // PROCESS_H
