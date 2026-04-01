#!/usr/bin/env python3
import json
import time
import os
from rich.live import Live
from rich.table import Table
from rich.panel import Panel
from rich.layout import Layout
from rich.console import Console
from rich.text import Text
from rich import box
from datetime import datetime

STATS_FILE = "/tmp/veridis_stats.json"

console = Console()

def load_stats():
    if not os.path.exists(STATS_FILE):
        return None
    try:
        with open(STATS_FILE, "r") as f:
            return json.load(f)
    except:
        return None

def make_layout() -> Layout:
    layout = Layout()
    layout.split(
        Layout(name="header", size=3),
        Layout(name="main", ratio=1),
        Layout(name="footer", size=3)
    )
    layout["main"].split_row(
        Layout(name="sidebar", ratio=1),
        Layout(name="body", ratio=3)
    )
    return layout

class VeridisHeader:
    def __rich__(self) -> Panel:
        grid = Table.grid(expand=True)
        grid.add_column(justify="left", ratio=1)
        grid.add_column(justify="center", ratio=1)
        grid.add_column(justify="right", ratio=1)
        grid.add_row(
            Text("VERIDIS", style="bold green"),
            Text("Power-Aware Process Regulator", style="white"),
            Text(datetime.now().strftime("%H:%M:%S"), style="bold blue"),
        )
        return Panel(grid, style="white on black", box=box.HORIZONTALS)

def make_summary(stats):
    if not stats:
        return Panel("Waiting for data...", title="Summary", border_style="red")
    
    table = Table.grid(padding=1)
    table.add_column(style="bold cyan", justify="right")
    table.add_column(style="white")
    
    table.add_row("CPU Power:", f"{stats.get('cpu_power_w', 0):.1f} W")
    table.add_row("GPU Power:", f"{stats.get('gpu_power_w', 0):.1f} W")
    table.add_row("Total Power:", f"[bold white]{stats.get('power_w', 0):.1f} W[/]")
    table.add_row("Carbon Intensity:", f"{stats.get('carbon_intensity', 0)} gCO2/kWh")
    carbon_used = stats.get('total_carbon_used_g', 0)
    carbon_saved = stats.get('total_carbon_saved_g', 0)
    table.add_row("Used/Saved:", f"[red]{carbon_used:.3f}g[/] / [green]{carbon_saved:.3f}g[/]")
    
    # Power Bar
    pwr = stats.get('power_w', 0)
    hard = stats.get('hard_limit', 100)
    if hard == 0: hard = 100
    
    percent = min(100, (pwr / hard) * 100)
    bar_color = "red" if pwr > stats.get('soft_limit', hard) else "green"
    bar = f"[{bar_color}]{'█' * int(percent/5)}{'░' * (20 - int(percent/5))}[/]"
    
    table.add_row("", "")
    table.add_row("Power Load:", bar)

    # History Summary
    history = stats.get("history", [])
    if history:
        table.add_row("", "")
        table.add_row("[bold]Last 7 Days (Saved)[/]", "")
        for day in history[:5]:
            table.add_row(f"{day['date']}:", f"[green]{day['saved']:.2f} g[/]")
    
    return Panel(table, title="[bold]System Status[/bold]", border_style="green", padding=(1, 2))

def make_proc_table(stats):
    if not stats or "processes" not in stats:
        return Panel("No processes monitored", title="Monitored Processes", border_style="yellow")

    table = Table(box=box.SIMPLE_HEAVY, expand=True)
    table.add_column("PID", style="dim", width=8)
    table.add_column("Process Name", style="bold white")
    table.add_column("User", style="blue")
    table.add_column("CPU %", justify="right", style="magenta")
    table.add_column("Status", justify="center")

    sorted_procs = sorted(stats["processes"], key=lambda x: x["cpu_percent"], reverse=True)

    for p in sorted_procs:
        status_text = "[bold red]THROTTLED[/]" if p.get("is_throttled") else "[bold green]OK[/]"
        table.add_row(
            str(p["pid"]),
            p["name"],
            p["user"],
            f"{p['cpu_percent']:.1f}%",
            status_text
        )

    gpu_procs = stats.get("gpu_processes", [])
    for p in gpu_procs:
        table.add_row(
            str(p["pid"]),
            f"[chartreuse1]{p['name']} (GPU)[/]",
            "user",
            f"{p.get('vram_mb', 0):.0f} MB",
            "[bold orange1]GPU_LOAD[/]"
        )

    return Panel(table, title="[bold]Resource Intensive Processes[/bold]", border_style="blue")

def make_footer(stats):
    total_cpu = len(stats.get("processes", [])) if stats else 0
    total_gpu = len(stats.get("gpu_processes", [])) if stats else 0
    return Panel(
        Text(f"Total Monitored: {total_cpu} CPU, {total_gpu} GPU | Press Ctrl+C to Exit | Veridis v1.1", justify="center", style="dim"),
        box=box.SIMPLE
    )

def run():
    layout = make_layout()
    header = VeridisHeader()
    
    with Live(layout, refresh_per_second=1, screen=True):
        while True:
            stats = load_stats()
            layout["header"].update(header)
            layout["sidebar"].update(make_summary(stats))
            layout["body"].update(make_proc_table(stats))
            layout["footer"].update(make_footer(stats))
            time.sleep(1)

if __name__ == "__main__":
    try:
        run()
    except KeyboardInterrupt:
        pass
