import sys
import select

from . import config
from .connection import close_clients, set_connected


def djb2(s: str) -> int:
    h = 5381
    for c in s:
        h = ((h << 5) + h) + ord(c)
        h &= 0xFFFFFFFF
    return h


def send_command(command):
    with config.clients_lock:
        if len(config.clients) == 0:
            config.add_log("[ERROR] no connected client")
            return

        dead_clients = []
        for client in config.clients:
            try:
                client.sendall(config.xor_bytes((command + "\n").encode()))
            except OSError:
                dead_clients.append(client)

        for client in dead_clients:
            if client in config.clients:
                config.clients.remove(client)

        if len(config.clients) == 0:
            set_connected(False)


def handle_command(command):
    cmd_lower = command.strip().lower()

    if cmd_lower in config.LOCAL_EXIT_COMMANDS:
        config.server_running = False
        close_clients()
        return

    if cmd_lower == "help":
        for line in config.HELP_TEXT:
            config.add_log(line)
        return

    if command.startswith("HASHPASS "):
        password = command[len("HASHPASS "):]
        h = djb2(password)
        config.add_log(f"[HASHPASS] DJB2(\"{password}\") = 0x{h:08x}")
        config.add_log(f"[HASHPASS] #define WLKOM_PASSWORD_HASH 0x{h:08x}")
        return

    if command == "LOGOUT":
        send_command("QUIT")
        return

    if command:
        if not config.is_connected:
            config.add_log("[ERROR] unknown command or not connected : type HELP for the list")
            return
        pwd_part = config.last_pwd if config.last_pwd else "/"
        if config.last_exit_code is not None:
            if config.last_exit_code == 0:
                config.add_log(f"✓ {pwd_part} >> {command}")
            else:
                config.add_log(f"✗ {config.last_exit_code} {pwd_part} >> {command}")
        else:
            config.add_log(f"{pwd_part} >> {command}")
        send_command(command)


def handle_keyboard():
    ready, _, _ = select.select([sys.stdin], [], [], 0)
    if not ready:
        return

    char = sys.stdin.read(1)

    if char in ("\x03", "\x04"):
        config.server_running = False
        close_clients()
        return

    if char in ("\n", "\r"):
        command = config.current_input.strip()
        config.current_input = ""
        handle_command(command)
        return

    if char in ("\x7f", "\b"):
        config.current_input = config.current_input[:-1]
        return

    if char.isprintable():
        config.current_input += char
