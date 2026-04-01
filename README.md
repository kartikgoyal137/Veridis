# Veridis

**Veridis** is a power-aware process regulator for Linux systems. It dynamically monitors system power consumption (CPU & GPU) and CPU usage to throttle specific processes when energy thresholds are exceeded.

The system leverages **eBPF** for low-overhead CPU accounting, **RAPL** for Intel power monitoring, and **Nvidia SMI** for GPU regulation.

---

## Technical Architecture

Veridis operates through a central **userspace daemon** that coordinates four core components:

### 1. Power Monitoring (RAPL & GPU)

- **CPU**: Reads energy metrics directly from the Intel **RAPL** (Running Average Power Limit) interface.
- **GPU**: Monitors Nvidia GPU power draw and utilization via `nvidia-smi`.
- Computes real-time combined power usage (in **Watts**).

### 2. CPU Accounting (eBPF)

- Hooks into the `tracepoint/sched/sched_switch` tracepoint to track exact CPU time consumed by each thread and process.

### 3. GPU Regulation

- When power usage exceeds thresholds, Veridis can dynamically cap the **GPU Power Limit** to reduce overall energy consumption.

### 4. Enforcement (Cgroups v2)

- CPU-heavy processes are migrated to a dedicated restricted cgroup (`veridis/bad_jobs`) with bandwidth limits.

---

## Persistence & Analytics

Veridis tracks carbon emissions over time, storing daily aggregates in a local **SQLite** database (`~/.veridis/history.db`).

---

## Terminal User Interface (TUI)

Veridis includes a real-time interactive TUI and a report generation tool.

### Running the TUI

1.  **Start the Veridis daemon**:
    ```bash
    sudo ./veridis
    ```
2.  **Launch the TUI**:
    ```bash
    python3 scripts/veridis_tui.py
    ```

### Generating Reports

To generate a weekly or monthly carbon saving report:
```bash
python3 scripts/generate_report.py [days]
```
Example: `python3 scripts/generate_report.py 7` (last 7 days).

---

## Build Instructions

```bash
make
```

### Requirements
- **System**: Linux with eBPF and Cgroups v2.
- **Hardware**: Intel CPU (RAPL) and Nvidia GPU (optional).
- **Libraries**: `libbpf`, `libelf`, `zlib`, `libcurl`, `sqlite3`.
- **Python**: `pip install rich`.
