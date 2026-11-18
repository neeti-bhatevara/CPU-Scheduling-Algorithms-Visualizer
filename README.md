# CPU Scheduling & Visualization System

An operating systems simulation and visualization platform built with a high-performance **native C11 scheduling engine**, a **zero-dependency Python HTTP backend**, and a **modern responsive web dashboard**.

Designed for computer science academic demonstrations, viva examinations, and operating system scheduling coursework.

---

## 1. Project Overview

This project provides both an interactive console application and a modern graphical dashboard to simulate, visualize, and benchmark CPU scheduling algorithms. The frontend communicates with the native C application via a lightweight local API, ensuring that **100% of scheduling decisions, Gantt timeline intervals, and performance metrics are calculated directly by the genuine C11 engine**.

### Key Highlights
- **8 Implemented Scheduling Algorithms** including preemptive, non-preemptive, priority, aging, and multi-queue feedback policies.
- **Proportional Timeline Gantt Chart** displaying exact burst intervals, PID color badges, and explicit CPU idle periods.
- **Full Comparative Benchmarking** comparing all 8 algorithms simultaneously with bar charts and detailed tables.
- **One-Click Startup** via `start_project.bat` with automated environment checks, C recompilation, backend startup, and browser launch.
- **Dual Interface Modes**: Modern web dashboard (GUI) and interactive terminal console application.

---

## 2. All 8 Implemented Scheduling Algorithms

| # | Algorithm | Type | Description & Policy Rules | Tie-Breaker |
|---|:---|:---:|:---|:---|
| 1 | **First Come, First Served (FCFS)** | Non-Preemptive | Allocates CPU strictly according to arrival time in FIFO order. Illustrates the *convoy effect*. | Earlier Arrival &rarr; Smaller PID |
| 2 | **Shortest Job First (SJF)** | Non-Preemptive | Selects the ready job with the shortest CPU burst time. Minimizes average waiting time among non-preemptive policies. | Earlier Arrival &rarr; Smaller PID |
| 3 | **Shortest Remaining Time First (SRTF)** | Preemptive | Preemptive SJF. Evaluates ready jobs at each unit of time. If a newly arriving process has a smaller remaining time than the executing process, preemption occurs immediately. | Earlier Arrival &rarr; Smaller PID |
| 4 | **Round Robin (RR)** | Preemptive | Circular FIFO time-sharing queue. Allocates a fixed time slice (*quantum*) per process. Arriving processes take precedence over the uncompleted requeued process at time slice boundaries. | Earlier Arrival &rarr; Smaller PID |
| 5 | **Priority (Non-Preemptive)** | Non-Preemptive | Schedules the process with the highest priority (smaller numeric value = higher priority). Executes uninterrupted until completion. | Earlier Arrival &rarr; Smaller PID |
| 6 | **Priority (Preemptive)** | Preemptive | Immediately preempts the CPU if a newly arriving process has a strictly higher priority (lower numeric value) than the running process. | Earlier Arrival &rarr; Smaller PID |
| 7 | **Highest Response Ratio Next (HRRN)** | Non-Preemptive | Prevents starvation by dynamically computing response ratio: $R = \frac{W + S}{S} = 1 + \frac{W}{S}$, where $W$ is elapsed waiting time and $S$ is burst time. | Earlier Arrival &rarr; Smaller PID |
| 8 | **Multilevel Feedback Queue (MLFQ)** | Preemptive / Feedback | 3 distinct priority queues: <br>&bull; **Queue 1:** Round Robin ($Q = 2\text{ ms}$)<br>&bull; **Queue 2:** Round Robin ($Q = 4\text{ ms}$)<br>&bull; **Queue 3:** First Come, First Served (FCFS)<br>New jobs enter Queue 1. Jobs that exhaust their quantum demote to the next lower tier. Higher queues strictly preempt lower queues. | Earlier Arrival &rarr; Smaller PID |

### Mathematical Evaluation Formulas
- **Completion Time ($CT$):** Time instant at which the process finishes execution.
- **Turnaround Time ($TAT$):** $TAT = CT - AT$
- **Waiting Time ($WT$):** $WT = TAT - BT$
- **Response Time ($RT$):** $RT = \text{First Start Time} - AT$
- **CPU Utilization:** $\frac{\text{Total Time} - \text{Idle Time}}{\text{Total Time}} \times 100\%$
- **Throughput:** $\frac{\text{Completed Processes}}{\text{Total Elapsed Time Interval}}$
- **Context Switches:** Transitions between distinct processes (idle intervals excluded).

---

## 3. Technologies Used

- **C11 Native Engine:** High-performance, memory-safe C scheduling logic compiled with GCC (`-std=c11 -Wall -Wextra`).
- **Python 3 Standard Library:** Zero-dependency HTTP backend server (`http.server`, `socketserver`, `subprocess`, `json`, `tempfile`). No Flask, FastAPI, or external pip packages required.
- **Web Frontend:** Pure HTML5, modern Vanilla CSS3 (custom CSS custom properties, glassmorphism, responsive grid), and ES6+ JavaScript.
- **Windows Automation:** Batch scripting (`start_project.bat`) with automated dependency detection and compiler integration.

---

## 4. Project Folder Structure

```
C:\cpu sched[uling and visualise final\
├── start_project.bat          # One-click Windows startup batch script
├── README.md                  # Comprehensive project documentation
├── index.html                 # Root redirect to web dashboard
│
├── backend/                   # Python HTTP backend
│   ├── server.py              # Zero-dependency API server & static file host
│   └── test_api.py            # Automated end-to-end API test suite
│
├── frontend/                  # Responsive web GUI dashboard
│   ├── index.html             # Dashboard structure & OS concept reference
│   ├── style.css              # Custom dark navy theme & responsive layout
│   └── app.js                 # API communication & dynamic visualization
│
├── workloads/                 # Sample simulation workloads
│   └── demo_workload.csv      # Recommended 5-process viva demonstration workload
│
├── build/                     # Compiled executables & CSV outputs
│   ├── cpu-scheduler.exe      # Compiled C11 scheduling binary
│   ├── test_scheduler.exe     # Compiled C regression test suite
│   └── demo_workload.csv      # Local build copy of demonstration workload
│
├── tests/                     # C unit and regression tests
│   └── test_scheduler.c       # Comprehensive C test suite
│
├── main.c                     # Interactive console menu & CLI JSON batch engine
├── process.c / process.h      # Process models, CSV handling, Gantt & JSON formatting
├── fcfs.c / fcfs.h            # First Come, First Served policy
├── sjf.c / sjf.h              # Shortest Job First policy
├── srtf.c / srtf.h            # Shortest Remaining Time First policy
├── rr.c / rr.h                # Round Robin policy
├── priority_np.c / .h         # Non-preemptive priority policy
├── priority_p.c / .h          # Preemptive priority policy
├── hrrn.c / hrrn.h            # Highest Response Ratio Next policy
└── mlfq.c / mlfq.h            # Multilevel Feedback Queue (3 priority tiers)
```

---

## 5. Installation & Prerequisites

- **Operating System:** Windows 10 or 11 (64-bit).
- **Python:** Python 3.8 or higher (included in PATH).
- **C Compiler:** GCC (MinGW-w64) supporting C11 (included in PATH).
- **Web Browser:** Google Chrome, Microsoft Edge, or Mozilla Firefox.

---

## 6. How to Launch the Application

### Method 1: One-Click Startup (Recommended for Viva)
Double-click [`start_project.bat`](file:///c:/cpu%20sched[uling%20and%20visualise%20final/start_project.bat) in the project root folder.

The batch script will automatically:
1. Verify Python is installed.
2. Check if the C engine needs recompilation (recompiles automatically if source files were modified).
3. Check if the backend is already running (prevents duplicate instances).
4. Launch the Python HTTP backend server minimized in the background.
5. Open your default web browser directly to:
   ```
   http://localhost:8080/frontend/index.html
   ```

### Method 2: Manual Terminal Launch
In PowerShell (note: use `-LiteralPath` because of the square bracket in the directory path):
```powershell
Set-Location -LiteralPath 'C:\cpu sched[uling and visualise final'
python backend/server.py 8080
```
Then navigate to `http://localhost:8080/frontend/index.html` in your web browser.

### Method 3: Interactive Console C Application
To run the traditional C command-line menu:
```powershell
Set-Location -LiteralPath 'C:\cpu sched[uling and visualise final'
.\build\cpu-scheduler.exe
```

---

## 7. How the Frontend, Backend, and C Engine Communicate

```
+------------------------------------------------------------------------+
|                  Web Browser Frontend (frontend/app.js)                |
|  - Manages workload in browser memory                                  |
|  - Dispatches JSON payloads to REST API endpoints                      |
|  - Renders proportional Gantt charts, metrics, and benchmark bars     |
+------------------------------------------------------------------------+
                                    |
                           HTTP POST /api/simulate
                           HTTP POST /api/compare
                                    v
+------------------------------------------------------------------------+
|               Python Backend Server (backend/server.py)                |
|  - Standard library `http.server` listening on port 8080               |
|  - Validates process constraints (PIDs, bounds, cumulative limits)     |
|  - Generates temporary sanitized workload CSV                          |
|  - Invokes C executable safely via `subprocess.run(..., shell=False)`  |
+------------------------------------------------------------------------+
                                    |
                        CLI Batch Arguments:
      `build/cpu-scheduler.exe --json <algo> <quantum> <temp_csv>`
      `build/cpu-scheduler.exe --json-all <quantum> <temp_csv>`
                                    v
+------------------------------------------------------------------------+
|              Native C11 Scheduler (build/cpu-scheduler.exe)            |
|  - Parses CSV into `Process` struct arrays                             |
|  - Executes pure C scheduling algorithms                              |
|  - Computes exact metrics and Gantt execution segments                 |
|  - Outputs clean, structured JSON to stdout                            |
+------------------------------------------------------------------------+
```

---

## 8. Demonstration Workload & Viva Discussion Guide

The system includes a pre-configured **5-process workload** specifically structured to illustrate the core differences and edge cases of CPU scheduling theory:

### Workload Parameters ([`workloads/demo_workload.csv`](file:///c:/cpu%20sched[uling%20and%20visualise%20final/workloads/demo_workload.csv))
| Process | Arrival Time | Burst Time | Priority | Expected Role / Behavior |
|:---:|:---:|:---:|:---:|:---|
| **P1** | 2 ms | 5 ms | 3 | Starts after initial idle gap. Demotes across MLFQ queues. |
| **P2** | 3 ms | 2 ms | 1 | Highest priority & short burst. Preempts P1 in SRTF/Priority(P). |
| **P3** | 5 ms | 8 ms | 5 | Long compute-bound batch task. Exercises Q1 &rarr; Q2 &rarr; Q3 demotion in MLFQ. |
| **P4** | 6 ms | 1 ms | 2 | Short interactive job. Highlights convoy effect in FCFS vs instant finish in SJF/MLFQ. |
| **P5** | 9 ms | 4 ms | 4 | Medium arrival. Exercises ready-queue interleaving. |

### Key Viva Points to Show Your Professor
1. **CPU Idle Period Display:**
   - Because P1 arrives at $t=2$, the CPU is idle from $t=0$ to $t=2\text{ ms}$.
   - The Gantt chart visualizer explicitly highlights this `IDLE` slot with a distinct striped bar, and CPU Utilization is accurately computed as **90.91%** ($\frac{20}{22}\times 100\%$).
2. **Convoy Effect in FCFS:**
   - In FCFS, short interactive process P4 (burst 1 ms) arrives at $t=6$ but is blocked behind long process P3 (burst 8 ms), waiting until $t=17$ to execute (Waiting Time = 11 ms).
3. **Preemption in SRTF & Priority (P):**
   - At $t=3$, P2 arrives with remaining burst 2 ms (less than P1's remaining 4 ms) and Priority 1 (higher than P1's Priority 3). Both SRTF and Priority (P) immediately preempt P1.
4. **Adaptive Multilevel Feedback Queue (MLFQ):**
   - Shows all 3 tiers in action: P4 finishes in Queue 1 ($q=2$), P1 finishes in Queue 2 ($q=4$), and long job P3 demotes down to Queue 3 (FCFS). Demonstrates how MLFQ achieves the lowest average response time (**0.80 ms**) without knowing burst times in advance.
5. **Comparative Benchmark Bar Chart:**
   - Clicking **"Compare All 8 Algorithms"** runs all policies through the C engine in parallel and flags the winning policy with an emerald **"Optimal"** badge.

---

## 9. Automated Testing & Verification

### Running C Engine Regression Tests
Verifies internal calculations, memory safety, and scheduling accuracy across all policies:
```powershell
Set-Location -LiteralPath 'C:\cpu sched[uling and visualise final'
.\build\test_scheduler.exe
```

### Running Backend API Integration Tests
Tests `/api/health`, `/api/simulate`, `/api/compare`, error validation, and idle periods:
```powershell
Set-Location -LiteralPath 'C:\cpu sched[uling and visualise final'
python backend/test_api.py
```
Both test suites pass with **0 errors and 0 warnings**.
