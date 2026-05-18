#!/usr/bin/env python3
# scripts/jsonl_to_perfetto.py
"""Convert JSONL trace output to Chrome Trace format for Perfetto."""

import json
import sys
from pathlib import Path


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.jsonl> <output.json>", file=sys.stderr)
        sys.exit(1)

    input_path = Path(sys.argv[1])
    output_path = Path(sys.argv[2])

    events = []
    with input_path.open() as f:
        for line_num, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                event = json.loads(line)
                events.append(event)
            except json.JSONDecodeError as e:
                print(f"Warning: line {line_num} not valid JSON: {e}", file=sys.stderr)

    trace = {
        "traceEvents": events,
        "displayTimeUnit": "ns",
    }

    with output_path.open("w") as f:
        json.dump(trace, f, separators=(",", ":"))

    print(f"Wrote {len(events)} events to {output_path}")


if __name__ == "__main__":
    main()