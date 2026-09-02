import tty
import time
import threading
import termios
import sys

from rich.live import Live
from rich.panel import Panel
from rich.text import Text

from . import config
from .colors import PRIMARY, SECONDARY
from .connection import server_loop
from .command import handle_keyboard


def build_panel():
    content = Text()

    if config.is_connected:
        content.append("🟢 CONNECTED", style="bold green")
        title_status = "[green bold]🟢 CONNECTED[/green bold]"
        border = "green"
    else:
        content.append("🔴 DISCONNECTED", style="bold red")
        title_status = "[red bold]🔴 DISCONNECTED[/red bold]"
        border = "red"

    if config.is_authenticated:
        content.append(" | 🔐 AUTHENTICATED\n", style="bold green")
        auth_status = "[green bold]🔐 AUTH[/green bold]"
    else:
        content.append(" | 🔓 NOT AUTHENTICATED\n", style="bold yellow")
        auth_status = "[yellow bold]🔓 NO AUTH[/yellow bold]"

    content.append("────────────────────────────────────────\n", style=SECONDARY)

    with config.logs_lock:
        for i, (log, is_stderr) in enumerate(config.logs):
            if i > 0:
                content.append("\n")
            if is_stderr:
                content.append("⚠ ", style="bold red")
                content.append(log, style="red")
            elif log.startswith("✓"):
                content.append("✓", style="bold green")
                content.append(log[1:], style=SECONDARY)
            elif log.startswith("✗"):
                idx = log.find(" >> ")
                if idx != -1:
                    content.append(log[:idx], style="bold red")
                    content.append(log[idx:], style=SECONDARY)
                else:
                    content.append(log, style=SECONDARY)
            else:
                content.append(log, style=SECONDARY)

    content.append("\n", style=SECONDARY)
    pwd_part = config.last_pwd if config.last_pwd else "/"
    if config.last_exit_code is not None:
        if config.last_exit_code == 0:
            content.append("✓", style="bold green")
            content.append(f" {pwd_part} >> ", style=SECONDARY)
        else:
            content.append(f"✗ {config.last_exit_code} {pwd_part}", style="bold red")
            content.append(" >> ", style=SECONDARY)
    else:
        content.append(f"{pwd_part} >> ", style=SECONDARY)
    content.append(config.current_input, style=SECONDARY)

    title = (
        f"[{PRIMARY} bold]WLKOM Attacking Program[/{PRIMARY} bold] "
        f"{title_status} {auth_status}"
    )

    return Panel(
        content,
        title=title,
        border_style=border,
        expand=True,
    )


def screen():
    server_thread = threading.Thread(target=server_loop)
    server_thread.daemon = True
    server_thread.start()

    old_settings = termios.tcgetattr(sys.stdin)

    try:
        tty.setcbreak(sys.stdin.fileno())

        with config.console.screen():
            with Live(build_panel(), console=config.console,
                      refresh_per_second=12) as live:
                while config.server_running:
                    handle_keyboard()
                    live.update(build_panel())
                    time.sleep(0.05)
    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
