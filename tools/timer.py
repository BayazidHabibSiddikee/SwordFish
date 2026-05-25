#!/usr/bin/env python3
# tools/timer.py — runs as its own process
# Usage: python timer.py --duration 300   (300 seconds = 5 minutes)

import os, sys, time, argparse, subprocess
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

try:
    import arrow
except ImportError:
    print("Error: 'arrow' library missing. Run pip install arrow")
    sys.exit(1)


def play_alarm():
    """Play alarm.wav using available system tools (ffplay, powershell, etc.)"""
    root = Path(__file__).resolve().parent.parent
    alarm_file = root / 'alarm.wav'
    if not alarm_file.exists():
        print("SPEAK: Timer done! No alarm.wav found.")
        return

    try:
        if sys.platform == "win32":
            # Use PowerShell on Windows
            cmd = ["powershell", "-c", f"(New-Object Media.SoundPlayer '{alarm_file}').PlaySync()"]
            subprocess.run(cmd, check=False)
        else:
            # Use ffplay (since ffmpeg is a dependency) or aplay on Linux
            if subprocess.run(["which", "ffplay"], capture_output=True).returncode == 0:
                subprocess.run(["ffplay", "-nodisp", "-autoexit", str(alarm_file)], 
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            else:
                subprocess.run(["aplay", str(alarm_file)], 
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception as e:
        print(f"SPEAK: Timer done! Sound error: {e}")


def format_duration(seconds: int) -> str:
    h = seconds // 3600
    m = (seconds % 3600) // 60
    s = seconds % 60
    parts = []
    if h: parts.append(f"{h} hour(s)")
    if m: parts.append(f"{m} minute(s)")
    if s: parts.append(f"{s} second(s)")
    return " ".join(parts) if parts else "0 seconds"


def run_timer(duration_seconds: int):
    if duration_seconds <= 0:
        print("SPEAK: Invalid duration.")
        sys.exit(1)

    label = format_duration(duration_seconds)
    now = arrow.now()
    target = now.shift(seconds=duration_seconds)
    end_str = target.format('H:m:s')

    print(f"\u2192 Starting timer for [{label}]")
    print(f"SPEAK: Timer set for {label}. Goes off at {target.format('h:mm A')}.")
    sys.stdout.flush()

    while True:
        if arrow.now().format('H:m:s') == end_str:
            print("SPEAK: Time's up!")
            sys.stdout.flush()
            play_alarm()
            break
        time.sleep(1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Countdown timer")
    parser.add_argument('--duration', type=int, required=True,
                        help='Duration in seconds (e.g. 300 = 5 minutes)')
    args = parser.parse_args()
    run_timer(args.duration)
