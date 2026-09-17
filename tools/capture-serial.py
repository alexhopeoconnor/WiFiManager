#!/usr/bin/env python3
"""Passively retain serial evidence for a physical portal OTA upload.

The recorder deliberately opens the ESP USB-UART with both modem-control lines
inactive.  It never resets the target: the calling runner has already flashed
and booted fixture A before this process attaches.
"""

import argparse
import os
import signal
import sys
import time

import serial


_stop_requested = False


def request_stop(_signum, _frame):
    """Let the read loop finish promptly after a normal runner cleanup."""
    global _stop_requested
    _stop_requested = True


def open_capture_port(port):
    """Open a UART without PySerial's default reset-causing line assertion."""
    serial_port = serial.Serial(
        port=None,
        baudrate=115200,
        timeout=0.25,
        rtscts=False,
        dsrdtr=False,
    )
    serial_port.dtr = False
    serial_port.rts = False
    serial_port.port = port
    serial_port.open()
    return serial_port


def write_ready(path):
    """Publish readiness only after the passive serial port is open."""
    descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    try:
        os.fchmod(descriptor, 0o600)
        os.write(descriptor, b"ready\n")
    finally:
        os.close(descriptor)


def capture(port, output, ready_file, should_stop=None):
    """Append serial bytes until terminated by the owning hardware runner."""
    global _stop_requested
    _stop_requested = False
    if should_stop is None:
        should_stop = lambda: _stop_requested

    try:
        with (
            open_capture_port(port) as serial_port,
            open(output, "ab", buffering=0) as output_file,
        ):
            os.fchmod(output_file.fileno(), 0o600)
            write_ready(ready_file)
            while not should_stop():
                data = serial_port.read(4096)
                if data:
                    output_file.write(data)
                else:
                    # PySerial normally blocks for its configured timeout. A
                    # broken backend or test double can return immediately;
                    # never let that turn a detached recorder into a CPU loop.
                    time.sleep(0.01)
    except (OSError, serial.SerialException) as error:
        print(f"Passive serial capture failed: {error}", file=sys.stderr)
        return 1
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--ready-file", required=True)
    args = parser.parse_args()

    signal.signal(signal.SIGTERM, request_stop)
    signal.signal(signal.SIGINT, request_stop)
    return capture(args.port, args.output, args.ready_file)


if __name__ == "__main__":
    sys.exit(main())
