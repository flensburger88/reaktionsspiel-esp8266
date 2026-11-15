"""Erzeugt die Druckdateien und prüft das Gehäuse.

    python export.py          (Python mit build123d: pip install build123d)

Schreibt in diesen Ordner:
  haube.step / haube.stl   Haube, kopfüber (Oberseite aufs Druckbett)
  boden.step / boden.stl   Bodenplatte, Außenseite unten
  _ref_*.step              Platzhalter der Bauteile in Einbaulage (nur zum Ansehen, z. B. in FreeCAD)

Prüfungen:
  1. Kein Bauteil-Platzhalter überschneidet Haube, Boden oder ein anderes Bauteil.
  2. Vor jeder LED-Tasche ist 4,5 mm Einbauraum frei (Plättchen + Kabel).
"""
from pathlib import Path
from build123d import export_step, export_stl, Rot, Pos, Box, Compound

HIER = Path(__file__).resolve().parent
ns = {}
exec((HIER / 'gehaeuse.py').read_text(encoding='utf-8'), ns)
g = type('G', (), ns)

haube_druck = Pos(0, 0, g.Z_TOP) * Rot(180, 0, 0) * g.haube
for name, teil in [('haube', haube_druck), ('boden', g.boden)]:
    export_step(teil, str(HIER / f'{name}.step'))
    export_stl(teil, str(HIER / f'{name}.stl'))
export_step(g.haube, str(HIER / '_ref_haube_montiert.step'))

bauteile = dict(taster_block=g.taster_block, batterie=g.batterie, clip=g.clip, kippschalter=g.kippschalter,
                platine=g.platine, wemos=g.wemos, leds=Compound(children=g.leds))
for name, teil in bauteile.items():
    export_step(teil, str(HIER / f'_ref_{name}.step'))

print(f'Außenmaß {g.A} x {g.A} x {g.Z_TOP} mm | gültig: Haube {g.haube.is_valid}, Boden {g.boden.is_valid}')

fehler = 0
alle = dict(haube=g.haube, boden=g.boden, **bauteile)
namen = list(alle)
for i in range(len(namen)):
    for j in range(i + 1, len(namen)):
        v = (alle[namen[i]] & alle[namen[j]]).volume
        if v > 0.01:
            fehler += 1
            print(f'  KOLLISION {namen[i]} / {namen[j]}: {v:.1f} mm³')

for w, seite in enumerate(['+X', '+Y', '-X', '-Y']):
    zone = Rot(0, 0, 90 * w) * (Pos(g.I2 - 2.25, 0, g.BODEN + g.LED_Z) * Box(4.5, 12, 12))
    im_weg = [n for n, s in alle.items() if n != 'leds' and (zone & s).volume > 0.01]
    if im_weg:
        fehler += 1
        print(f'  LED {seite}: Einbauraum blockiert durch {", ".join(im_weg)}')

print('OK – keine Probleme gefunden' if fehler == 0 else f'{fehler} Problem(e)')
