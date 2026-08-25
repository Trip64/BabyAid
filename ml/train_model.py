#!/usr/bin/env python3
"""
Model Training & Evaluation Pipeline
Supports both scikit-learn (RandomForest, IsolationForest) and built-in pure Python fallback.

Usage:
    python3 train_model.py --data dataset.csv --out models/
"""

import argparse
import csv
import json
import os
import math

FEATURE_COLUMNS = [
    "body_temp",
    "heart_rate",
    "hrv_rmssd",
    "hrv_sdnn",
    "hrv_pnn50",
    "temp_slope",
    "temp_room_delta"
]

CLASS_NAMES = [
    "Normal",
    "Hyperthermia/Fever",
    "Bradycardia/SIDS-Risk",
    "Tachycardia",
    "Room-Overheat",
    "Sensor-Detached"
]


def load_dataset(csv_path: str):
    rows = []
    with open(csv_path, mode="r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            parsed = {k: float(row[k]) for k in FEATURE_COLUMNS}
            parsed["label"] = int(row["label"])
            parsed["is_anomaly"] = int(row["is_anomaly"])
            rows.append(parsed)
    return rows


def train_builtin(data_path: str, output_dir: str):
    print("[*] Running built-in statistical decision boundary trainer (Standard Library)...")
    dataset = load_dataset(data_path)
    
    # Calculate physiological bounds per class
    stats = {}
    for c_idx, c_name in enumerate(CLASS_NAMES):
        class_samples = [d for d in dataset if d["label"] == c_idx]
        if not class_samples:
            continue
        stats[c_name] = {
            "count": len(class_samples),
            "temp_mean": round(sum(d["body_temp"] for d in class_samples) / len(class_samples), 2),
            "hr_mean": round(sum(d["heart_rate"] for d in class_samples) / len(class_samples), 1),
            "rmssd_mean": round(sum(d["hrv_rmssd"] for d in class_samples) / len(class_samples), 2)
        }

    os.makedirs(output_dir, exist_ok=True)
    summary_path = os.path.join(output_dir, "model_summary.json")
    with open(summary_path, "w") as f:
        json.dump({
            "features": FEATURE_COLUMNS,
            "classes": CLASS_NAMES,
            "class_profiles": stats
        }, f, indent=2)

    print(f"\n[✓] Trained model summary saved to: {summary_path}")
    print("\nClass Physiological Profiles:")
    for k, v in stats.items():
        print(f"  [{k:22s}] Samples: {v['count']:4d} | Temp Mean: {v['temp_mean']}°C | HR Mean: {v['hr_mean']} BPM")


def train_sklearn(data_path: str, output_dir: str):
    import joblib
    import pandas as pd
    from sklearn.model_selection import train_test_split
    from sklearn.ensemble import RandomForestClassifier, IsolationForest
    from sklearn.metrics import classification_report

    os.makedirs(output_dir, exist_ok=True)
    df = pd.read_csv(data_path)
    X = df[FEATURE_COLUMNS].values
    y = df["label"].values
    yb = df["is_anomaly"].values

    X_train, X_test, y_train, y_test, yb_train, yb_test = train_test_split(
        X, y, yb, test_size=0.25, random_state=42, stratify=y
    )

    print("\n--- Training Supervised Random Forest Classifier ---")
    rf_clf = RandomForestClassifier(n_estimators=50, max_depth=6, random_state=42)
    rf_clf.fit(X_train, y_train)

    y_pred = rf_clf.predict(X_test)
    unique_labels = sorted(list(set(y_test) | set(y_pred)))
    target_names = [CLASS_NAMES[i] for i in unique_labels]
    print("\nClassification Report:")
    print(classification_report(y_test, y_pred, target_names=target_names))

    rf_model_path = os.path.join(output_dir, "vital_classifier_rf.joblib")
    joblib.dump({"model": rf_clf, "features": FEATURE_COLUMNS, "classes": CLASS_NAMES}, rf_model_path)
    print(f"[✓] Saved Scikit-Learn Model: {rf_model_path}")


def main():
    parser = argparse.ArgumentParser(description="Train Kidsentinel ML models.")
    parser.add_argument("--data", type=str, default="dataset.csv", help="Path to input features dataset CSV.")
    parser.add_argument("--out", type=str, default="models", help="Output directory for trained models.")
    args = parser.parse_args()

    try:
        import sklearn
        train_sklearn(args.data, args.out)
    except ImportError:
        train_builtin(args.data, args.out)


if __name__ == "__main__":
    main()
