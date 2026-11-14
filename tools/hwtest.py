"""Drive 3 ButtonSpielV3 NodeMCUs over serial: election + scripted games."""
import random, re, sys, threading, time
import serial

PORTS = sys.argv[1:] or ["COM4", "COM7", "COM8"]
T0 = time.time()
logs = {p: [] for p in PORTS}
lock = threading.Lock()
conns = {}


def ts():
    return f"{time.time() - T0:7.2f}"


def reader(port):
    s = conns[port]
    buf = b""
    while True:
        data = s.read(256)
        if not data:
            continue
        buf += data
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            text = line.decode("ascii", "ignore").strip()
            if text:
                with lock:
                    logs[port].append((time.time(), text))
                print(f"{ts()} {port}: {text}", flush=True)


def send(port, cmd):
    print(f"{ts()} >> {port}: {cmd}", flush=True)
    conns[port].write((cmd + "\n").encode())


def find(port, pattern, since=0):
    with lock:
        for t, line in reversed(logs[port]):
            if t >= since:
                m = re.search(pattern, line)
                if m:
                    return m
    return None


def wait_for(port, pattern, timeout, since=0):
    end = time.time() + timeout
    while time.time() < end:
        m = find(port, pattern, since)
        if m:
            return m
        time.sleep(0.02)
    return None


def reset(port):
    s = conns[port]
    s.dtr = False
    s.rts = True
    time.sleep(0.1)
    s.rts = False


for p in PORTS:
    conns[p] = serial.Serial(p, 115200, timeout=0.05)
    threading.Thread(target=reader, args=(p,), daemon=True).start()

# --- election: first board boots alone, the others 5 s later ---
reset(PORTS[0])
time.sleep(5)
for p in PORTS[1:]:
    reset(p)
time.sleep(6)

roles, macs = {}, {}
for p in PORTS:
    r = find(p, r"Role: (\w+)")
    m = find(p, r"MAC ([0-9A-F:]{17})")
    roles[p] = r.group(1) if r else None
    macs[p] = m.group(1) if m else None
print("ROLES", roles, "MACS", macs)
gws = [p for p in PORTS if roles[p] == "gateway"]
assert len(gws) == 1, f"expected exactly one gateway, got {gws}"
gw = gws[0]
time.sleep(2)  # let HELLOs register


def unit_port(n):
    if n == 1:
        return gw
    with lock:
        hits = [re.search(rf"New unit {n}: ([0-9A-F:]{{17}})", l) for t, l in logs[gw]]
    mac = [h for h in hits if h][-1].group(1)
    return next(p for p in PORTS if macs[p] == mac)


def play(name, presses, timeout_ms, false_press_at=None, skip_at=None):
    start = time.time()
    send(gw, f"start {name} {presses} {timeout_ms}")
    seen = set()
    while True:
        m = None
        end = time.time() + timeout_ms / 1000 + 6
        while time.time() < end:
            with lock:
                lines = [(t, l) for t, l in logs[gw] if t >= start]
            over = [l for t, l in lines if l.startswith("Game over")]
            if over:
                return over[0]
            turns = [re.search(r"Turn (\d+) -> unit (\d+)", l) for t, l in lines]
            turns = [t for t in turns if t and int(t.group(1)) not in seen]
            if turns:
                m = turns[0]
                break
            time.sleep(0.01)
        if not m:
            return None
        turn, unit = int(m.group(1)), int(m.group(2))
        seen.add(turn)
        port = unit_port(unit)
        if turn == false_press_at:
            wrong = next(p for p in PORTS if p != port)
            send(wrong, "press")
        if turn == skip_at:
            continue  # let it time out
        time.sleep(random.uniform(0.15, 0.4))
        send(port, "press")


r1 = play("alice", 6, 3000, false_press_at=2, skip_at=4)
print("RESULT1", r1)
time.sleep(5)
r2 = play("bob", 5, 3000)
print("RESULT2", r2)
time.sleep(1)
send(gw, "state")
send(gw, "results")
time.sleep(1)

# --- gateway failover: hold gateway in reset, a unit must take over ---
conns[gw].rts = True
t = time.time()
others = [p for p in PORTS if p != gw]
taken = None
end = time.time() + 30
while time.time() < end and not taken:
    for p in others:
        if find(p, r"Role: gateway", t):
            taken = p
    time.sleep(0.1)
print("FAILOVER new gateway:", taken, f"after {time.time() - t:.1f}s")
conns[gw].rts = False
time.sleep(6)
print("OLD GW role after reboot:", find(gw, r"Role: (\w+)", t + 0.5) and find(gw, r"Role: (\w+)", t + 0.5).group(1))
