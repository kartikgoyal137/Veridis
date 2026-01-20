#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

SEC("tracepoint/sched/sched_switch")
int handle_sched_switch(struct trace_event_raw_sched_switch *ctx) {
    bpf_printk("Switch happening\n");
    return 0;
}
char LICENSE[] SEC("license") = "Dual BSD/GPL";
