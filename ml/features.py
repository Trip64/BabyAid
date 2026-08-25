"""
Infant Biomedical Feature Extraction & Signal Processing Module
Part of Kidsentinel ML Pipeline

Supports both pure Python standard library (zero-dependency) and NumPy / Pandas.
"""

import math
from typing import Dict, List, Union


def compute_rr_intervals_from_hr(heart_rate_bpm: List[float]) -> List[float]:
    """
    Converts instantaneous Heart Rate (BPM) into R-R interval times (milliseconds).
    RR (ms) = 60000 / BPM
    """
    rr_intervals = []
    for hr in heart_rate_bpm:
        if hr > 0:
            rr_intervals.append(60000.0 / hr)
        else:
            rr_intervals.append(0.0)
    return rr_intervals


def extract_hrv_features(rr_intervals_ms: List[float]) -> Dict[str, float]:
    """
    Extracts time-domain Heart Rate Variability (HRV) metrics from R-R intervals.
    
    Metrics:
    - mean_rr: Mean RR interval (ms)
    - sdnn: Standard deviation of NN/RR intervals (autonomic tone)
    - rmssd: Root Mean Square of Successive Differences (parasympathetic vagal tone)
    - pnn50: Percentage of successive intervals differing by > 50ms
    """
    valid_rr = [x for x in rr_intervals_ms if x > 0]

    if len(valid_rr) < 2:
        return {
            "mean_rr": 0.0,
            "sdnn": 0.0,
            "rmssd": 0.0,
            "pnn50": 0.0
        }

    mean_rr = sum(valid_rr) / len(valid_rr)
    variance = sum((x - mean_rr) ** 2 for x in valid_rr) / (len(valid_rr) - 1)
    sdnn = math.sqrt(variance)

    diffs = [valid_rr[i+1] - valid_rr[i] for i in range(len(valid_rr) - 1)]
    rmssd = math.sqrt(sum(d ** 2 for d in diffs) / len(diffs)) if diffs else 0.0
    nn50_count = sum(1 for d in diffs if abs(d) > 50.0)
    pnn50 = (nn50_count / len(diffs)) * 100.0 if diffs else 0.0

    return {
        "mean_rr": round(mean_rr, 2),
        "sdnn": round(sdnn, 2),
        "rmssd": round(rmssd, 2),
        "pnn50": round(pnn50, 2)
    }


def extract_thermal_features(temperatures_c: List[float], time_step_seconds: float = 2.0) -> Dict[str, float]:
    """
    Computes thermal velocity and variance metrics over a sliding window.
    """
    temps = [t for t in temperatures_c if t > 0]
    if len(temps) < 2:
        return {
            "temp_mean": temps[0] if temps else 0.0,
            "temp_variance": 0.0,
            "temp_slope_per_min": 0.0
        }

    mean_t = sum(temps) / len(temps)
    var_t = sum((t - mean_t) ** 2 for t in temps) / len(temps)

    # Linear slope (deg C per minute)
    n = len(temps)
    x = [(i * time_step_seconds) / 60.0 for i in range(n)]
    mean_x = sum(x) / n
    denom = sum((xi - mean_x) ** 2 for xi in x)
    if denom > 0:
        slope = sum((x[i] - mean_x) * (temps[i] - mean_t) for i in range(n)) / denom
    else:
        slope = 0.0

    return {
        "temp_mean": round(mean_t, 2),
        "temp_variance": round(var_t, 4),
        "temp_slope_per_min": round(slope, 3)
    }


def extract_window_features(
    temperatures: List[float],
    heart_rates: List[float],
    room_temperatures: List[float] = None
) -> Dict[str, float]:
    """
    Extracts unified feature vector for machine learning inference from sliding arrays.
    """
    rr_intervals = compute_rr_intervals_from_hr(heart_rates)
    hrv_dict = extract_hrv_features(rr_intervals)
    thermal_dict = extract_thermal_features(temperatures)

    current_temp = temperatures[-1] if temperatures else 0.0
    current_hr = heart_rates[-1] if heart_rates else 0.0
    
    room_delta = 0.0
    if room_temperatures and len(room_temperatures) > 0:
        room_delta = current_temp - room_temperatures[-1]

    return {
        "body_temp": round(current_temp, 2),
        "heart_rate": round(current_hr, 1),
        "hrv_rmssd": hrv_dict["rmssd"],
        "hrv_sdnn": hrv_dict["sdnn"],
        "hrv_pnn50": hrv_dict["pnn50"],
        "temp_slope": thermal_dict["temp_slope_per_min"],
        "temp_room_delta": round(room_delta, 2)
    }
