#!/usr/bin/env python3
import sqlite3
import os
import sys
from datetime import datetime, timedelta
from rich.console import Console
from rich.table import Table
from rich.panel import Panel

DB_PATH = os.path.expanduser("~/.veridis/history.db")

def generate_report(days=30):
    if not os.path.exists(DB_PATH):
        print("No history database found. Start Veridis to begin tracking.")
        return

    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    
    # Query for the last N days
    cursor.execute("SELECT date, used, saved FROM daily_carbon ORDER BY date DESC LIMIT ?", (days,))
    rows = cursor.fetchall()
    
    if not rows:
        print("No data recorded in the database yet.")
        return

    console = Console()
    table = Table(title=f"Veridis Carbon Report (Last {days} Days)")
    table.add_column("Date", style="dim")
    table.add_column("Carbon Used (g)", justify="right", style="red")
    table.add_column("Carbon Saved (g)", justify="right", style="green")
    table.add_column("Efficiency %", justify="right", style="cyan")

    total_used = 0
    total_saved = 0

    for date, used, saved in reversed(rows):
        total_used += used
        total_saved += saved
        efficiency = (saved / (used + saved) * 100) if (used + saved) > 0 else 0
        table.add_row(date, f"{used:.2f}", f"{saved:.2f}", f"{efficiency:.1f}%")

    console.print(table)
    
    avg_saved = total_saved / len(rows)
    summary = f"Total Carbon Saved: [bold green]{total_saved:.2f} g[/]\n"
    summary += f"Average Daily Savings: [bold white]{avg_saved:.2f} g[/]\n"
    summary += f"Total Theoretical Emissions Avoided: [bold cyan]{(total_saved/(total_used+total_saved)*100 if total_used+total_saved > 0 else 0):.1f}%[/]"
    
    console.print(Panel(summary, title="Summary Statistics", border_style="green"))
    conn.close()

if __name__ == "__main__":
    days = 30
    if len(sys.argv) > 1:
        try:
            days = int(sys.argv[1])
        except:
            pass
    generate_report(days)
