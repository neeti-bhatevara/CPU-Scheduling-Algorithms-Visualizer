#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "process.h"
#include "fcfs.h"
#include "sjf.h"
#include "srtf.h"
#include "rr.h"
#include "priority_np.h"
#include "priority_p.h"
#include "hrrn.h"
#include "mlfq.h"

static void display_menu(void) {
    printf("\n==================================================\n");
    printf("          CPU SCHEDULING VISUALIZER\n");
    printf("==================================================\n");
    printf("1. Enter or replace process data\n");
    printf("2. Edit a process\n");
    printf("3. Remove a process\n");
    printf("4. Import processes from CSV\n");
    printf("5. Export processes to CSV\n");
    printf("6. Run FCFS\n");
    printf("7. Run SJF (Non-Preemptive)\n");
    printf("8. Run SRTF (Preemptive)\n");
    printf("9. Run Round Robin\n");
    printf("10. Run Priority (Non-Preemptive)\n");
    printf("11. Run Priority (Preemptive)\n");
    printf("12. Run HRRN (Non-Preemptive)\n");
    printf("13. Run MLFQ (Multilevel Feedback Queue)\n");
    printf("14. Run All Algorithms\n");
    printf("15. Export last comparison to CSV\n");
    printf("16. Exit\n");
    printf("Priority rule: a lower number means a higher priority.\n");
    printf("==================================================\n");
}

static void wait_for_enter(void) {
    char line[8];
    read_line("\nPress Enter to return to the menu...", line, sizeof(line));
}

static int find_process(const Process processes[], int n, int pid) {
    for (int i = 0; i < n; i++) {
        if (processes[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

static int edit_process(Process processes[], int n, int *changed) {
    *changed = 0;
    int pid;
    if (n == 0) {
        printf("No process data is loaded.\n");
        return 1;
    }
    display_processes(processes, n);
    if (!read_integer("PID to edit: ", 1, 2147483647, &pid)) {
        return 0;
    }
    int index = find_process(processes, n, pid);
    if (index < 0) {
        printf("No process has PID %d.\n", pid);
        return 1;
    }

    int arrival;
    int burst;
    int priority;
    if (!read_integer("New arrival time (0-1000000): ", 0, 1000000,
                      &arrival) ||
        !read_integer("New burst time (1-100000): ", 1, 100000,
                      &burst) ||
        !read_integer("New priority (1-1000000): ", 1, 1000000,
                      &priority)) {
        return 0;
    }

    int totalBurst = burst;
    for (int i = 0; i < n; i++) {
        if (i != index) {
            totalBurst += processes[i].burstTime;
        }
    }
    if (totalBurst > 100000) {
        printf("Edit cancelled: total burst time cannot exceed 100000.\n");
        return 1;
    }
    init_process(&processes[index], pid, arrival, burst, priority);
    printf("Process P%d updated.\n", pid);
    *changed = 1;
    return 1;
}

static int remove_process(Process processes[], int *count, int *changed) {
    *changed = 0;
    int pid;
    if (*count == 0) {
        printf("No process data is loaded.\n");
        return 1;
    }
    display_processes(processes, *count);
    if (!read_integer("PID to remove: ", 1, 2147483647, &pid)) {
        return 0;
    }
    int index = find_process(processes, *count, pid);
    if (index < 0) {
        printf("No process has PID %d.\n", pid);
        return 1;
    }
    for (int i = index; i < *count - 1; i++) {
        processes[i] = processes[i + 1];
    }
    (*count)--;
    printf("Process P%d removed. %d process(es) remain.\n", pid, *count);
    *changed = 1;
    return 1;
}

static int ask_path(const char *prompt, char *path, size_t pathSize) {
    for (;;) {
        if (!read_line(prompt, path, pathSize)) {
            return 0;
        }
        if (path[0] != '\0') {
            return 1;
        }
        printf("File path cannot be empty.\n");
    }
}

static int ask_quantum(int *quantum) {
    return read_integer("Round Robin time quantum (1-100000): ", 1, 100000,
                        quantum);
}

static void print_comparison(const ResultSummary results[], int count) {
    printf("\n================ ALGORITHM COMPARISON ================\n");
    printf("%-27s %10s %10s %10s %11s %12s %10s\n",
           "Algorithm", "Avg WT", "Avg TAT", "Avg RT", "CPU Util.",
           "Throughput", "Switches");
    for (int i = 0; i < count; i++) {
        printf("%-27s %10.2f %10.2f %10.2f %10.2f%% %12.4f %10d\n",
               results[i].algoName, results[i].avgWT, results[i].avgTAT,
               results[i].avgRT, results[i].cpuUtilization,
               results[i].throughput, results[i].contextSwitches);
    }
    printf("=======================================================\n");
}

static int match_algo(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int run_single_algo_json(const char *algo, int quantum, Process processes[], int count) {
    set_json_output_mode(1);
    if (match_algo(algo, "fcfs")) {
        execute_fcfs(processes, count);
    } else if (match_algo(algo, "sjf") || match_algo(algo, "sjf_np") || match_algo(algo, "sjf-np")) {
        execute_sjf_non_preemptive(processes, count);
    } else if (match_algo(algo, "srtf") || match_algo(algo, "sjf_p") || match_algo(algo, "sjf-p")) {
        execute_srtf(processes, count);
    } else if (match_algo(algo, "rr") || match_algo(algo, "round_robin") || match_algo(algo, "roundrobin")) {
        execute_rr(processes, count, quantum > 0 ? quantum : 2);
    } else if (match_algo(algo, "priority_np") || match_algo(algo, "priority-np") || match_algo(algo, "prioritynp")) {
        execute_priority_np(processes, count);
    } else if (match_algo(algo, "priority_p") || match_algo(algo, "priority-p") || match_algo(algo, "priorityp")) {
        execute_priority_p(processes, count);
    } else if (match_algo(algo, "hrrn")) {
        execute_hrrn(processes, count);
    } else if (match_algo(algo, "mlfq")) {
        execute_mlfq(processes, count);
    } else {
        fprintf(stderr, "Unknown algorithm: %s\n", algo);
        return 0;
    }
    return 1;
}

static void run_all_algos_json(int quantum, Process processes[], int count) {
    set_json_output_mode(1);
    printf("[\n");
    execute_fcfs(processes, count);
    printf(",\n");
    execute_sjf_non_preemptive(processes, count);
    printf(",\n");
    execute_srtf(processes, count);
    printf(",\n");
    execute_rr(processes, count, quantum > 0 ? quantum : 2);
    printf(",\n");
    execute_priority_np(processes, count);
    printf(",\n");
    execute_priority_p(processes, count);
    printf(",\n");
    execute_hrrn(processes, count);
    printf(",\n");
    execute_mlfq(processes, count);
    printf("\n]\n");
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--json") == 0 && argc >= 5) {
            const char *algo = argv[2];
            int quantum = atoi(argv[3]);
            const char *csvPath = argv[4];
            Process *loaded = NULL;
            int loadedCount = 0;
            char error[256];
            if (!import_processes_csv(csvPath, &loaded, &loadedCount, error, sizeof(error))) {
                fprintf(stderr, "Error loading CSV: %s\n", error);
                return 1;
            }
            int success = run_single_algo_json(algo, quantum, loaded, loadedCount);
            free(loaded);
            return success ? 0 : 1;
        } else if (strcmp(argv[1], "--json-all") == 0 && argc >= 4) {
            int quantum = atoi(argv[2]);
            const char *csvPath = argv[3];
            Process *loaded = NULL;
            int loadedCount = 0;
            char error[256];
            if (!import_processes_csv(csvPath, &loaded, &loadedCount, error, sizeof(error))) {
                fprintf(stderr, "Error loading CSV: %s\n", error);
                return 1;
            }
            run_all_algos_json(quantum, loaded, loadedCount);
            free(loaded);
            return 0;
        } else {
            fprintf(stderr, "Usage:\n  %s\n  %s --json <algo> <quantum> <csv_path>\n  %s --json-all <quantum> <csv_path>\n",
                    argv[0], argv[0], argv[0]);
            return 1;
        }
    }
    Process *processes = NULL;
    int processCount = 0;
    ResultSummary lastResults[8];
    int lastResultCount = 0;
    int running = 1;

    while (running) {
        int choice;
        display_menu();
        if (!read_integer("Choose an option (1-16): ", 1, 16, &choice)) {
            break;
        }

        switch (choice) {
            case 1: {
                int count;
                if (!read_integer("Number of processes (1-100): ", 1, 100,
                                  &count)) {
                    running = 0;
                    break;
                }
                Process *replacement = calloc((size_t)count,
                                              sizeof(*replacement));
                if (replacement == NULL) {
                    printf("Could not allocate process data.\n");
                    break;
                }
                if (!input_processes(replacement, count)) {
                    free(replacement);
                    running = 0;
                    break;
                }
                free(processes);
                processes = replacement;
                processCount = count;
                lastResultCount = 0;
                printf("\nProcess data saved:\n");
                display_processes(processes, processCount);
                break;
            }
            case 2: {
                int changed;
                if (!edit_process(processes, processCount, &changed)) {
                    running = 0;
                } else if (changed) {
                    lastResultCount = 0;
                }
                break;
            }
            case 3: {
                int changed;
                if (!remove_process(processes, &processCount, &changed)) {
                    running = 0;
                } else if (changed) {
                    lastResultCount = 0;
                }
                break;
            }
            case 4: {
                char path[512];
                char error[256];
                Process *loaded = NULL;
                int loadedCount = 0;
                if (!ask_path("CSV file path to import: ", path, sizeof(path))) {
                    running = 0;
                    break;
                }
                if (!import_processes_csv(path, &loaded, &loadedCount,
                                          error, sizeof(error))) {
                    printf("Import failed: %s\n", error);
                    break;
                }
                free(processes);
                processes = loaded;
                processCount = loadedCount;
                lastResultCount = 0;
                printf("Imported %d process(es) from %s.\n", processCount, path);
                display_processes(processes, processCount);
                break;
            }
            case 5: {
                char path[512];
                char error[256];
                if (processCount == 0) {
                    printf("No process data is loaded.\n");
                    break;
                }
                if (!ask_path("CSV file path to export: ", path, sizeof(path))) {
                    running = 0;
                    break;
                }
                if (export_processes_csv(path, processes, processCount,
                                         error, sizeof(error))) {
                    printf("Process data exported to %s.\n", path);
                } else {
                    printf("Export failed: %s\n", error);
                }
                break;
            }
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14: {
                if (processCount == 0) {
                    printf("No process data found. Enter or import process data first.\n");
                    break;
                }
                int quantum = 0;
                if ((choice == 9 || choice == 14) && !ask_quantum(&quantum)) {
                    running = 0;
                    break;
                }

                if (choice == 6) {
                    execute_fcfs(processes, processCount);
                } else if (choice == 7) {
                    execute_sjf_non_preemptive(processes, processCount);
                } else if (choice == 8) {
                    execute_srtf(processes, processCount);
                } else if (choice == 9) {
                    execute_rr(processes, processCount, quantum);
                } else if (choice == 10) {
                    execute_priority_np(processes, processCount);
                } else if (choice == 11) {
                    execute_priority_p(processes, processCount);
                } else if (choice == 12) {
                    execute_hrrn(processes, processCount);
                } else if (choice == 13) {
                    execute_mlfq(processes, processCount);
                } else {
                    lastResults[0] = execute_fcfs(processes, processCount);
                    lastResults[1] = execute_sjf_non_preemptive(processes, processCount);
                    lastResults[2] = execute_srtf(processes, processCount);
                    lastResults[3] = execute_rr(processes, processCount, quantum);
                    lastResults[4] = execute_priority_np(processes, processCount);
                    lastResults[5] = execute_priority_p(processes, processCount);
                    lastResults[6] = execute_hrrn(processes, processCount);
                    lastResults[7] = execute_mlfq(processes, processCount);
                    lastResultCount = 8;
                    print_comparison(lastResults, lastResultCount);
                }
                wait_for_enter();
                break;
            }
            case 15: {
                char path[512];
                char error[256];
                if (lastResultCount == 0) {
                    printf("Run all algorithms first to create a comparison.\n");
                    break;
                }
                if (!ask_path("Comparison CSV file path: ", path, sizeof(path))) {
                    running = 0;
                    break;
                }
                if (export_results_csv(path, lastResults, lastResultCount,
                                       error, sizeof(error))) {
                    printf("Comparison exported to %s.\n", path);
                } else {
                    printf("Export failed: %s\n", error);
                }
                break;
            }
            case 16:
                running = 0;
                break;
        }
    }

    free(processes);
    printf("\nExiting Simulator. Goodbye!\n");
    return 0;
}
