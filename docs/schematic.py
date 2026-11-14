"""Schaltplan einer ButtonSpiel-Einheit zeichnen (schemdraw).

    python -m pip install schemdraw
    python docs/schematic.py [ausgabeordner]

Erzeugt schaltplan.svg (Standard: docs/img).
"""
import sys
from pathlib import Path

import schemdraw
import schemdraw.elements as elm

out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / "img"
out.mkdir(parents=True, exist_ok=True)

OPT = "#8a8a8a"  # empfohlen, im Steckbrett-Prototyp noch nicht verbaut
GND_Y, V5_Y = 0.0, 5.0  # Masse- und +5-V-Schiene


def pin(name, side, slot=None, anchor=None):
    return elm.IcPin(name=name, side=side, slot=slot, anchorname=anchor or name)


with schemdraw.Drawing(show=False) as d:
    d.config(fontsize=11, unit=2.5)

    # --- Versorgung: Batterie -> Schalter -> AMS1117-5.0 ---------------------
    d += elm.Line().at((0, GND_Y)).up(0.8)
    d += elm.BatteryCell().up().label("7–12 V\n9-V-Block", loc="bottom", ofst=0.3)
    d += elm.Line().toy(V5_Y)
    d += elm.Switch().right().label("Ein/Aus", loc="top").color(OPT)
    d += elm.Line().right(0.5)
    reg = elm.Ic(
        pins=[pin("VIN", "left", "2/2"), pin("GND", "left", "1/2", "GNDL"),
              pin("VOUT", "right", "2/2"), pin("GND", "right", "1/2", "GNDR")],
        size=(3, 2), pinspacing=1.2, edgepadH=0.4,
    ).anchor("VIN").label("AMS1117-5.0 (Modul)", loc="top")
    d += reg
    d += elm.Line().at(reg.GNDL).left(0.6)
    d += elm.Line().toy(GND_Y)
    d += elm.Line().at(reg.GNDR).right(0.6)
    d += elm.Line().toy(GND_Y)

    # +5-V- und Masse-Schiene
    x_end = 13.5
    d += elm.Line().at(reg.VOUT).tox(x_end).label("+5 V", loc="top", ofst=0.1)
    d += elm.Line().at((0, GND_Y)).tox(x_end - 0.6)
    d += elm.Ground().at((0, GND_Y))

    # Pufferelko
    x_c = reg.VOUT[0] + 2.5
    d += elm.Dot().at((x_c, V5_Y))
    d += elm.Capacitor(polar=True).at((x_c, V5_Y)).down().toy(GND_Y).color(OPT) \
        .label("470–1000 µF / 10 V", loc="bottom", ofst=0.25)
    d += elm.Dot().at((x_c, GND_Y))

    # --- Wemos D1 mini --------------------------------------------------------
    wemos = elm.Ic(
        pins=[pin("5V", "left", "2/2"), pin("G", "left", "1/2"),
              pin("D1 (GPIO5)", "right", "2/2", "D1"), pin("D3 (GPIO0)", "right", "1/2", "D3")],
        size=(4.5, 3), pinspacing=1.5, edgepadH=0.5, lofst=0.2,
    ).right().at((x_end + 0.8, 1.0)).anchor("G").label("Wemos D1 mini (ESP8266)", loc="top")
    d += wemos
    d += elm.Line().at(wemos["5V"]).tox(x_end)
    d += elm.Line().toy(V5_Y)
    d += elm.Line().at(wemos.G).tox(x_end - 0.6)
    d += elm.Line().toy(GND_Y)
    d += elm.Dot().at((x_end - 0.6, GND_Y))

    # --- Taster an D3 ---------------------------------------------------------
    d += elm.Line().at(wemos.D3).right(1.5)
    d += elm.Button().down().label("Taster", loc="bottom", ofst=0.2)
    d += elm.Ground()

    # --- LED-Kette an D1 ------------------------------------------------------
    d += elm.Line().at(wemos.D1).right(1.5)
    d += elm.Line().up(4.5)
    d += elm.Resistor().right().label("330 Ω", loc="top").color(OPT)

    def led(n):
        return elm.Ic(
            pins=[pin("DIN", "left"), pin("DOUT", "right"),
                  pin("", "top", anchor="VDD"), pin("", "bottom", anchor="GND")],
            size=(4.2, 1.8), edgepadW=0.3,
        ).label(f"WS2812B\n#{n}", fontsize=10)

    l1 = led(1).anchor("DIN")
    d += l1
    d += elm.Line().at(l1.DOUT).right(1)
    l2 = led(2).anchor("DIN")
    d += l2
    d += elm.Line().at(l2.DOUT).right(0.6)
    d += elm.Gap().right(1.2).label("…", fontsize=18)
    d += elm.Line().right(0.6)
    l8 = led(8).anchor("DIN")
    d += l8
    for l in (l1, l2, l8):
        d += elm.Vdd().at(l.VDD).label("+5 V")
        d += elm.Ground().at(l.GND)

    # --- Hinweise -------------------------------------------------------------
    d += elm.Label().at((0, GND_Y - 3.2)).label(
        "grau = empfohlen, im Steckbrett-Prototyp noch nicht verbaut.\n"
        "Taster: kein Widerstand nötig (INPUT_PULLUP + 10 kΩ auf dem Wemos an D3). "
        "D3 = GPIO0: beim Einschalten nicht drücken (sonst Flash-Modus).\n"
        "LEDs mit +5 V versorgen, nicht aus dem 3V3-Pin. Im fertigen Button: 8× WS2812B als Ring.",
        halign="left", valign="top", fontsize=10, color="#444444",
    )

    d.save(str(out / "schaltplan.svg"))

print("geschrieben:", out / "schaltplan.svg")
