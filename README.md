# Veridis

**Veridis** is a power-aware process regulator for Linux systems. It dynamically monitors system power consumption and CPU usage to throttle specific processes when energy thresholds are exceeded.

The system leverages **eBPF** for low-overhead CPU accounting and **Cgroups v2** for precise resource enforcement.

---

## Technical Architecture

Veridis operates through a central **userspace daemon** that coordinates three core components:

### 1. Power Monitoring (RAPL)

- Reads energy metrics directly from the Intel **RAPL** (Running Average Power Limit) interface
- Uses `/sys/class/powercap` to sample energy counters
- Computes real-time power usage (in **Watts**) by measuring energy deltas over precise intervals

### 2. CPU Accounting (eBPF)

- Loads a custom **eBPF program** into the kernel
- Hooks into the `tracepoint/sched/sched_switch` tracepoint
- Tracks exact CPU time consumed by each **thread and process** using a kernel hash map
- Avoids polling `/proc`, reducing overhead and eliminating race conditions

### 3. Enforcement (Cgroups v2)

- When power usage exceeds defined **Soft** or **Hard** limits:
  - The scheduler identifies CPU-heavy processes belonging to monitored users
  - Processes exceeding CPU thresholds are migrated to a dedicated cgroup:
    ```
    veridis/bad_jobs
    ```
- CPU bandwidth is strictly limited using the `cpu.max` interface

---

## Dependencies

### System Requirements

- **Linux Kernel**
  - Must support **eBPF** and **BTF (BPF Type Format)**

- **Cgroups**
  - Cgroups v2 must be mounted at:
    ```
    /sys/fs/cgroup
    ```

### Libraries

- `libbpf`
- `libelf`
- `zlib`

### Hardware

- Intel CPU with **RAPL** support exposed to the operating system

---

## Configuration

Veridis is configured using a `config.json` file.

### Configuration Options

| Key | Description |
|---|---|
| `cgroup_root` | Path to the cgroup v2 mount point |
| `power_limit_soft` | Power threshold (Watts) for moderate throttling |
| `power_limit_hard` | Power threshold (Watts) for aggressive throttling |
| `cpu_threshold_ms` | CPU time accumulation before a process is throttled |
| `monitored_users` | List of usernames subject to regulation |
| `whitelist` | Process names exempt from throttling (e.g., `Xorg`, `sshd`) |
| `probation_cycles` | Number of cycles a process remains throttled before release |

---

## Build Instructions

The project includes a `Makefile` for building the userspace daemon.

```bash
make

