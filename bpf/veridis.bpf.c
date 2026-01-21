#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 32768);
  __type(key, u32);
  __type(value, u64);
} start_time SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 32768);
  __type(key, u32);
  __type(value, u64);
} cpu_time SEC(".maps");

SEC("tracepoint/sched/sched_switch")
int handle_sched_switch(struct trace_event_raw_sched_switch *ctx) {
  u64 now = bpf_ktime_get_ns();

  u32 prev_pid = ctx->prev_pid;
  u64 *start = bpf_map_lookup_elem(&start_time, &prev_pid);
  if(start) {
    u64 delta = now - *start;
    u64 *total = bpf_map_lookup_elem(&cpu_time, &prev_pid);

    if(total)
      *total += delta;
    else 
      bpf_map_update_elem(&cpu_time, &prev_pid, &delta, BPF_ANY);

    bpf_map_delete_elem(&start_time, &prev_pid);
  }

  u32 next_pid = ctx->next_pid;
  bpf_map_update_elem(&start_time, &next_pid, &now, BPF_ANY);
  

  return 0;
}
char LICENSE[] SEC("license") = "Dual BSD/GPL";
