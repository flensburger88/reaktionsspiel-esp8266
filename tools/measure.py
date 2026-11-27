"""Measure press -> next button lit over USB serial.
Usage: python measure.py [--presses 20] [--games 2] [--react 200] COMa COMb
Resets both boards, waits for the election, starts solo games on the gateway,
presses every lit unit after --react ms and logs the gap from press to the
next 'Lit turn' line on any board (includes the random pause of the game)."""
import argparse, re, statistics, threading, time
import serial

ap = argparse.ArgumentParser()
ap.add_argument("ports", nargs="+")
ap.add_argument("--presses", type=int, default=20)
ap.add_argument("--games", type=int, default=2)
ap.add_argument("--react", type=int, default=200)
ap.add_argument("--duel", action="store_true")
args = ap.parse_args()

t0 = time.time()
lock = threading.Lock()
ports = {}
gateway = None
last_press = None
gaps = []
lit_count = 0
game_over = threading.Event()
pause_re = re.compile(r"pause (\d+)")
pending_pause = []


def log(msg):
    with lock:
        print(f"{time.time() - t0:8.3f} {msg}", flush=True)


def reader(port):
    global gateway, last_press, lit_count
    s = ports[port]
    buf = b""
    while True:
        buf += s.read(256)
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            line = raw.decode("ascii", "ignore").strip()
            if not line:
                continue
            now = time.time()
            log(f"{port}: {line}")
            if line.startswith("Role: gateway"):
                gateway = port
            if line.startswith("Game over"):
                game_over.set()
            m = re.match(r"Pause (\d+) ms", line)
            if m:
                pending_pause.append(int(m.group(1)))
            if line.startswith("Lit turn"):
                with lock:
                    lit_count += 1
                    if last_press is not None and pending_pause:
                        gaps.append((now - last_press) * 1000 - pending_pause.pop(0))
                    pending_pause.clear()
                    last_press = None
                def press(s=s, port=port):
                    global last_press
                    with lock:
                        last_press = time.time()
                    s.write(b"press\n")
                threading.Timer(args.react / 1000, press).start()


for p in args.ports:
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = p, 115200, 0.02
    s.dtr = False
    s.rts = False
    s.open()
    ports[p] = s
# reset all boards: RTS -> EN low
for s in ports.values():
    s.rts = True
time.sleep(0.1)
for s in ports.values():
    s.rts = False
for p in ports:
    threading.Thread(target=reader, args=(p,), daemon=True).start()

deadline = time.time() + 10
while gateway is None and time.time() < deadline:
    time.sleep(0.1)
if gateway is None:
    print("no gateway")
    raise SystemExit(1)
time.sleep(2)  # let the units say hello
for g in range(args.games):
    game_over.clear()
    with lock:
        last_press = None
    cmd = f"duel R B {args.presses} 5000\n" if args.duel else f"start T {args.presses} 5000\n"
    ports[gateway].write(cmd.encode())
    game_over.wait(120)
    time.sleep(5)

if gaps:
    gaps.sort()
    print(f"\nOVERHEAD (gap - pause) n={len(gaps)} min={gaps[0]:.0f} median={statistics.median(gaps):.0f} "
          f"mean={statistics.mean(gaps):.0f} p90={gaps[int(len(gaps)*0.9)]:.0f} max={gaps[-1]:.0f} ms")
