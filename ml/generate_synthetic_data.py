#!/usr/bin/env python3
"""
Synthetic Infant Vital Signs Dataset Generator
Generates physiological time-series data with labeled normal baselines and clinical risk events.

Zero third-party dependencies (works out of the box with standard Python library).

Usage:
    python3 generate_synthetic_data.py --samples 5000 --output dataset.csv
"""

import argparse
import csv
import math
import random
from features import extract_window_features


def generate_synthetic_dataset(num_samples: int = 5000, seed: int = 42, output_path: str = "dataset.csv"):
    random.seed(seed)
    
    base_temp = 36.8
    base_hr = 115.0
    base_room_temp = 22.0
    base_room_hum = 50.0

    raw_records = []
    
    current_temp = base_temp
    current_hr = base_hr
    current_room_t = base_room_temp
    current_room_h = base_room_hum
    
    state = "NORMAL"
    state_duration = 0

    for i in range(num_samples):
        if state_duration <= 0:
            rand_val = random.random()
            if rand_val < 0.70:
                state = "NORMAL"
                state_duration = random.randint(150, 400)
            elif rand_val < 0.78:
                state = "FEVER"
                state_duration = random.randint(100, 250)
            elif rand_val < 0.86:
                state = "BRADYCARDIA"
                state_duration = random.randint(40, 100)
            elif rand_val < 0.93:
                state = "TACHYCARDIA"
                state_duration = random.randint(50, 120)
            elif rand_val < 0.97:
                state = "ROOM_OVERHEAT"
                state_duration = random.randint(80, 180)
            else:
                state = "SENSOR_DETACHED"
                state_duration = random.randint(30, 80)

        state_duration -= 1

        if state == "NORMAL":
            current_temp += random.gauss(0, 0.02)
            current_temp = max(36.4, min(37.2, current_temp))
            current_hr += random.gauss(0, 1.5)
            current_hr = max(100, min(135, current_hr))
            current_room_t += random.gauss(0, 0.01)
            current_room_t = max(20.5, min(23.5, current_room_t))
            current_room_h += random.gauss(0, 0.05)
            current_room_h = max(45, min(55, current_room_h))
            label = 0

        elif state == "FEVER":
            current_temp += random.gauss(0.04, 0.01)
            current_temp = max(37.3, min(39.4, current_temp))
            current_hr += random.gauss(0.5, 1.2)
            current_hr = max(130, min(165, current_hr))
            label = 1

        elif state == "BRADYCARDIA":
            current_temp += random.gauss(0, 0.02)
            current_hr -= random.gauss(1.2, 0.8)
            current_hr = max(42, min(58, current_hr))
            label = 2

        elif state == "TACHYCARDIA":
            current_temp += random.gauss(0, 0.02)
            current_hr += random.gauss(1.0, 1.0)
            current_hr = max(168, min(195, current_hr))
            label = 3

        elif state == "ROOM_OVERHEAT":
            current_room_t += random.gauss(0.05, 0.02)
            current_room_t = max(28.5, min(34.0, current_room_t))
            current_temp += random.gauss(0.02, 0.01)
            label = 4

        elif state == "SENSOR_DETACHED":
            current_hr = 0
            current_temp = random.gauss(24.0, 0.5)
            label = 5

        raw_records.append({
            "temp": round(current_temp, 2),
            "hr": int(round(current_hr)),
            "room_temp": round(current_room_t, 2),
            "room_hum": round(current_room_h, 1),
            "label": label
        })

    # Windowed feature generation
    window_size = 15
    feature_rows = []

    for idx in range(window_size, len(raw_records)):
        sub = raw_records[idx - window_size : idx]
        temps = [r["temp"] for r in sub]
        hrs = [r["hr"] for r in sub]
        room_temps = [r["room_temp"] for r in sub]
        
        feats = extract_window_features(temps, hrs, room_temps)
        feats["label"] = raw_records[idx - 1]["label"]
        feats["is_anomaly"] = 0 if feats["label"] == 0 else 1
        feature_rows.append(feats)

    # Write to CSV
    fieldnames = [
        "body_temp", "heart_rate", "hrv_rmssd", "hrv_sdnn",
        "hrv_pnn50", "temp_slope", "temp_room_delta", "label", "is_anomaly"
    ]
    with open(output_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(feature_rows)

    print(f"[✓] Generated {len(feature_rows)} feature samples to {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Generate synthetic infant vital signs dataset.")
    parser.add_argument("--samples", type=int, default=5000, help="Number of time steps to simulate.")
    parser.add_argument("--output", type=str, default="dataset.csv", help="Path to output CSV file.")
    args = parser.parse_args()

    print(f"[*] Generating {args.samples} synthetic vital sign samples...")
    generate_synthetic_dataset(num_samples=args.samples, output_path=args.output)


if __name__ == "__main__":
    main()
