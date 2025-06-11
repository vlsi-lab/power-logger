# SPDX-License-Identifier: GPL-3.0-or-later
#
# Copyright © 2025 Christian Conti, Alessandro Varaldi
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the Licence, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https:#www.gnu.org/licenses/>.

import argparse
import csv
import subprocess
import sys
import time
import serial
from serial.tools import list_ports
from datetime import datetime
from pathlib import Path

UPLOAD_DELAY = 2
BAUD = 2_000_000
SPINNER = ["|", "/", "-", "\\"]

verbose = False 

def autodetect_port() -> str:
    """Return first serial port mentioning 'Arduino'; else raise."""
    ports = list(list_ports.comports())

    for p in ports:
        if "Arduino" in (p.manufacturer or "") or "Arduino" in (p.description or ""):
            if verbose:
                print(f"[INFO]: Auto-detected Arduino on {p.device}")

            return p.device

    if not ports:
        raise RuntimeError("No serial ports found.")

    msg = ("Unable to auto-detect an Arduino on serial port.\n"
           "Detected serial ports:\n  "
           + "\n  ".join(f"{p.device}: {p.description}" for p in ports)
           + "\nSpecify the port manually with --port")
           
    raise RuntimeError(msg)


def compile_sketch(**kwargs) -> None:
    arduino_board = kwargs["arduino_board"]
    sketch = kwargs["sketch"]
    target_board = kwargs["target_board"]

    flags = f"-DBOARD_{target_board} "
    flags += "-DEXT_TRIGGER " if kwargs["ext_trigger"] else ""

    cmd = ["arduino-cli", "compile", "--fqbn", arduino_board,
        "--build-property", f"build.extra_flags={flags}",
        str(sketch)]

    if verbose:
        print("[CMD]:", " ".join(cmd))

    try:
        subprocess.run(cmd, check=True, text=True)
    except FileNotFoundError as exc:
        raise RuntimeError("arduino-cli not found.") from exc


def upload_sketch(sketch: Path, arduino_board: str, port: str) -> None:
    try:
        subprocess.run( ["arduino-cli", "upload", "-p", port, "--fqbn", arduino_board, str(sketch)], check=True, text=True)
    except FileNotFoundError as exc:
        raise RuntimeError("arduino-cli not found.") from exc


def _write_header(writer: csv.writer, field_count: int) -> None:
    writer.writerow([f"value{i+1}" for i in range(field_count)])


def read_serial_and_log(port: str, csv_path: Path, ext_trigger: bool = False) -> None:
    if verbose:
        print(f"[INFO]: Opening {port} @ {BAUD} (Ctrl-C to exit)")
    csv_path.parent.mkdir(parents=True, exist_ok=True)

    base_stem = csv_path.stem
    suffix = csv_path.suffix

    current_f = None
    writer = None
    header_written = False
    max_fields = 0
    spinner_idx = 0

    with serial.Serial(port, BAUD, timeout=None) as ser:
        time.sleep(UPLOAD_DELAY)
        try:
            while True:
                if not verbose:
                    sys.stdout.write(f"\r[INFO]: Running... {SPINNER[spinner_idx]}")
                    sys.stdout.flush()
                    spinner_idx = (spinner_idx + 1) % len(SPINNER)

                line_bytes = ser.readline()
                if not line_bytes:
                    continue

                line = line_bytes.decode(errors="replace").rstrip()
                if not line:
                    continue

                # Handle trigger markers ------------------------------------------------
                if line == "#START":
                    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S.%f")[:-3]
                    current_path = csv_path.with_name(f"{base_stem}_{timestamp}{suffix}")
                    current_f = current_path.open("w", newline="", encoding="utf-8")
                    writer = csv.writer(current_f)
                    header_written = False
                    max_fields = 0
                    if verbose:
                        print(f"\n[INFO]: START logging -> {current_path}")
                    continue

                if line == "#STOP":
                    if current_f:
                        current_f.close()
                        if verbose:
                            print(f"\n[INFO]: STOP logging")
                    current_f = None
                    writer = None
                    continue
                # -----------------------------------------------------------------------

                # Fallback: if no START received, open base file once
                if not ext_trigger and writer is None:
                    current_f = csv_path.open("a", newline="", encoding="utf-8")
                    writer = csv.writer(current_f)
                    header_written = current_f.tell() != 0
                    max_fields = 0

                if writer is None:
                    if verbose:
                        print("[ERROR]: No writer available, cannot log data.")
                    continue

                values = line.split("\t")
                field_count = len(values)
                if not header_written or field_count > max_fields:
                    max_fields = max(max_fields, field_count)
                    _write_header(writer, max_fields)
                    header_written = True

                padded = values + [""] * (max_fields - field_count)
                writer.writerow(padded)
                current_f.flush()

                if verbose:
                    print(f"{line}")

        except serial.SerialException as exc:
            print(f"\n[ERROR]: Serial error: {exc}")
        except KeyboardInterrupt:
            print("\n[INFO]: Power logger stopped by user")
        finally:
            if current_f is not None:
                current_f.close()


def main(argv=None) -> None:
    parser = argparse.ArgumentParser(prog = "power_log.py", description = "Log and monitor power on ZCU102/ZCU106 platforms" )
    parser.add_argument("-s", "--sketch", default="./src/src.ino", help="Sketch directory or .ino file (default: ./src/src.ino)")
    parser.add_argument("-b", "--target-board", default="ZCU106", choices=["ZCU102", "ZCU106"], help="Target board (default: ZCU106)")
    parser.add_argument("-a", "--arduino-board", dest="arduino_board", default="arduino:mbed:nano33ble", help="Target Arduino (default: arduino:mbed:nano33ble)")
    parser.add_argument("-p", "--port", help="Serial port (auto-detect if omitted)")
    parser.add_argument("-d", "--dst", default="./logs", help="CSV output dir (default: ./logs)")
    parser.add_argument("-t", "--ext-trigger", action="store_true", help="Start/stop sampling on external trigger")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args(argv)

    global verbose
    verbose = args.verbose

    sketch_path = Path(args.sketch).expanduser().resolve()
    if not sketch_path.exists():
        sys.exit(f"[ERROR]: Sketch {sketch_path} not found.")

    try:
        c_kwargs = dict(sketch = sketch_path, arduino_board = args.arduino_board, target_board = args.target_board, ext_trigger = args.ext_trigger)
        compile_sketch(**c_kwargs)

        port = args.port or autodetect_port()
        upload_sketch(sketch_path, args.arduino_board, port)

        timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        csv_name = f"power_log_{timestamp}.csv"
        log_dir = Path(args.dst).expanduser().resolve()
        log_dir.mkdir(parents=True, exist_ok=True)

        csv_path = log_dir / csv_name
        read_serial_and_log(port, csv_path, ext_trigger=args.ext_trigger)

    except subprocess.CalledProcessError as exc:
        sys.exit(f"[ERROR]: Command failed with exit code {exc.returncode}")
    except Exception as exc:
        sys.exit(f"\n[ERROR]: {exc}")

if __name__ == "__main__":
    main()