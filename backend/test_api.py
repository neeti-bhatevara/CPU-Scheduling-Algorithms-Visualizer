"""
Automated Backend API Verification Suite
Tests /api/health, /api/simulate (all 8 algorithms, idle periods, edge cases),
and /api/compare against expected C engine outputs.
"""
import json
import urllib.request
import urllib.error
import sys

BASE_URL = "http://localhost:8080"

def post_json(endpoint, data):
    url = f"{BASE_URL}{endpoint}"
    req = urllib.request.Request(
        url,
        data=json.dumps(data).encode("utf-8"),
        headers={"Content-Type": "application/json"}
    )
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode("utf-8"))

def get_json(endpoint):
    url = f"{BASE_URL}{endpoint}"
    with urllib.request.urlopen(url) as resp:
        return json.loads(resp.read().decode("utf-8"))

def test_all():
    print("[1/10] Testing GET /api/health...")
    health = get_json("/api/health")
    assert health["status"] == "online"
    assert health["executable_exists"] is True
    print(f"  OK: {health['engine']}, 8 algorithms supported.")

    # Standard test workload: P1(arr 0, burst 5, prio 2), P2(arr 1, burst 3, prio 1), P3(arr 2, burst 1, prio 3)
    standard_workload = [
        {"pid": 1, "arrival": 0, "burst": 5, "priority": 2},
        {"pid": 2, "arrival": 1, "burst": 3, "priority": 1},
        {"pid": 3, "arrival": 2, "burst": 1, "priority": 3}
    ]

    print("\n[2/10] Testing POST /api/simulate -> FCFS...")
    res_fcfs = post_json("/api/simulate", {"algorithm": "fcfs", "processes": standard_workload})
    assert res_fcfs["algorithm"] == "FCFS"
    assert abs(res_fcfs["metrics"]["avgWT"] - (10.0 / 3.0)) < 0.05
    assert abs(res_fcfs["metrics"]["avgTAT"] - (19.0 / 3.0)) < 0.05
    assert res_fcfs["metrics"]["contextSwitches"] == 2
    assert len(res_fcfs["chart"]) == 3
    print(f"  OK: FCFS avgWT={res_fcfs['metrics']['avgWT']}, avgTAT={res_fcfs['metrics']['avgTAT']}, switches={res_fcfs['metrics']['contextSwitches']}, gantt blocks={len(res_fcfs['chart'])}")

    print("\n[3/10] Testing POST /api/simulate -> SRTF...")
    res_srtf = post_json("/api/simulate", {"algorithm": "srtf", "processes": standard_workload})
    assert "SRTF" in res_srtf["algorithm"]
    assert abs(res_srtf["metrics"]["avgWT"] - (5.0 / 3.0)) < 0.05
    assert abs(res_srtf["metrics"]["avgTAT"] - (14.0 / 3.0)) < 0.05
    assert res_srtf["metrics"]["contextSwitches"] == 4
    print(f"  OK: SRTF avgWT={res_srtf['metrics']['avgWT']}, avgTAT={res_srtf['metrics']['avgTAT']}, switches={res_srtf['metrics']['contextSwitches']}")

    print("\n[4/10] Testing POST /api/simulate -> Round Robin (quantum=2)...")
    res_rr = post_json("/api/simulate", {"algorithm": "rr", "quantum": 2, "processes": standard_workload})
    assert "Round Robin" in res_rr["algorithm"]
    assert abs(res_rr["metrics"]["avgWT"] - (10.0 / 3.0)) < 0.05
    assert abs(res_rr["metrics"]["avgTAT"] - (19.0 / 3.0)) < 0.05
    assert res_rr["metrics"]["contextSwitches"] == 5
    assert len(res_rr["chart"]) == 6
    print(f"  OK: RR(Q=2) avgWT={res_rr['metrics']['avgWT']}, avgTAT={res_rr['metrics']['avgTAT']}, switches={res_rr['metrics']['contextSwitches']}")

    print("\n[5/10] Testing POST /api/simulate -> Priority Non-Preemptive...")
    res_pnp = post_json("/api/simulate", {"algorithm": "priority_np", "processes": standard_workload})
    assert res_pnp["algorithm"] == "Priority (Non-Preemptive)"
    assert abs(res_pnp["metrics"]["avgWT"] - (10.0 / 3.0)) < 0.05
    assert abs(res_pnp["metrics"]["avgTAT"] - (19.0 / 3.0)) < 0.05
    assert res_pnp["metrics"]["contextSwitches"] == 2
    print(f"  OK: Priority NP avgWT={res_pnp['metrics']['avgWT']}, avgTAT={res_pnp['metrics']['avgTAT']}")

    print("\n[6/10] Testing POST /api/simulate -> MLFQ (Q1=2, Q2=4, Q3=FCFS)...")
    res_mlfq = post_json("/api/simulate", {"algorithm": "mlfq", "processes": standard_workload})
    assert "MLFQ" in res_mlfq["algorithm"]
    assert abs(res_mlfq["metrics"]["avgWT"] - (10.0 / 3.0)) < 0.05
    assert abs(res_mlfq["metrics"]["avgTAT"] - (19.0 / 3.0)) < 0.05
    assert res_mlfq["metrics"]["contextSwitches"] == 4
    print(f"  OK: MLFQ avgWT={res_mlfq['metrics']['avgWT']}, avgTAT={res_mlfq['metrics']['avgTAT']}, switches={res_mlfq['metrics']['contextSwitches']}")

    print("\n[7/10] Testing POST /api/simulate -> SJF, Priority Preemptive, HRRN...")
    # SJF
    res_sjf = post_json("/api/simulate", {"algorithm": "sjf", "processes": standard_workload})
    assert abs(res_sjf["metrics"]["avgWT"] - (8.0 / 3.0)) < 0.05
    assert res_sjf["metrics"]["contextSwitches"] == 2
    print(f"  OK: SJF avgWT={res_sjf['metrics']['avgWT']}, avgTAT={res_sjf['metrics']['avgTAT']}")

    # Priority Preemptive
    res_pp = post_json("/api/simulate", {"algorithm": "priority_p", "processes": standard_workload})
    assert abs(res_pp["metrics"]["avgWT"] - 3.00) < 0.05
    assert abs(res_pp["metrics"]["avgTAT"] - 6.00) < 0.05
    assert res_pp["metrics"]["contextSwitches"] == 3
    print(f"  OK: Priority Preemptive avgWT={res_pp['metrics']['avgWT']}, avgTAT={res_pp['metrics']['avgTAT']}")

    # HRRN
    res_hrrn = post_json("/api/simulate", {"algorithm": "hrrn", "processes": standard_workload})
    assert abs(res_hrrn["metrics"]["avgWT"] - (8.0 / 3.0)) < 0.05
    assert res_hrrn["metrics"]["contextSwitches"] == 2
    print(f"  OK: HRRN avgWT={res_hrrn['metrics']['avgWT']}, avgTAT={res_hrrn['metrics']['avgTAT']}")

    print("\n[8/10] Testing Idle CPU Period handling (P9 arrives at t=5)...")
    idle_workload = [{"pid": 9, "arrival": 5, "burst": 3, "priority": 1}]
    res_idle = post_json("/api/simulate", {"algorithm": "fcfs", "processes": idle_workload})
    assert res_idle["chart"][0]["pid"] == -1  # IDLE block
    assert res_idle["chart"][0]["startTime"] == 0
    assert res_idle["chart"][0]["endTime"] == 5
    assert res_idle["chart"][1]["pid"] == 9
    assert res_idle["chart"][1]["startTime"] == 5
    assert res_idle["chart"][1]["endTime"] == 8
    assert abs(res_idle["metrics"]["cpuUtilization"] - 37.5) < 0.1
    print(f"  OK: IDLE period detected: 0-5ms IDLE, 5-8ms P9. CPU Util: {res_idle['metrics']['cpuUtilization']}%")

    print("\n[9/10] Testing POST /api/compare (All 8 algorithms)...")
    compare_res = post_json("/api/compare", {"quantum": 2, "processes": standard_workload})
    assert isinstance(compare_res, list)
    assert len(compare_res) == 8
    print(f"  OK: Returned 8 algorithms:")
    for item in compare_res:
        print(f"      - {item['algorithm']:<30} WT: {item['metrics']['avgWT']:<5.2f} TAT: {item['metrics']['avgTAT']:<5.2f} Switches: {item['metrics']['contextSwitches']}")

    print("\n[10/10] Testing Input Validation & Error Handling...")
    # Empty workload
    try:
        post_json("/api/simulate", {"algorithm": "fcfs", "processes": []})
        print("  FAIL: empty processes should have returned 400")
        sys.exit(1)
    except urllib.error.HTTPError as e:
        assert e.code == 400
        err_msg = json.loads(e.read().decode())["error"]
        print(f"  OK: Empty workload rejected with 400: '{err_msg}'")

    # Invalid algorithm
    try:
        post_json("/api/simulate", {"algorithm": "unknown_algo", "processes": standard_workload})
        print("  FAIL: invalid algorithm should have returned 400")
        sys.exit(1)
    except urllib.error.HTTPError as e:
        assert e.code == 400
        err_msg = json.loads(e.read().decode())["error"]
        print(f"  OK: Unknown algo rejected with 400: '{err_msg}'")

    # Negative arrival time
    try:
        bad_p = [{"pid": 1, "arrival": -5, "burst": 4, "priority": 1}]
        post_json("/api/simulate", {"algorithm": "fcfs", "processes": bad_p})
        print("  FAIL: negative arrival should have returned 400")
        sys.exit(1)
    except urllib.error.HTTPError as e:
        assert e.code == 400
        err_msg = json.loads(e.read().decode())["error"]
        print(f"  OK: Negative arrival rejected with 400: '{err_msg}'")

    print("\n" + "=" * 60)
    print("ALL 10 API & SCHEDULING INTEGRATION TESTS PASSED SUCCESSFULLY!")
    print("=" * 60)

if __name__ == "__main__":
    test_all()
