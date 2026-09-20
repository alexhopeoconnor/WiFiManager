#!/usr/bin/env python3
"""Unit-check the passive portal-OTA serial recorder without real hardware."""

import importlib.util
import stat
import sys
import tempfile
import types
from pathlib import Path


class FakeSerial:
    events = []
    read_calls = 0

    def __init__(self, port=None, **kwargs):
        assert port is None
        assert kwargs["baudrate"] == 115200
        assert kwargs["timeout"] == 0.25
        assert kwargs["rtscts"] is False
        assert kwargs["dsrdtr"] is False
        self._dtr = True
        self._rts = True
        self._port = None
        self.is_open = False
        self.events.append(("init", port))

    @property
    def dtr(self):
        return self._dtr

    @dtr.setter
    def dtr(self, value):
        self._dtr = value
        self.events.append(("dtr", value))

    @property
    def rts(self):
        return self._rts

    @rts.setter
    def rts(self, value):
        self._rts = value
        self.events.append(("rts", value))

    @property
    def port(self):
        return self._port

    @port.setter
    def port(self, value):
        self._port = value
        self.events.append(("port", value))

    def open(self):
        assert self._dtr is False
        assert self._rts is False
        self.is_open = True
        self.events.append(("open", self._dtr, self._rts, self._port))

    def close(self):
        self.is_open = False
        self.events.append(("close",))

    def read(self, _size):
        type(self).read_calls += 1
        return b"[OTA] serial evidence\n" if type(self).read_calls == 1 else b""

    def __enter__(self):
        return self

    def __exit__(self, _type, _value, _traceback):
        self.close()


class FakeClock:
    def __init__(self):
        self.value = 0.0

    def __call__(self):
        return self.value

    def advance(self, seconds):
        self.value += seconds


class BlockingEmptySerial(FakeSerial):
    clock = None

    def read(self, _size):
        type(self).read_calls += 1
        type(self).clock.advance(0.25)
        return b""


def load_capture_module():
    fake_serial_module = types.ModuleType("serial")
    fake_serial_module.Serial = FakeSerial
    fake_serial_module.SerialException = OSError
    sys.modules["serial"] = fake_serial_module

    source = Path(__file__).resolve().parents[1] / "capture-serial.py"
    spec = importlib.util.spec_from_file_location("capture_serial", source)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    module = load_capture_module()

    serial_port = module.open_capture_port("/dev/fake")
    assert FakeSerial.events == [
        ("init", None),
        ("dtr", False),
        ("rts", False),
        ("port", "/dev/fake"),
        ("open", False, False, "/dev/fake"),
    ]
    serial_port.close()

    FakeSerial.events.clear()
    FakeSerial.read_calls = 0
    with tempfile.TemporaryDirectory() as temporary_directory:
        output = Path(temporary_directory) / "serial-ota.log"
        ready = Path(temporary_directory) / "serial-ota.ready"
        original_write_ready = module.write_ready

        def checked_write_ready(path):
            assert ("open", False, False, "/dev/fake") in FakeSerial.events
            original_write_ready(path)

        module.write_ready = checked_write_ready
        assert module.capture(
            "/dev/fake",
            output,
            ready,
            should_stop=lambda: FakeSerial.read_calls >= 2,
        ) == 0
        assert ready.read_text() == "ready\n"
        assert output.read_bytes() == b"[OTA] serial evidence\n"
        assert stat.S_IMODE(output.stat().st_mode) == 0o600
        assert stat.S_IMODE(ready.stat().st_mode) == 0o600

    assert ("open", False, False, "/dev/fake") in FakeSerial.events
    assert ("close",) in FakeSerial.events
    assert FakeSerial.read_calls == 2

    # An immediate empty-return backend must back off progressively instead of
    # spinning. This takes no real sleep because the sleeper is injected.
    FakeSerial.read_calls = 0
    with tempfile.TemporaryDirectory() as temporary_directory:
        output = Path(temporary_directory) / "serial-ota.log"
        ready = Path(temporary_directory) / "serial-ota.ready"
        empty_sleeps = []
        assert module.capture(
            "/dev/fake",
            output,
            ready,
            should_stop=lambda: FakeSerial.read_calls >= 5,
            sleep=empty_sleeps.append,
        ) == 0
        assert empty_sleeps == [0.01, 0.02, 0.04, 0.08]

    # A normal 250 ms serial timeout is already rate-limited by the device
    # driver, so it must not receive an additional backoff sleep.
    module.serial.Serial = BlockingEmptySerial
    BlockingEmptySerial.events = []
    BlockingEmptySerial.read_calls = 0
    clock = FakeClock()
    BlockingEmptySerial.clock = clock
    with tempfile.TemporaryDirectory() as temporary_directory:
        output = Path(temporary_directory) / "serial-ota.log"
        ready = Path(temporary_directory) / "serial-ota.ready"
        empty_sleeps = []
        assert module.capture(
            "/dev/fake",
            output,
            ready,
            should_stop=lambda: BlockingEmptySerial.read_calls >= 2,
            clock=clock,
            sleep=empty_sleeps.append,
        ) == 0
        assert empty_sleeps == []
    module.serial.Serial = FakeSerial

    FakeSerial.read_calls = 0
    with tempfile.TemporaryDirectory() as temporary_directory:
        output = Path(temporary_directory) / "serial-ota.log"
        ready = Path(temporary_directory) / "serial-ota.ready"
        assert module.capture(
            "/dev/fake",
            output,
            ready,
            should_stop=lambda: False,
            deadline_seconds=0.0,
        ) == 2

    print("WiFiManager passive OTA serial-capture test-harness check passed")


if __name__ == "__main__":
    main()
