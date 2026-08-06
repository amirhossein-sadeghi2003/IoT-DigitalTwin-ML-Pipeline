from pathlib import Path
import csv

PIPELINE_DIR = Path(__file__).resolve().parents[1]

INPUT_PATH = PIPELINE_DIR / "data" / "sample_telemetry.csv"
RESULTS_DIR = PIPELINE_DIR / "results"
OUTPUT_CSV = RESULTS_DIR / "decision_replay_predictions.csv"
SUMMARY_TXT = RESULTS_DIR / "decision_replay_summary.txt"

TARGET_COLS = ["heater", "cooler", "flood", "window"]


def to_float(value, default=0.0):
    if value is None:
        return float(default)

    text = str(value).strip().lower()
    if text in {"", "null", "none", "nan"}:
        return float(default)

    try:
        return float(text)
    except ValueError:
        return float(default)


def predict_outputs(row):
    temp = to_float(row.get("temp"))
    light = to_float(row.get("light"))
    magnet = to_float(row.get("magnet"))
    alarm = to_float(row.get("alarm"))

    if alarm == 1.0:
        return {
            "heater": 0,
            "cooler": 0,
            "flood": 0,
            "window": 0,
        }

    return {
        "heater": int(temp < 20.0),
        "cooler": int(temp > 28.0),
        "flood": int(light < 100.0),
        "window": int(magnet == 0.0),
    }


def distance_missing(row):
    value = row.get("distance")

    if value is None:
        return 1

    text = str(value).strip().lower()
    if text in {"", "null", "none", "nan", "-1"}:
        return 1

    return 0


def main():
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    with INPUT_PATH.open(newline="") as f:
        rows = list(csv.DictReader(f))

    output_rows = []

    for row in rows:
        preds = predict_outputs(row)
        out = dict(row)
        out["distance_missing"] = distance_missing(row)

        for target in TARGET_COLS:
            out[target] = preds[target]

        out["active_outputs"] = sum(preds.values())
        output_rows.append(out)

    fieldnames = list(output_rows[0].keys())

    with OUTPUT_CSV.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(output_rows)

    counts = {
        target: sum(int(row[target]) for row in output_rows)
        for target in TARGET_COLS
    }

    alarm_rows = [
        row for row in output_rows
        if to_float(row.get("alarm")) == 1.0
    ]

    if alarm_rows:
        alarm_all_off_rate = sum(
            sum(int(row[target]) for target in TARGET_COLS) == 0
            for row in alarm_rows
        ) / len(alarm_rows)
    else:
        alarm_all_off_rate = 0.0

    lines = [
        "Offline decision replay summary",
        "===============================",
        "",
        f"Input file: {INPUT_PATH.relative_to(PIPELINE_DIR.parent)}",
        f"Output file: {OUTPUT_CSV.relative_to(PIPELINE_DIR.parent)}",
        "",
        f"Telemetry rows replayed: {len(output_rows)}",
        f"Rows with missing distance: {sum(int(row['distance_missing']) for row in output_rows)}",
        f"Alarm rows: {len(alarm_rows)}",
        f"Alarm all-off rate: {alarm_all_off_rate:.3f}",
        "",
        "Predicted active output counts:",
    ]

    for target, count in counts.items():
        lines.append(f"- {target}: {count}")

    lines.extend(["", "Scenario outputs:"])

    for row in output_rows:
        active = [
            target for target in TARGET_COLS
            if int(row[target]) == 1
        ]
        active_text = ", ".join(active) if active else "none"
        lines.append(f"- {row['scenario']}: {active_text}")

    SUMMARY_TXT.write_text("\n".join(lines) + "\n")

    print(
        "Saved predictions to: "
        f"{OUTPUT_CSV.relative_to(PIPELINE_DIR.parent)}"
    )
    print(
        "Saved summary to: "
        f"{SUMMARY_TXT.relative_to(PIPELINE_DIR.parent)}"
    )


if __name__ == "__main__":
    main()
