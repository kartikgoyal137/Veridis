#!/usr/bin/env python3
import json
import time
import os
from typing import Optional, Dict, Any
from datetime import datetime

from rich.live import Live
from rich.table import Table
from rich.panel import Panel
from rich.layout import Layout
from rich.console import Console
from rich.text import Text
from rich.progress import BarColumn, Progress, TextColumn
from rich import box
from rich.align import Align
from rich.padding import Padding

STATS_FILE = "/tmp/veridis_stats.json"

console = Console()

def load_stats() -> Optional[Dict[str, Any]]:
    if not os.path.exists(STATS_FILE):
        return None
    try:
        # Check if file has been updated recently (within 10s)
        mtime = os.path.getmtime(STATS_FILE)
        if time.time() - mtime > 10:
            # Data might be stale
            pass 
            
        with open(STATS_FILE, "r") as f:
            data = json.load(f)
            return data
    except (json.JSONDecodeError, IOError):
        return None

def make_layout() -> Layout:
    layout = Layout()
    layout.split(
        Layout(name="header", size=4),
        Layout(name="main", ratio=1),
        Layout(name="footer", size=3)
    )
    layout["main"].split_row(
        Layout(name="sidebar", ratio=1),
        Layout(name="body", ratio=3)
    )
    layout["body"].split_column(
        Layout(name="power", size=10),
        Layout(name="processes", ratio=1)
    )
    return layout

class Header:
    def __init__(self, stats: Optional[Dict[str, Any]]):
        self.stats = stats

    def __rich__(self) -> Panel:
        grid = Table.grid(expand=True)
        grid.add_column(justify="left", ratio=1)
        grid.add_column(justify="center", ratio=1)
        grid.add_column(justify="right", ratio=1)
        
        policy = self.stats.get("policy", "Unknown") if self.stats else "N/A"
        policy_style = "bold green" if policy == "green" else "bold yellow" if policy == "moderate" else "bold red"
        
        grid.add_row(
            Text.assemble((" VERIDIS ", "bold white on dark_green"), (" v1.2", "dim")),
            Text.assemble(("POLICY: ", "white"), (policy.upper(), policy_style)),
            Text(datetime.now().strftime("%Y-%m-%d %H:%M:%S"), style="bold cyan"),
        )
        return Panel(Padding(grid, (0, 1)), style="white on black", box=box.ROUNDED)

def make_power_panel(stats: Optional[Dict[str, Any]]) -> Panel:
    if not stats:
        return Panel(Align.center(Text("Waiting for system data...", style="dim")), title="Power & Carbon")

    # Power Progress Bars
    cpu_pwr = stats.get("cpu_power_w", 0)
    gpu_pwr = stats.get("gpu_power_w", 0)
    total_pwr = stats.get("power_w", 0)
    soft_lim = stats.get("soft_limit", 1)
    hard_lim = stats.get("hard_limit", 1)
    gpu_util = stats.get("gpu_utilization", 0)
    gpu_lim = stats.get("gpu_limit", hard_lim)
    
    # Grid for Power and Carbon
    grid = Table.grid(expand=True, padding=(0, 2))
    grid.add_column(ratio=2) # Power Bars
    grid.add_column(ratio=1) # Carbon intensity

    # Power section
    power_grid = Table.grid(expand=True)
    power_grid.add_column(width=10) # Label
    power_grid.add_column()         # Bar
    power_grid.add_column(width=8, justify="right")  # Value

    def add_power_row(label, value, limit, color):
        progress = Progress(
            BarColumn(bar_width=None, style="grey23", complete_style=color, finished_style=color),
            expand=True
        )
        task = progress.add_task("", total=limit, completed=min(value, limit))
        power_grid.add_row(
            Text(label, style="bold"),
            progress,
            Text(f"{value:>5.1f}W", style=f"bold {color}")
        )

    add_power_row("CPU", cpu_pwr, hard_lim, "cyan")
    add_power_row("GPU", gpu_pwr, gpu_lim, "magenta")
    
    # Combined Bar
    add_power_row("TOTAL", total_pwr, hard_lim, "green" if total_pwr < soft_lim else "yellow" if total_pwr < hard_lim else "red")
    
    # Limit markers
    limits_text = Text.assemble(
        ("Soft: ", "white"), (f"{soft_lim:.1f}W", "yellow"),
        (" | ", "dim"),
        ("Hard: ", "white"), (f"{hard_lim:.1f}W", "red"),
        (" | ", "dim"),
        ("GPU Util: ", "white"), (f"{gpu_util}%", "bold magenta")
    )
    power_grid.add_row("", Align.center(limits_text), "")

    # Carbon Intensity Section
    intensity = stats.get("carbon_intensity", 0)
    if intensity < 250:
        int_color, int_label = "green", "CLEAN"
    elif intensity < 450:
        int_color, int_label = "yellow", "MODERATE"
    else:
        int_color, int_label = "red", "DIRTY"
        
    carbon_text = Text.assemble(
        ("\nCARBON INTENSITY\n", "bold white"),
        (f"{intensity:.0f}", f"bold {int_color} size 20"), (" gCO2/kWh\n", "dim"),
        (f"[{int_label}]", f"bold {int_color} reverse")
    )
    
    grid.add_row(power_grid, Align.center(carbon_text))
    
    return Panel(grid, title="[bold]Real-time Power & Carbon[/]", border_style="blue")

def make_history_panel(stats: Optional[Dict[str, Any]]) -> Panel:
    if not stats:
        return Panel("", title="Stats")

    table = Table.grid(padding=(0, 1))
    table.add_column(style="bold cyan")
    table.add_column(justify="right")

    used = stats.get("total_carbon_used_g", 0)
    saved = stats.get("total_carbon_saved_g", 0)
    
    table.add_row("Total Used:", f"[red]{used:.2f}g[/]")
    table.add_row("Total Saved:", f"[green]{saved:.2f}g[/]")
    
    efficiency = (saved / (used + saved) * 100) if (used + saved) > 0 else 0
    table.add_row("Efficiency:", f"[bold white]{efficiency:.1f}%[/]")
    
    history = stats.get("history", [])
    if history:
        table.add_row("", "")
        table.add_row("[bold underline]7-Day History", "")
        for entry in reversed(history[-5:]): # Show last 5 days
            date_short = entry['date'].split('-')[-2:]
            date_short = "/".join(date_short)
            table.add_row(f"{date_short}:", f"[green]+{entry['saved']:.1f}g[/]")

    return Panel(Align.center(table), title="[bold]Carbon Insights[/]", border_style="green")

def make_process_table(stats: Optional[Dict[str, Any]]) -> Panel:
    if not stats:
        return Panel(Align.center(Text("Searching for processes...", style="dim")), title="Process Monitor")

    table = Table(box=box.SIMPLE, expand=True, header_style="bold magenta")
    table.add_column("PID", style="dim", width=7)
    table.add_column("Process Name", style="bold white", ratio=2)
    table.add_column("User", style="blue", ratio=1)
    table.add_column("Metric", justify="right", ratio=1)
    table.add_column("Status", justify="center", width=12)

    # CPU Processes
    procs = stats.get("processes", [])
    sorted_procs = sorted(procs, key=lambda x: x.get("cpu_percent", 0), reverse=True)
    
    # Show more processes to improve visibility
    for p in sorted_procs[:18]: 
        throttled = p.get("is_throttled", False)
        status = "[bold white on red] THROTTLED [/]" if throttled else "[bold green] OK [/]"
        name = p.get("name") or "[unknown]"
        table.add_row(
            str(p.get("pid")),
            name,
            p.get("user", "root"),
            f"{p.get('cpu_percent', 0):.1f}% CPU",
            status
        )

    # GPU Processes
    gpu_procs = stats.get("gpu_processes", [])
    if gpu_procs:
        table.add_section()
        for p in gpu_procs:
            table.add_row(
                str(p.get("pid")),
                f"[chartreuse1]{p.get('name', 'unknown')}[/]",
                "---",
                f"{p.get('vram_mb', 0):.0f} MB VRAM",
                "[bold orange1] GPU_LOAD [/]"
            )

    return Panel(table, title="[bold]Active Resource Monitor[/]", border_style="magenta")

def make_footer(stats: Optional[Dict[str, Any]]) -> Panel:
    ts = stats.get("timestamp", 0) if stats else 0
    dt = datetime.fromtimestamp(ts).strftime("%H:%M:%S") if ts > 0 else "N/A"
    
    status_msg = " [bold green]System Active[/] " if stats else " [bold red]Connecting...[/] "
    
    footer_text = Text.assemble(
        (status_msg, "white"),
        (" | last update: ", "dim"), (dt, "cyan"),
        (" | CTRL+C to Exit ", "bold white")
    )
    return Panel(Align.center(footer_text), box=box.SIMPLE)

def run():
    layout = make_layout()
    
    with Live(layout, refresh_per_second=2, screen=True):
        while True:
            stats = load_stats()
            
            layout["header"].update(Header(stats))
            layout["sidebar"].update(make_history_panel(stats))
            layout["power"].update(make_power_panel(stats))
            layout["processes"].update(make_process_table(stats))
            layout["footer"].update(make_footer(stats))
            
            time.sleep(0.5)

if __name__ == "__main__":
    try:
        run()
    except KeyboardInterrupt:
        pass
