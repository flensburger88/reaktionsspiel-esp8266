"""Auto-player: presses each unit over USB when it lights up.

Start a game on the phone and watch it play itself. Usage (PlatformIO python):
    python autoplay.py [--minutes 30] [--min-ms 180] [--max-ms 650] COM4 COM7 COM8
Does not reset the boards (DTR/RTS stay low).
"""
import argparse, random, threading, time
import serial

ap = argparse.ArgumentParser()
ap.add_argument("ports", nargs="+")
ap.add_argument("--minutes", type=float, default=30)
ap.add_argument("--min-ms", type=int, default=180)
ap.add_argument("--max-ms", type=int, default=650)
args = ap.parse_args()

t0 = time.time()
end = t0 + args.minutes * 60
lock = threading.Lock()


def log(msg):
    with lock:
        print(f"{time.time() - t0:8.2f} {msg}", flush=True)


def run(port):
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, 115200, 0.05
    s.dtr = s.rts = False  # keep the board running
    s.open()
    buf = b""
    while time.time() < end:
        buf += s.read(256)
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            line = raw.decode("ascii", "ignore").strip()
            if not line:
                continue
            log(f"{port}: {line}")
            if line.startswith("Lit turn"):
                delay = random.randint(args.min_ms, args.max_ms) / 1000
                threading.Timer(delay, lambda: (s.write(b"press\n"), log(f"{port}: >> press"))).start()


threads = [threading.Thread(target=run, args=(p,), daemon=True) for p in args.ports]
for t in threads:
    t.start()
log(f"auto-player running on {', '.join(args.ports)} for {args.minutes:g} min")
while time.time() < end:
    time.sleep(1)
