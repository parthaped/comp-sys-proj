#!/bin/bash
# Benchmark wrapper for ECE 434 — writes results.csv next to this script.
# Usage: chmod +x bench.sh && ./bench.sh

set -u

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR" || exit 1

OUTCSV="$DIR/results.csv"
TMP_TIME="$DIR/.bench_time_$$"

echo "L,PN,branch,seconds" > "$OUTCSV"

run_one() {
  local L=$1 PN=$2 B=$3
  local IN="in_${L}_${PN}_${B}.txt"
  local OUT="out_${L}_${PN}_${B}.txt"
  local sec

  if [ -x /usr/bin/time ]; then
    /usr/bin/time -f '%e' -o "$TMP_TIME" ./ece434_proj1 "$L" 10 "$PN" "$B" --generate --sleep 0 "$IN" "$OUT" >/dev/null 2>&1 || true
    sec="$(tr -d '\n\r ' < "$TMP_TIME" 2>/dev/null || echo "nan")"
  else
    local t0 t1
    t0=$(date +%s)
    ./ece434_proj1 "$L" 10 "$PN" "$B" --generate --sleep 0 "$IN" "$OUT" >/dev/null 2>&1 || true
    t1=$(date +%s)
    sec=$((t1 - t0))
  fi

  echo "$L,$PN,$B,$sec" >> "$OUTCSV"
}

rm -f "$TMP_TIME"

# Smaller grid first. Add 1000000 and larger PN as your patience allows.
for L in 12000 100000; do
  for PN in 1 4 8; do
    for B in 2 5; do
      echo "Running L=$L PN=$PN B=$B ..." >&2
      run_one "$L" "$PN" "$B"
    done
  done
done

rm -f "$TMP_TIME"
echo "Wrote $OUTCSV" >&2
cat "$OUTCSV"
