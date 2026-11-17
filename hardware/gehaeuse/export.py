"""Erzeugt die Druckdateien und prüft das Gehäuse.

    python export.py          (Python mit build123d: pip install build123d)

Schreibt in diesen Ordner:
  haube.step / haube.stl       Haube, kopfüber (Oberseite aufs Druckbett)
  boden.step / boden.stl       Bodenplatte, Außenseite unten
  klammer.step / klammer.stl   LED-Klammer (4× drucken), Wandseite unten
  _ref_*.step                  Platzhalter der Bauteile in Einbaulage (nur zum Ansehen, z. B. in FreeCAD)

Prüfungen:
  1. Kein Bauteil-Platzhalter überschneidet Haube, Boden, Klammern oder ein anderes Bauteil.
     Ausnahme: Klammer und LED-Platine überlappen um LED_ANDRUCK – so wird die Platine angedrückt.
     Der Platzhalter „kabel“ (Lötpunkte + seitlich abgehende Kabel) darf nichts berühren.
  2. Neben jedem LED-Halter ist auf der Einschubseite eine Klammerlänge frei.
"""
from pathlib import Path
from build123d import export_step, export_stl, Rot, Pos, Box, Compound

HIER = Path(__file__).resolve().parent
ns = {}
exec((HIER / 'gehaeuse.py').read_text(encoding='utf-8'), ns)
g = type('G', (), ns)

haube_druck = Pos(0, 0, g.Z_TOP) * Rot(180, 0, 0) * g.haube
klammer_druck = Rot(0, 90, 0) * g.klammer          # Wandseite (+X) nach unten
bb = klammer_druck.bounding_box()
klammer_druck = Pos(-bb.center().X, -bb.center().Y, -bb.min.Z) * klammer_druck
for name, teil in [('haube', haube_druck), ('boden', g.boden), ('klammer', klammer_druck)]:
    export_step(teil, str(HIER / f'{name}.step'))
    export_stl(teil, str(HIER / f'{name}.stl'))
export_step(g.haube, str(HIER / '_ref_haube_montiert.step'))
export_step(Compound(children=g.klammern), str(HIER / '_ref_klammern_montiert.step'))

bauteile = dict(taster_block=g.taster_block, batterie=g.batterie, clip=g.clip, kippschalter=g.kippschalter,
                platine=g.platine, wemos=g.wemos, leds=Compound(children=g.leds),
                kabel=Compound(children=g.kabel))
for name, teil in bauteile.items():
    export_step(teil, str(HIER / f'_ref_{name}.step'))

print(f'Außenmaß {g.A} x {g.A} x {g.Z_TOP} mm | gültig: Haube {g.haube.is_valid}, Boden {g.boden.is_valid}, '
      f'Klammer {g.klammer.is_valid}')
print(f'LED: Tasche {g.LED_TASCHE:.2f} mm, Sockel {g.SOCKEL:.2f} mm, Halter ragt {g.HALTER_T:.2f} mm in den Raum')
if g.LED_TASCHE < g.WAND - g.LED_FENSTER:
    print(f'  Hinweis: Plättchen dünn – Fenster wird {g.WAND - g.LED_TASCHE:.2f} mm statt {g.LED_FENSTER} mm')

fehler = 0
alle = dict(haube=g.haube, boden=g.boden, klammern=Compound(children=g.klammern), **bauteile)
namen = list(alle)
for i in range(len(namen)):
    for j in range(i + 1, len(namen)):
        v = (alle[namen[i]] & alle[namen[j]]).volume
        if {namen[i], namen[j]} == {'klammern', 'leds'}:
            print(f'  Andruck Klammer / LED: {v:.2f} mm³ (gewollt, {g.LED_ANDRUCK} mm)')
            continue
        if v > 0.01:
            fehler += 1
            print(f'  KOLLISION {namen[i]} / {namen[j]}: {v:.1f} mm³')

for w, seite in enumerate(['+X', '+Y', '-X', '-Y']):
    zone = Rot(0, 0, 90 * w) * g.quader(g.SOCKEL + 0.05, g.U_PLATTE1, g.KL_L / 2 + 0.5, 1.5 * g.KL_L + 1,
                                        g.KL_Z0 + 0.5, g.KL_Z1)
    im_weg = [n for n, s in alle.items() if n != 'kabel' and (zone & s).volume > 0.01]
    if im_weg:
        fehler += 1
        print(f'  LED {seite}: Einschubweg blockiert durch {", ".join(im_weg)}')

print('OK – keine Probleme gefunden' if fehler == 0 else f'{fehler} Problem(e)')
