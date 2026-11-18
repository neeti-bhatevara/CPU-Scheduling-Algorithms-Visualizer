/**
 * CPU Scheduling Simulator - Clean Single-Page Frontend Engine
 * Communicates directly with native C11 scheduling engine via Python API.
 */

// ==========================================================================
// 1. Workload Presets & State
// ==========================================================================

const DEMO_WORKLOAD = [
    { pid: 1, arrivalTime: 2, burstTime: 5, priority: 3 },
    { pid: 2, arrivalTime: 3, burstTime: 2, priority: 1 },
    { pid: 3, arrivalTime: 5, burstTime: 8, priority: 5 },
    { pid: 4, arrivalTime: 6, burstTime: 1, priority: 2 },
    { pid: 5, arrivalTime: 9, burstTime: 4, priority: 4 }
];

const STANDARD_WORKLOAD = [
    { pid: 1, arrivalTime: 0, burstTime: 5, priority: 2 },
    { pid: 2, arrivalTime: 1, burstTime: 3, priority: 1 },
    { pid: 3, arrivalTime: 2, burstTime: 1, priority: 3 }
];

// Active workload in memory (default to 5-process demonstration workload)
let processList = JSON.parse(JSON.stringify(DEMO_WORKLOAD));

let editingPid = null;
let currentAlgorithm = 'MLFQ';
let rrQuantum = 2;
let activeComparisonMetric = 'wt';

// Real-time results returned by native C engine
let currentSimulationData = null;
let comparisonData = null;

// Determine backend API host dynamically
const API_BASE = (window.location.protocol.startsWith('http'))
    ? window.location.origin
    : 'http://localhost:8080';

// Mapping UI algorithm keys to backend/C arguments
const ALGO_MAP = {
    'FCFS': 'fcfs',
    'SJF': 'sjf',
    'SRTF': 'srtf',
    'RR': 'rr',
    'PriorityNP': 'priority_np',
    'PriorityP': 'priority_p',
    'HRRN': 'hrrn',
    'MLFQ': 'mlfq'
};

const ALGO_TITLES = {
    'FCFS': 'First Come, First Served (FCFS)',
    'SJF': 'Shortest Job First (SJF, Non-Preemptive)',
    'SRTF': 'Shortest Remaining Time First (SRTF, Preemptive)',
    'RR': 'Round Robin (RR, Preemptive)',
    'PriorityNP': 'Priority Scheduling (Non-Preemptive)',
    'PriorityP': 'Priority Scheduling (Preemptive)',
    'HRRN': 'Highest Response Ratio Next (HRRN, Non-Preemptive)',
    'MLFQ': 'Multilevel Feedback Queue (MLFQ: Q1=2, Q2=4, Q3=FCFS)'
};

// ==========================================================================
// 2. DOM Initialization
// ==========================================================================

document.addEventListener('DOMContentLoaded', () => {
    initControls();
    initProcessManager();
    initSimulator();
    initComparisonChart();

    // Initial render
    renderProcessTable();

    // Health check & initial simulation execution
    checkBackendHealth();
    runSimulation(currentAlgorithm, rrQuantum);
});

// ==========================================================================
// 3. Navigation & Actions
// ==========================================================================

function initControls() {
    // Preset buttons
    const loadDemoBtn = document.getElementById('loadDemoWorkloadBtn');
    if (loadDemoBtn) {
        loadDemoBtn.addEventListener('click', () => {
            loadDemoPreset();
        });
    }

    const quickSampleBtn = document.getElementById('quickSampleBtn');
    if (quickSampleBtn) {
        quickSampleBtn.addEventListener('click', () => {
            resetDefaultProcesses();
            showToast('Standard 3-process benchmark loaded.', 'info');
            runSimulation(currentAlgorithm, rrQuantum);
        });
    }

    const clearAllBtn = document.getElementById('clearAllProcessesBtn');
    if (clearAllBtn) {
        clearAllBtn.addEventListener('click', () => {
            if (confirm('Clear all processes from current workload?')) {
                processList = [];
                renderProcessTable();
                showToast('Workload cleared.', 'error');
            }
        });
    }
}

function loadDemoPreset() {
    processList = JSON.parse(JSON.stringify(DEMO_WORKLOAD));
    resetForm();
    renderProcessTable();
    showToast('Loaded sample workload (5 processes with idle period t=0..2ms).', 'success');
    runSimulation(currentAlgorithm, rrQuantum);
}

function resetDefaultProcesses() {
    processList = JSON.parse(JSON.stringify(STANDARD_WORKLOAD));
    resetForm();
    renderProcessTable();
}

function navigateTo(sectionId) {
    const el = document.getElementById(sectionId);
    if (el) {
        el.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
}

// ==========================================================================
// 4. Backend Health Check
// ==========================================================================

async function checkBackendHealth() {
    const statusIndicator = document.querySelector('.status-indicator');
    const statusTitle = document.querySelector('.status-title');
    const statusDesc = document.querySelector('.status-desc');

    try {
        const response = await fetch(`${API_BASE}/api/health`, { method: 'GET' });
        if (response.ok) {
            if (statusIndicator) statusIndicator.className = 'status-indicator online';
            if (statusTitle) statusTitle.textContent = 'Native C11 Engine';
            if (statusDesc) statusDesc.textContent = 'Active';
        } else {
            throw new Error(`HTTP ${response.status}`);
        }
    } catch (err) {
        if (statusIndicator) {
            statusIndicator.className = 'status-indicator';
            statusIndicator.style.backgroundColor = '#f59e0b';
        }
        if (statusTitle) statusTitle.textContent = 'C Engine Offline';
        if (statusDesc) statusDesc.textContent = 'Run start_project.bat';
    }
}

// ==========================================================================
// 5. C Engine Simulation API Call
// ==========================================================================

async function runSimulation(algoKey, quantum) {
    if (processList.length === 0) {
        showToast('Workload is empty. Please add at least 1 process.', 'error');
        return;
    }

    const runBtn = document.getElementById('runSimulationBtn');
    const originalText = runBtn ? runBtn.innerHTML : '';
    if (runBtn) {
        runBtn.disabled = true;
        runBtn.innerHTML = `Calculating with C Engine...`;
    }

    const payload = {
        algorithm: ALGO_MAP[algoKey] || algoKey.toLowerCase(),
        quantum: quantum || 2,
        processes: processList
    };

    try {
        const response = await fetch(`${API_BASE}/api/simulate`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });

        if (!response.ok) {
            const errData = await response.json().catch(() => ({}));
            throw new Error(errData.error || `Server responded with status ${response.status}`);
        }

        const data = await response.json();
        currentSimulationData = data;
        currentAlgorithm = algoKey;

        // Render live results from C engine
        updateVisualizerUI(data, algoKey);
        updatePerformanceUI(data.metrics, data.algorithm);
        renderProcessResultsTable(data.processes);
        showToast(`Simulation completed: ${data.algorithm} (calculated in C)`, 'success');

    } catch (err) {
        console.error('Simulation API Error:', err);
        showToast(`C Engine Error: ${err.message}`, 'error');
    } finally {
        if (runBtn) {
            runBtn.disabled = false;
            runBtn.innerHTML = originalText;
        }
    }
}

async function runAllComparison(quantum) {
    if (processList.length === 0) {
        showToast('Workload is empty. Please add processes first.', 'error');
        return;
    }

    const compareBtn = document.getElementById('runAllComparisonBtn');
    const originalText = compareBtn ? compareBtn.innerHTML : '';
    if (compareBtn) {
        compareBtn.disabled = true;
        compareBtn.innerHTML = `Running C Benchmark for all 8 algorithms...`;
    }

    const payload = {
        quantum: quantum || 2,
        processes: processList
    };

    try {
        const response = await fetch(`${API_BASE}/api/compare`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });

        if (!response.ok) {
            const errData = await response.json().catch(() => ({}));
            throw new Error(errData.error || `Server responded with status ${response.status}`);
        }

        const results = await response.json();
        comparisonData = results;

        renderBenchmarkBars(activeComparisonMetric);
        renderComparisonTable();
        navigateTo('comparisonSection');
        showToast('Benchmarked all 8 algorithms through C engine!', 'success');

    } catch (err) {
        console.error('Comparison API Error:', err);
        showToast(`Comparison Error: ${err.message}`, 'error');
    } finally {
        if (compareBtn) {
            compareBtn.disabled = false;
            compareBtn.innerHTML = originalText;
        }
    }
}

// ==========================================================================
// 6. Gantt Chart Visualizer
// ==========================================================================

function updateVisualizerUI(simData, algoKey) {
    const vizPolicyTitle = document.getElementById('vizPolicyTitle');
    const totalDurationEl = document.getElementById('vizTotalDuration');
    const track = document.getElementById('ganttTrack');
    const ruler = document.getElementById('ganttRuler');
    const legend = document.getElementById('ganttLegend');
    const seqTable = document.getElementById('sequenceTableBody');

    const algoTitle = simData.algorithm || ALGO_TITLES[algoKey] || algoKey;
    if (vizPolicyTitle) vizPolicyTitle.textContent = `Simulation: ${algoTitle}`;

    track.innerHTML = '';
    ruler.innerHTML = '';
    legend.innerHTML = '';
    seqTable.innerHTML = '';

    const chart = simData.chart;
    if (!chart || chart.length === 0) {
        track.innerHTML = `<div style="padding: 1rem; color: var(--text-muted); text-align: center; width: 100%;">No Gantt blocks recorded.</div>`;
        return;
    }

    const totalDuration = chart[chart.length - 1].endTime - chart[0].startTime;
    if (totalDurationEl) totalDurationEl.textContent = `${totalDuration} ms`;

    // Process Legend
    const uniquePids = [...new Set(chart.map(b => b.pid))];
    uniquePids.forEach(pid => {
        const span = document.createElement('span');
        span.className = 'legend-badge';
        if (pid < 0) {
            span.innerHTML = `<span class="legend-color-box" style="background: #cbd5e1;"></span> CPU IDLE`;
        } else {
            const colorClass = `p-color-${((pid - 1) % 8) + 1}`;
            span.innerHTML = `<span class="legend-color-box ${colorClass}"></span> Process P${pid}`;
        }
        legend.appendChild(span);
    });

    // Proportional Gantt Blocks
    chart.forEach((block, index) => {
        const duration = block.endTime - block.startTime;
        const widthPercent = (totalDuration > 0) ? (duration / totalDuration) * 100 : 100;
        const isCompact = widthPercent < 6;

        const blockDiv = document.createElement('div');
        const isIdle = block.pid < 0;
        const colorClass = isIdle ? 'idle-block' : `p-color-${((block.pid - 1) % 8) + 1}`;

        blockDiv.className = `gantt-block ${colorClass} ${isCompact ? 'compact-block' : ''}`;
        blockDiv.style.width = `${widthPercent}%`;
        blockDiv.title = isIdle ?
            `CPU IDLE: [${block.startTime}ms → ${block.endTime}ms] (${duration}ms)` :
            `Process P${block.pid}: [${block.startTime}ms → ${block.endTime}ms] (${duration}ms)`;

        blockDiv.innerHTML = `
            <span class="block-label">${isIdle ? 'IDLE' : 'P' + block.pid}</span>
            <span class="block-duration">${duration}ms</span>
        `;
        track.appendChild(blockDiv);

        // Sequence Table Row
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td>#${index + 1}</td>
            <td>
                <span class="pid-badge">
                    <span class="pid-dot ${colorClass}"></span>
                    ${isIdle ? 'CPU IDLE' : 'Process P' + block.pid}
                </span>
            </td>
            <td>${block.startTime} ms</td>
            <td>${block.endTime} ms</td>
            <td>${duration} ms</td>
            <td><span class="badge ${isIdle ? '' : 'badge-info'}">${isIdle ? 'Idle' : 'Running'}</span></td>
        `;
        seqTable.appendChild(tr);
    });

    // Time Ruler Ticks
    const timePoints = [chart[0].startTime, ...chart.map(b => b.endTime)];
    const uniqueTimePoints = [...new Set(timePoints)];

    uniqueTimePoints.forEach(t => {
        const percent = (totalDuration > 0) ? ((t - chart[0].startTime) / totalDuration) * 100 : 0;
        const mark = document.createElement('div');
        mark.className = 'ruler-tick';
        mark.style.left = `${percent}%`;
        mark.textContent = `${t}ms`;
        ruler.appendChild(mark);
    });
}

// ==========================================================================
// 7. Results & Performance Cards
// ==========================================================================

function updatePerformanceUI(metrics, algoName) {
    if (!metrics) return;

    const perfBadge = document.getElementById('perfPolicyBadge');
    if (perfBadge && algoName) perfBadge.textContent = `${algoName} (C Engine)`;

    document.getElementById('metricAvgWT').textContent = metrics.avgWT.toFixed(2);
    document.getElementById('metricAvgTAT').textContent = metrics.avgTAT.toFixed(2);
    document.getElementById('metricAvgRT').textContent = metrics.avgRT.toFixed(2);
    document.getElementById('metricCpuUtil').textContent = metrics.cpuUtilization.toFixed(1);
    document.getElementById('metricThroughput').textContent = metrics.throughput.toFixed(4);
    document.getElementById('metricContextSwitches').textContent = metrics.contextSwitches;
}

function renderProcessResultsTable(processes) {
    const tbody = document.getElementById('processResultsTableBody');
    if (!tbody) return;
    tbody.innerHTML = '';

    if (!processes || processes.length === 0) {
        tbody.innerHTML = `<tr><td colspan="8" style="text-align: center; padding: 12px; color: var(--text-muted);">No process metrics available.</td></tr>`;
        return;
    }

    processes.forEach(p => {
        const colorClass = `p-color-${((p.pid - 1) % 8) + 1}`;
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td>
                <span class="pid-badge">
                    <span class="pid-dot ${colorClass}"></span>
                    P${p.pid}
                </span>
            </td>
            <td>${p.arrivalTime} ms</td>
            <td>${p.burstTime} ms</td>
            <td>${p.priority}</td>
            <td><strong>${p.completionTime} ms</strong></td>
            <td><strong>${p.turnaroundTime} ms</strong></td>
            <td><strong>${p.waitingTime} ms</strong></td>
            <td><strong>${p.responseTime} ms</strong></td>
        `;
        tbody.appendChild(tr);
    });
}

// ==========================================================================
// 8. Algorithm Comparison Rendering
// ==========================================================================

function initComparisonChart() {
    const buttons = document.querySelectorAll('.chart-metric-selector .btn');
    buttons.forEach(btn => {
        btn.addEventListener('click', () => {
            buttons.forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            activeComparisonMetric = btn.getAttribute('data-metric');
            renderBenchmarkBars(activeComparisonMetric);
        });
    });
}

function renderBenchmarkBars(metricKey) {
    const container = document.getElementById('barChartGrid');
    if (!container) return;
    container.innerHTML = '';

    const list = comparisonData || [];
    if (list.length === 0) {
        container.innerHTML = `<div style="text-align: center; color: var(--text-muted); padding: 1.5rem;">Click "Compare All 8 Algorithms" to generate the C engine benchmark.</div>`;
        return;
    }

    const metricProperty = {
        'wt': 'avgWT',
        'tat': 'avgTAT',
        'rt': 'avgRT',
        'switches': 'contextSwitches'
    }[metricKey] || 'avgWT';

    const values = list.map(item => item.metrics[metricProperty]);
    const maxVal = Math.max(...values, 0.001);
    const minVal = Math.min(...values);

    list.forEach(item => {
        const val = item.metrics[metricProperty];
        const isOptimal = (val === minVal);
        const percent = Math.max(Math.round((val / maxVal) * 100), 5);

        const unitText = (metricKey === 'switches') ? 'switches' : 'ms';
        const formattedVal = typeof val === 'number' && val % 1 !== 0 ? val.toFixed(2) : val;

        const row = document.createElement('div');
        row.className = `chart-row ${isOptimal ? 'optimal-row' : ''}`;
        row.innerHTML = `
            <div class="chart-algo-name" title="${item.algorithm}">
                ${item.algorithm}
                ${isOptimal ? '<span class="optimal-badge">Optimal</span>' : ''}
            </div>
            <div class="chart-bar-bg">
                <div class="chart-bar-fill" style="width: ${percent}%;"></div>
            </div>
            <div class="chart-value-text">${formattedVal} ${unitText}</div>
        `;
        container.appendChild(row);
    });
}

function getAlgorithmType(algoName) {
    if (!algoName) return 'Non-Preemptive';
    const lower = algoName.toLowerCase();
    if (lower.includes('non-preemptive') || lower.startsWith('fcfs') || lower.startsWith('hrrn') || (lower.startsWith('sjf') && !lower.includes('preemptive'))) {
        return 'Non-Preemptive';
    }
    if (lower.includes('preemptive') || lower.includes('round robin') || lower.includes('mlfq') || lower.includes('srtf') || /^rr\b/i.test(algoName)) {
        return 'Preemptive';
    }
    return 'Non-Preemptive';
}

function renderComparisonTable() {
    const tbody = document.getElementById('comparisonTableBody');
    if (!tbody) return;
    tbody.innerHTML = '';

    const list = comparisonData || [];
    if (list.length === 0) {
        tbody.innerHTML = `<tr><td colspan="8" style="text-align: center; padding: 1.5rem; color: var(--text-muted);">No comparison data loaded yet.</td></tr>`;
        return;
    }

    list.forEach(item => {
        const m = item.metrics;
        const tr = document.createElement('tr');
        if (currentSimulationData && currentSimulationData.algorithm === item.algorithm) {
            tr.style.backgroundColor = '#eff6ff';
        }

        const type = getAlgorithmType(item.algorithm);
        const isPreemptive = type === 'Preemptive';

        tr.innerHTML = `
            <td><strong>${item.algorithm}</strong></td>
            <td><span class="badge${isPreemptive ? ' badge-info' : ''}">${type}</span></td>
            <td><strong>${m.avgWT.toFixed(2)} ms</strong></td>
            <td><strong>${m.avgTAT.toFixed(2)} ms</strong></td>
            <td><strong>${m.avgRT.toFixed(2)} ms</strong></td>
            <td>${m.cpuUtilization.toFixed(1)}%</td>
            <td>${m.throughput.toFixed(4)}</td>
            <td>${m.contextSwitches}</td>
        `;
        tbody.appendChild(tr);
    });
}

// ==========================================================================
// 9. Process Workload Manager (CRUD)
// ==========================================================================

function initProcessManager() {
    const form = document.getElementById('processForm');
    const cancelEditBtn = document.getElementById('cancelEditBtn');

    if (form) {
        form.addEventListener('submit', (e) => {
            e.preventDefault();
            saveProcess();
        });
    }

    if (cancelEditBtn) {
        cancelEditBtn.addEventListener('click', () => {
            resetForm();
        });
    }
}

function renderProcessTable() {
    const tbody = document.getElementById('processTableBody');
    const countTag = document.getElementById('tableProcessCount');

    if (!tbody) return;
    tbody.innerHTML = '';

    const count = processList.length;
    if (countTag) countTag.textContent = count;

    if (count === 0) {
        tbody.innerHTML = `
            <tr>
                <td colspan="5" style="text-align: center; padding: 1.5rem; color: var(--text-muted);">
                    No processes in workload. Add a process or click "Load Sample Workload".
                </td>
            </tr>
        `;
        return;
    }

    processList.forEach(p => {
        const tr = document.createElement('tr');
        const colorClass = `p-color-${((p.pid - 1) % 8) + 1}`;

        tr.innerHTML = `
            <td>
                <span class="pid-badge">
                    <span class="pid-dot ${colorClass}"></span>
                    P${p.pid}
                </span>
            </td>
            <td>${p.arrivalTime} ms</td>
            <td>${p.burstTime} ms</td>
            <td>${p.priority}</td>
            <td class="text-right">
                <button type="button" class="btn btn-outline btn-xs" onclick="editProcess(${p.pid})">Edit</button>
                <button type="button" class="btn btn-danger-outline btn-xs" onclick="deleteProcess(${p.pid})">Delete</button>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function saveProcess() {
    const pidInput = document.getElementById('inputPid');
    const arrivalInput = document.getElementById('inputArrival');
    const burstInput = document.getElementById('inputBurst');
    const priorityInput = document.getElementById('inputPriority');

    const pid = parseInt(pidInput.value, 10);
    const arrival = parseInt(arrivalInput.value, 10);
    const burst = parseInt(burstInput.value, 10);
    const priority = parseInt(priorityInput.value, 10);

    if (isNaN(pid) || pid <= 0) {
        showToast('PID must be a positive integer.', 'error');
        return;
    }
    if (isNaN(arrival) || arrival < 0) {
        showToast('Arrival time must be 0 or greater.', 'error');
        return;
    }
    if (isNaN(burst) || burst <= 0) {
        showToast('Burst time must be at least 1 ms.', 'error');
        return;
    }
    if (isNaN(priority) || priority <= 0) {
        showToast('Priority must be at least 1.', 'error');
        return;
    }

    if (editingPid !== null) {
        const index = processList.findIndex(p => p.pid === editingPid);
        if (index !== -1) {
            processList[index] = { pid, arrivalTime: arrival, burstTime: burst, priority };
            showToast(`Process P${pid} updated.`, 'success');
        }
    } else {
        if (processList.some(p => p.pid === pid)) {
            showToast(`PID ${pid} already exists in workload.`, 'error');
            return;
        }
        if (processList.length >= 100) {
            showToast('Workload limit is 100 processes.', 'error');
            return;
        }
        processList.push({ pid, arrivalTime: arrival, burstTime: burst, priority });
        showToast(`Process P${pid} added.`, 'success');
    }

    resetForm();
    renderProcessTable();
    runSimulation(currentAlgorithm, rrQuantum);
}

function editProcess(pid) {
    const proc = processList.find(p => p.pid === pid);
    if (!proc) return;

    editingPid = pid;
    document.getElementById('inputPid').value = proc.pid;
    document.getElementById('inputPid').disabled = true;
    document.getElementById('inputArrival').value = proc.arrivalTime;
    document.getElementById('inputBurst').value = proc.burstTime;
    document.getElementById('inputPriority').value = proc.priority;

    document.getElementById('formTitle').textContent = `Edit Process P${pid}`;
    document.getElementById('saveProcessBtn').textContent = 'Update Process';
    document.getElementById('cancelEditBtn').style.display = 'inline-flex';
}

function deleteProcess(pid) {
    processList = processList.filter(p => p.pid !== pid);
    showToast(`Process P${pid} removed.`, 'error');
    if (editingPid === pid) {
        resetForm();
    }
    renderProcessTable();
    runSimulation(currentAlgorithm, rrQuantum);
}

function resetForm() {
    editingPid = null;
    const form = document.getElementById('processForm');
    if (form) form.reset();
    document.getElementById('inputPid').disabled = false;
    document.getElementById('formTitle').textContent = 'Add New Process';
    document.getElementById('saveProcessBtn').textContent = 'Add Process';
    document.getElementById('cancelEditBtn').style.display = 'none';
}

// ==========================================================================
// 10. Simulator Controls Setup
// ==========================================================================

function initSimulator() {
    const algoSelect = document.getElementById('algorithmSelect');
    const quantumGroup = document.getElementById('quantumGroup');
    const runBtn = document.getElementById('runSimulationBtn');
    const compareBtn = document.getElementById('runAllComparisonBtn');
    const rrQuantumInput = document.getElementById('rrQuantumInput');

    if (algoSelect) {
        algoSelect.addEventListener('change', () => {
            const val = algoSelect.value;
            currentAlgorithm = val;

            if (quantumGroup) {
                quantumGroup.style.display = (val === 'RR') ? 'flex' : 'none';
            }
            updatePolicyCallout(val);
        });
    }

    if (rrQuantumInput) {
        rrQuantumInput.addEventListener('change', () => {
            rrQuantum = parseInt(rrQuantumInput.value, 10) || 2;
        });
    }

    if (runBtn) {
        runBtn.addEventListener('click', () => {
            runSimulation(currentAlgorithm, rrQuantum);
            navigateTo('visualizer');
        });
    }

    if (compareBtn) {
        compareBtn.addEventListener('click', () => {
            runAllComparison(rrQuantum);
        });
    }
}

function updatePolicyCallout(algoKey) {
    const titles = {
        'FCFS': 'First Come, First Served (FCFS)',
        'SJF': 'Shortest Job First (Non-Preemptive)',
        'SRTF': 'Shortest Remaining Time First (Preemptive)',
        'RR': 'Round Robin (RR)',
        'PriorityNP': 'Priority Scheduling (Non-Preemptive)',
        'PriorityP': 'Priority Scheduling (Preemptive)',
        'HRRN': 'Highest Response Ratio Next (HRRN)',
        'MLFQ': 'Multilevel Feedback Queue (MLFQ)'
    };

    const descs = {
        'FCFS': 'Processes execute in arrival order without preemption. Simple, but susceptible to the convoy effect.',
        'SJF': 'Selects the ready process with the minimum CPU burst time. Minimizes average waiting time.',
        'SRTF': 'Preempts running process if a newly arriving process has a smaller remaining burst time.',
        'RR': 'Time-sharing policy. Allocates a fixed time slice (quantum) cyclically to ready processes.',
        'PriorityNP': 'Schedules the process with the highest priority (lowest numeric value) to completion.',
        'PriorityP': 'Preempts immediately if a process arrives with a strictly higher priority.',
        'HRRN': 'Calculates (Waiting + Burst) / Burst dynamically to prevent process starvation.',
        'MLFQ': 'Queue 1 (RR Q=2) → Queue 2 (RR Q=4) → Queue 3 (FCFS). Short jobs complete in Q1; batch jobs demote.'
    };

    const calloutTitle = document.getElementById('calloutTitle');
    const calloutDesc = document.getElementById('calloutDesc');
    if (calloutTitle && titles[algoKey]) calloutTitle.textContent = titles[algoKey];
    if (calloutDesc && descs[algoKey]) calloutDesc.textContent = descs[algoKey];
}

// ==========================================================================
// 11. Toast Notifications
// ==========================================================================

function showToast(message, type = 'info') {
    const container = document.getElementById('toastContainer');
    if (!container) return;

    const iconMap = {
        'success': `<svg class="toast-icon" viewBox="0 0 24 24" fill="none" stroke="#10b981" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>`,
        'error': `<svg class="toast-icon" viewBox="0 0 24 24" fill="none" stroke="#ef4444" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>`,
        'info': `<svg class="toast-icon" viewBox="0 0 24 24" fill="none" stroke="#2563eb" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="12" y1="16" x2="12" y2="12"/><line x1="12" y1="8" x2="12.01" y2="8"/></svg>`
    };

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `
        ${iconMap[type] || iconMap.info}
        <span class="toast-message">${message}</span>
    `;

    container.appendChild(toast);

    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transform = 'translateY(10px)';
        toast.style.transition = 'all 0.25s ease';
        setTimeout(() => toast.remove(), 250);
    }, 3000);
}
