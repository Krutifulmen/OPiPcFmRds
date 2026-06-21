#!/usr/bin/env bash
set -euo pipefail

FREQ="107.9"
RADIO_DEV="/dev/radio0"
ALSA_DEV="default"
AUDIO_FILE=""
PS="OPI-PC"
RT="Orange Pi PC FM"

usage() {
  cat <<USAGE
Usage: $0 -audio FILE [-freq MHz] [-radio /dev/radio0] [-alsa DEVICE] [-ps TEXT] [-rt TEXT]

Controls a Linux V4L2 Si4713 FM transmitter device and plays audio from the
Orange Pi PC audio output into the Si4713 analog line input.

This is the supported "actually hear music on an FM receiver" path for Orange
Pi PC. It requires external Si4713-compatible FM transmitter hardware connected
over I2C plus analog audio wiring from Orange Pi PC line/headphone output to the
Si4713 LIN/RIN inputs.

Examples:
  $0 -audio song.wav -freq 107.9
  $0 -audio song.wav -freq 102.3 -radio /dev/radio0 -alsa hw:0,0 -ps OPIPC -rt 'Orange Pi test'
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -audio) AUDIO_FILE="${2:-}"; shift 2 ;;
    -freq) FREQ="${2:-}"; shift 2 ;;
    -radio) RADIO_DEV="${2:-}"; shift 2 ;;
    -alsa) ALSA_DEV="${2:-}"; shift 2 ;;
    -ps) PS="${2:-}"; shift 2 ;;
    -rt) RT="${2:-}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage >&2; exit 1 ;;
  esac
done

if [[ -z "$AUDIO_FILE" ]]; then
  echo "Error: -audio FILE is required." >&2
  usage >&2
  exit 1
fi

if [[ ! -e "$AUDIO_FILE" ]]; then
  echo "Error: audio file not found: $AUDIO_FILE" >&2
  exit 1
fi

if [[ ! -e "$RADIO_DEV" ]]; then
  cat >&2 <<ERR
Error: $RADIO_DEV does not exist.
Load/configure the Linux si4713 V4L2 radio driver and confirm the transmitter
appears as /dev/radioN. This script cannot transmit from Orange Pi PC GPIO alone.
ERR
  exit 1
fi

for cmd in v4l2-ctl aplay; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "Error: required command not found: $cmd" >&2
    exit 1
  fi
done

echo "Configuring Si4713 transmitter $RADIO_DEV at ${FREQ} MHz..."
v4l2-ctl -d "$RADIO_DEV" --set-freq="$FREQ"

# Control names differ slightly between kernel versions/platform wrappers. Try
# common RDS TX controls but do not fail audio transmission if unavailable.
if v4l2-ctl -d "$RADIO_DEV" --list-ctrls 2>/dev/null | grep -q 'rds'; then
  v4l2-ctl -d "$RADIO_DEV" -c "rds_tx_ps_name=${PS:0:8}" 2>/dev/null || true
  v4l2-ctl -d "$RADIO_DEV" -c "rds_tx_radio_text=${RT:0:64}" 2>/dev/null || true
fi

cat <<INFO
Now playing: $AUDIO_FILE
ALSA output: $ALSA_DEV

IMPORTANT:
- Orange Pi PC audio output must be physically wired to Si4713 LIN/RIN/GND.
- Keep power low, use filtering/shielding, and comply with local RF law.
- Stop with Ctrl+C.
INFO

aplay -D "$ALSA_DEV" "$AUDIO_FILE"
