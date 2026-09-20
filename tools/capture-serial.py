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
IMMEDIATE_EMPTY_READ_SECONDS = 0.01
EMPTY_READ_BACKOFF_INITIAL_SECONDS = 0.01
EMPTY_READ_BACKOFF_MAX_SECONDS = 0.25


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


def capture(
    port,
    output,
    ready_file,
    should_stop=None,
    deadline_seconds=None,
    clock=time.monotonic,
    sleep=time.sleep,
):
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
            deadline = None
            if deadline_seconds is not None:
                deadline = clock() + deadline_seconds
            empty_read_backoff = EMPTY_READ_BACKOFF_INITIAL_SECONDS
            while not should_stop():
                if deadline is not None and clock() >= deadline:
                    print("Passive serial capture reached its deadline.", file=sys.stderr)
                    return 2
                read_started = clock()
                data = serial_port.read(4096)
                if data:
                    empty_read_backoff = EMPTY_READ_BACKOFF_INITIAL_SECONDS
                    output_file.write(data)
                    continue

                # A real serial port blocks for its configured timeout. A
                # broken or detached backend can return an empty read
                # immediately. Back off only in that pathological case, and
                # reset after real data, so the recorder cannot become a
                # CPU-bound loop without penalising normal serial timeouts.
                if clock() - read_started < IMMEDIATE_EMPTY_READ_SECONDS:
                    sleep(empty_read_backoff)
                    empty_read_backoff = min(
                        empty_read_backoff * 2,
                        EMPTY_READ_BACKOFF_MAX_SECONDS,
                    )
                else:
                    empty_read_backoff = EMPTY_READ_BACKOFF_INITIAL_SECONDS
    except (OSError, serial.SerialException) as error:
        print(f"Passive serial capture failed: {error}", file=sys.stderr)
        return 1
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--ready-file", required=True)
    parser.add_argument("--deadline-seconds", type=float)
    args = parser.parse_args()

    if args.deadline_seconds is not None and args.deadline_seconds <= 0:
        parser.error("--deadline-seconds must be greater than zero")

    signal.signal(signal.SIGTERM, request_stop)
    signal.signal(signal.SIGINT, request_stop)
    return capture(args.port, args.output, args.ready_file, deadline_seconds=args.deadline_seconds)


if __name__ == "__main__":
    sys.exit(main())
