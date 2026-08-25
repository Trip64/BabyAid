#!/usr/bin/env python3
"""
Telemetry Data Collector & Logger
Supports polling live JSON data from the Display Node Web Hub or reading Serial debug output.

Usage:
    python data_collector.py --http http://192.168.4.1/api/state --output live_telemetry.csv
    python data_collector.py --serial /dev/ttyUSB0 --baud 115200 --output live_serial.csv
"""

import argparse
import csv
import json
import os
import sys
import time
import requests


def collect_http(url: str, output_file: str, interval: float = 2.0):
    print(f"[*] Starting HTTP telemetry polling from: {url}")
    print(f"[*] Saving telemetry stream to: {output_file}")
    
    file_exists = os.path.exists(output_file)
    with open(output_file, mode="a", newline="") as f:
        writer = csv.writer(f)
        if not file_exists:
            writer.writerow(["timestamp", "infant_temp", "heart_rate", "room_temp", "room_hum", "alert", "msg"])
        
        while True:
            try:
                response = requests.get(url, timeout=3)
                if response.status_code == 200:
                    data = response.json()
                    now = time.strftime("%Y-%m-%d %H:%M:%S")
                    row = [
                        now,
                        data.get("bt", 0.0),
                        data.get("hr", 0),
                        data.get("rt", 0.0),
                        data.get("rh", 0.0),
                        data.get("alert", False),
                        data.get("msg", "")
                    ]
                    writer.writerow(row)
                    f.flush()
                    print(f"[{now}] Temp: {data.get('bt')}°C | HR: {data.get('hr')} BPM | Room: {data.get('rt')}°C {data.get('rh')}% | Alert: {data.get('alert')}")
                else:
                    print(f"[!] Warning: HTTP Status {response.status_code}")
            except requests.exceptions.RequestException as e:
                print(f"[✗] Connection error: {e}")
            
            time.sleep(interval)


def collect_serial(port: str, baud: int, output_file: str):
    import serial
    print(f"[*] Connecting to serial port {port} at {baud} baud...")
    ser = serial.Serial(port, baud, timeout=2)
    file_exists = os.path.exists(output_file)

    with open(output_file, mode="a", newline="") as f:
        writer = csv.writer(f)
        if not file_exists:
            writer.writerow(["timestamp", "raw_line"])

        while True:
            try:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    now = time.strftime("%Y-%m-%d %H:%M:%S")
                    writer.writerow([now, line])
                    f.flush()
                    print(f"[{now}] {line}")
            except Exception as e:
                print(f"[✗] Serial error: {e}")
                break


def main():
    parser = argparse.ArgumentParser(description="Live telemetry acquisition tool for Kidsentinel.")
    parser.add_argument("--http", type=str, default=None, help="Display node HTTP API state URL (e.g. http://192.168.4.1/api/state)")
    parser.add_argument("--serial", type=str, default=None, help="Serial port path (e.g. /dev/cu.usbserial-0001 or COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate.")
    parser.add_argument("--interval", type=float, default=2.0, help="Polling interval in seconds (HTTP mode).")
    parser.add_argument("--output", type=str, default="telemetry_log.csv", help="Output CSV log file path.")
    args = parser.parse_args()

    try:
        if args.http:
            collect_http(args.http, args.output, args.interval)
        elif args.serial:
            collect_serial(args.serial, args.baud, args.output)
        else:
            print("[!] Error: You must specify either --http or --serial mode.")
            parser.print_help()
    except KeyboardInterrupt:
        print("\n[*] Data collection stopped by user.")


if __name__ == "__main__":
    main()
