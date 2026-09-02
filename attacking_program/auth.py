import os
import sys
import getpass
import base64

from argon2 import PasswordHasher
from argon2.exceptions import VerifyMismatchError

from . import config


def _load_auth():
    if not os.path.exists(config.AUTH_FILE):
        return None, None
    try:
        with open(config.AUTH_FILE, "r") as f:
            lines = [l.strip() for l in f if l.strip()]
        if len(lines) < 2:
            return None, None
        password_hash = base64.b64decode(lines[0]).decode()
        xor_key = base64.b64decode(lines[1])
        return password_hash, xor_key
    except Exception:
        return None, None


def _save_auth(password_hash: str, xor_key: bytes):
    for path in [os.path.expanduser("~/.config"), config.CONFIG_DIR]:
        if not os.path.exists(path):
            os.mkdir(path, 0o700)
    line1 = base64.b64encode(password_hash.encode()).decode()
    line2 = base64.b64encode(xor_key).decode()
    fd = os.open(config.AUTH_FILE, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "w") as f:
        f.write(f"{line1}\n{line2}\n")


def _first_setup():
    print("First run : setting up credentials.")
    print("These will be stored (hashed / encoded) in:", config.AUTH_FILE)
    print()

    while True:
        try:
            password = getpass.getpass("Choose a password: ")
            confirm = getpass.getpass("Confirm password: ")
        except (KeyboardInterrupt, EOFError):
            print()
            sys.exit(1)
        if password == confirm:
            break
        print("Passwords do not match, try again.")

    try:
        xor_key_input = input("XOR key [default: wlkom]: ").strip()
    except (KeyboardInterrupt, EOFError):
        print()
        sys.exit(1)

    xor_key = xor_key_input.encode() if xor_key_input else b"wlkom"

    ph = PasswordHasher(time_cost=3, memory_cost=65536, parallelism=4)
    password_hash = ph.hash(password)
    _save_auth(password_hash, xor_key)
    print("Credentials saved. Launching...\n")
    return password_hash, xor_key


def login():
    password_hash, xor_key = _load_auth()

    if password_hash is None:
        password_hash, xor_key = _first_setup()

    config.XOR_KEY = xor_key

    ph = PasswordHasher()
    max_attempts = 3

    for attempt in range(1, max_attempts + 1):
        try:
            password = getpass.getpass(f"Password ({attempt}/{max_attempts}): ")
        except (KeyboardInterrupt, EOFError):
            print()
            sys.exit(1)

        try:
            ph.verify(password_hash, password)
            return
        except VerifyMismatchError:
            print("Wrong password.")

    print("Too many failed attempts. Exiting.", file=sys.stderr)
    sys.exit(1)
