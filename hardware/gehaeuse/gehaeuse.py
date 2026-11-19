"""ButtonSpiel – Gehäuse V6 (parametrisch, build123d).

PROTOTYP – V5 probegedruckt, V6 = waagerechte LED-Klammern statt Heißkleber,
V6b = +0,25 mm Kabelspalt an der Klammer + senkrechte Kantenführung fürs Lochraster.
V6c = Kabeldurchgang auf halber Höhe der Kantenführung (Unterkante 45° angeschrägt für den Druck).

Einteilige HAUBE (Decke mit Not-Aus + 4 Wände) trägt alles: Not-Aus, LEDs, Kippschalter,
Lochraster-Nut, 9-V-Halter. Darunter eine abnehmbare BODENPLATTE (4× M3 Senkkopf von unten
in Einschmelzmuttern der Haube). Zum Arbeiten Haube kopfüber hinlegen, Boden abschrauben.

Druck: Haube mit der Oberseite aufs Bett (offen nach oben), Bodenplatte flach (Außenseite unten).
Alle Maße in mm. Maße der Bauteile siehe README.md in diesem Ordner.

Koordinaten: Mitte = (0, 0), Unterseite Bodenplatte z = 0, Oberseite Haube z = BODEN + INNEN_H + DECKE.
  -Y-Wand: 9-V-Block (auf der Schmalseite), Clip-Ende frei zur +X-Seite
  +Y-Wand: Lochraster senkrecht, Wemos zeigt nach innen (Nut an Decke + Nut auf Bodenplatte)
  -X-Wand: Kippschalter EIN/AUS/EIN nahe +Y-Ecke; USB geschlossen (USB_SCHLITZ)
"""
from build123d import *

# ---------- Grundmaße ----------
WAND = 3.0
BODEN = 3.0             # Bodenplatte
INNEN_XY = 80.0
INNEN_H = 40.0          # Not-Aus-Korpus 35 + Luft
DECKE = 4.0             # Oberseite der Haube
R_AUSSEN = 6.0

# ---------- Not-Aus (ohne Verdrehnase) ----------
TASTER_LOCH = 24.4
TASTER_TIEFE = 35.0
TASTER_KORPUS = (38.0, 30.0)

# ---------- Schrauben: M3 Senkkopf (ISO 10642) + Einschmelzmutter ----------
BOSS_D = 9.0
INSERT_D, INSERT_T = 4.0, 6.0
SCHR_D, SENK_D = 3.4, 6.6   # Senkung 90°

# ---------- 9-V-Block ----------
BAT = (48.5, 17.5, 26.5)
BAT_X0 = -31.5
CLIP_L = 15.0
BAT_SPIEL = 0.4
BAT_FINNE_UNTEN = 8.0   # seitliche Finne reicht von der Decke bis 8 mm über Bodenplatte
# ---------- Lochraster ----------
PCB_H, PCB_T = 30.0, 1.6
PCB_B = 40.0                # nur Platzhalter – Nuten erlauben jede Breite bis NUT_L
PCB_WANDABSTAND = 6.0           # Luft hinter der Platine: Lötstellen + +Y-LED mit Kabeln
PCB_Y = INNEN_XY / 2 - PCB_WANDABSTAND - PCB_T / 2
PCB_X = 5.5
NUT_X0, NUT_X1 = -20.0, 31.0
NUT_B = PCB_T + 0.4
NUT_T_BODEN = 2.0
NUT_T_DECKE = 4.0           # gleicht Höhentoleranz aus (Platine 29–33 mm)
# Kantenführung (V6b): senkrechte U-Nut an der +X-Kante (gegenüber Kippschalter), wächst von der Decke und hängt
# am Eckdom – hält das Lochraster senkrecht, während die Bodenplatte aufgesetzt wird.
PCB_KANTE_X = 30.0          # MESSEN: x der +X-Kante des zugeschnittenen Lochrasters (Innenwand bei x = 40)
HAKEN_GRIFF = 3.0           # so weit greift die Führung über die Platinenkante
HAKEN_X1 = 33.5             # Ende am Eckdom (Einschmelzmutter bleibt frei)
HAKEN_WAND_V, HAKEN_WAND_H = 1.2, 2.0   # Wandstärke vor (Wemos-Seite) / hinter der Platine, wie die Deckenrippe
# Kabeldurchgang (V6c): Fenster auf halber Höhe durch beide Nutwände, damit die LED-Kabel hinter der Platine über
# die Platinenkante (Spalt bis zum geschlossenen Ende) nach vorn laufen. Die Unterkante steigt unter 45° zum
# geschlossenen Ende an – kopfüber gedruckt wächst der Rest der Führung so ohne freien Überhang weiter.
DURCHGANG = True
DURCHGANG_OBEN = 26.0       # Oberkante des Fensters über Bodenplatte (waagerecht, unter der Deckenrippe ab 30)
DURCHGANG_H = 8.0           # lichte Höhe am geschlossenen Ende; zur freien Kante hin wird es größer
WEMOS = (34.2, 25.6)
USB_SCHLITZ = False
USB_B, USB_H = 13.0, 9.0
USB_Y = PCB_Y - 0.8 - 11.0
USB_Z = 1.0 + 3.0 + WEMOS[1] / 2

# ---------- Kippschalter EIN/AUS/EIN ----------
KIPP_D = 6.4
KIPP_Y, KIPP_Z = 25.0, 18.0             # in -X-Wand, Z über Bodenplatte
KIPP_KORPUS = (18.0, 6.0, 15.0)         # Tiefe inkl. Lötpunkte, Breite, Höhe (gemessen)

# ---------- LEDs ----------
LED_D = 10.0
LED_LICHT_D = 5.0
LED_FENSTER = 0.8
LED_DURCHBRUCH = False
LED_Z = 33.0
LED_DICKE = 3.2             # gemessen: Platine + LED am Rand, ohne Lötpunkte
LED_LOET = 0.6              # Lötpunkte hinten (gemessen gesamt 3,8)
KABEL_D = 1.0               # Litze mit Isolierung; 6 Pads in 2 Spalten, Kabel gehen flach nach links/rechts weg
LED_ANDRUCK = 0.15          # so weit steht die Platine aus der Tasche, die Klammer drückt sie ins Fenster

# ---------- LED-Klammer (waagerechte Schiebeklammer statt Heißkleber) ----------
# Führung = Rahmen ohne Überhang: Lippe an der Decke, zwei senkrechte Stützen an den Enden (wachsen beim Druck
# von der Decke aus), dazwischen unten Boden + Lippe als Brücke. Die Klammer wird von der Seite (Kabelrichtung)
# hinter der Stütze am offenen Ende eingeschoben bis zum Anschlag. Zwei Stege drücken nur oben/unten auf den Platinenrand (dort keine Pads),
# dazwischen bleibt unter der Platte ein Spalt, durch den die Kabel seitlich hinauslaufen.
# Druckteil: 4×, flach mit der Wandseite aufs Bett.
KL_L = 14.0                 # Länge in Schieberichtung
KL_T = 1.0                  # Dicke der Platte über den Kabeln
KABEL_LUFT = 0.65           # V6b: +0,25 (Kabel ließen sich nicht sauber unter die Klammer legen)
KL_SPALT = max(LED_LOET, KABEL_D) + KABEL_LUFT  # Luft zwischen Platinenrückseite und Platte
KL_STEG_ABST = 3.5          # Stege drücken ab 3,5 mm über/unter LED-Mitte (Pads enden ~3 mm)
KL_UNTEN = LED_Z - 5.5      # Oberkante der Bodenbrücke über Bodenplatte (9-V-Block an der -Y-Wand endet bei 26,5)
BRUECKE_T = 0.8             # Dicke der Bodenbrücke
LIPPE_U_H = 1.5             # Höhe der unteren Lippe über der Brücke
STUETZE_B = 1.2             # Breite der Stützen an den Enden
KL_SPIEL = 0.2
LIPPE_T, LIPPE_H = 1.0, 2.0
ANSCHLAG_L = 1.5

# ---------- abgeleitet ----------
A = INNEN_XY + 2 * WAND
I2 = INNEN_XY / 2
Z_DECKE = BODEN + INNEN_H               # Unterseite Decke
Z_TOP = Z_DECKE + DECKE
ECK = I2 - BOSS_D / 2 + 1.0
ECKEN = [(sx * ECK, sy * ECK) for sx in (-1, 1) for sy in (-1, 1)]
UNTEN = (Align.CENTER, Align.CENTER, Align.MIN)
OBEN = (Align.CENTER, Align.CENTER, Align.MAX)
NUT_L, NUT_XM = NUT_X1 - NUT_X0, (NUT_X0 + NUT_X1) / 2
PCB_UNTEN = BODEN + 1.0
PCB_OBEN = PCB_UNTEN + PCB_H
bat_x = BAT_X0 + BAT[0] / 2
bat_y = -I2 + BAT_SPIEL + BAT[1] / 2
# LED-Tasche: endet am Fenster; ist die Platine dicker als die Wand hergibt, kommt innen ein Sockel dazu
LED_TASCHE = LED_DICKE - LED_ANDRUCK
SOCKEL = max(0.0, LED_TASCHE - (WAND - LED_FENSTER))
LED_ZM = BODEN + LED_Z                  # LED-Mitte (absolut)
U_PLATINE = SOCKEL + LED_ANDRUCK        # Platinenrückseite, gemessen von der Innenwand in den Raum (u)
U_PLATTE0 = U_PLATINE + KL_SPALT        # Unterseite Klammerplatte
U_PLATTE1 = U_PLATTE0 + KL_T            # Oberseite Klammerplatte
U_LIPPE0 = U_PLATTE1 + KL_SPIEL
HALTER_T = U_LIPPE0 + LIPPE_T           # so weit ragt der Halter von der Innenwand in den Raum
Z_BRUECKE = BODEN + KL_UNTEN            # Oberkante Bodenbrücke = Auflage der Klammer
KL_Z0, KL_Z1 = Z_BRUECKE + KL_SPIEL, Z_DECKE - KL_SPIEL
V_ANSCHLAG = -KL_L / 2 - KL_SPIEL       # Anschlag auf der -v-Seite, eingeschoben wird von +v
V_OFFEN = KL_L / 2 + KL_SPIEL           # innere Kante der Stütze am offenen Ende
HALTER_V0 = V_ANSCHLAG - max(ANSCHLAG_L, STUETZE_B)
HALTER_V1 = V_OFFEN + STUETZE_B


def quader(u0, u1, v0, v1, z0, z1):
    """Quader in +X-Wand-Lage: u = Abstand von der Innenwand in den Raum, v = entlang der Wand (= y)."""
    return Pos(I2 - (u0 + u1) / 2, (v0 + v1) / 2, (z0 + z1) / 2) * Box(u1 - u0, v1 - v0, z1 - z0)


def led_halter():
    """Sockel + Rahmen (Lippe oben, Stützen an den Enden, Boden + Lippe unten) + Anschlag – +X-Wand-Lage.

    Druck kopfüber: Lippe, Anschlag und Stützen wachsen von der Decke aus, die untere Lippe und der Boden
    spannen als Brücke zwischen den Stützen bzw. zwischen Wand und Lippe – kein freier Überhang.
    """
    zb = Z_BRUECKE - BRUECKE_T
    teile = [
        quader(U_LIPPE0, HALTER_T, HALTER_V0, HALTER_V1, Z_DECKE - LIPPE_H, Z_DECKE + 0.01),          # Lippe oben
        quader(SOCKEL - 0.01, U_LIPPE0 + 0.01, HALTER_V0, V_ANSCHLAG, Z_DECKE - LIPPE_H, Z_DECKE + 0.01),  # Anschlag
        quader(U_LIPPE0, HALTER_T, HALTER_V0, V_ANSCHLAG, zb, Z_DECKE + 0.01),                         # Stütze Anschlag
        quader(U_LIPPE0, HALTER_T, V_OFFEN, HALTER_V1, zb, Z_DECKE + 0.01),                            # Stütze offen
        quader(U_LIPPE0, HALTER_T, HALTER_V0, HALTER_V1, zb, Z_BRUECKE + LIPPE_U_H),                   # Lippe unten
        quader(-0.01, HALTER_T, HALTER_V0, HALTER_V1, zb, Z_BRUECKE),                                  # Boden
    ]
    if SOCKEL > 0:
        teile.append(quader(-0.01, SOCKEL, HALTER_V0, HALTER_V1, zb, Z_DECKE + 0.01))
    halter = teile[0]
    for t in teile[1:]:
        halter += t
    return halter


def led_tasche():
    """Tasche Ø LED_D vom Fenster bis zur Sockel-/Wandfläche, +X-Lage."""
    x_in = I2 - SOCKEL
    return Pos(x_in + LED_TASCHE / 2 - 0.01, 0, LED_ZM) * Rot(0, 90, 0) * Cylinder(LED_D / 2, LED_TASCHE + 0.02)


def led_klammer():
    """Waagerechte Schiebeklammer in Einbaulage an der +X-Wand."""
    v0, v1 = -KL_L / 2, KL_L / 2
    platte = quader(U_PLATTE0, U_PLATTE1, v0, v1, KL_Z0, KL_Z1)
    steg_o = quader(SOCKEL, U_PLATTE0 + 0.01, v0, v1, LED_ZM + KL_STEG_ABST, KL_Z1)
    steg_u = quader(SOCKEL, U_PLATTE0 + 0.01, v0, v1, KL_Z0, LED_ZM - KL_STEG_ABST)
    k = platte + steg_o + steg_u
    # Einführschrägen an beiden Enden der Stege (Wandseite) – gleiten über den Platinenrand
    a = 0.5 * 2 ** 0.5
    for v in (v0, v1):
        k -= Pos(I2 - SOCKEL, v, (KL_Z0 + KL_Z1) / 2) * Rot(0, 0, 45) * Box(a, a, 30)
    # Loch zum Herausschieben (Nadel), nahe dem Einschub-Ende
    k -= Pos(I2 - (U_PLATTE0 + U_PLATTE1) / 2, v1 - 2.0, LED_ZM) * Rot(0, 90, 0) * Cylinder(0.8, KL_T * 3)
    return k


# ================= HAUBE =================
haube = Pos(0, 0, BODEN) * extrude(RectangleRounded(A, A, R_AUSSEN), INNEN_H + DECKE)
haube -= Pos(0, 0, BODEN) * extrude(RectangleRounded(INNEN_XY, INNEN_XY, R_AUSSEN - WAND), INNEN_H)

# Eckdome mit Einschmelzmuttern von unten
for x, y in ECKEN:
    haube += Pos(x, y, BODEN) * Cylinder(BOSS_D / 2, INNEN_H, align=UNTEN)
    haube -= Pos(x, y, BODEN) * Cylinder(INSERT_D / 2, INSERT_T, align=UNTEN)

# Not-Aus
haube -= Pos(0, 0, Z_DECKE) * Cylinder(TASTER_LOCH / 2, DECKE * 3)

# Kippschalter
haube -= Pos(-A / 2 + WAND / 2, KIPP_Y, BODEN + KIPP_Z) * Rot(0, 90, 0) * Cylinder(KIPP_D / 2, WAND * 3)
if USB_SCHLITZ:
    haube -= Pos(A / 2 - WAND / 2, USB_Y, BODEN + USB_Z) * Box(WAND * 3, USB_B, USB_H)

# Lochraster: Rippe mit Nut an der Decke (Wand Richtung Wemos = -Y nur 1,2 mm)
rippe_h = Z_DECKE - (PCB_OBEN - 1.0)
rippe = Pos(0, (2.0 - 1.2) / 2, 0) * Box(NUT_L, NUT_B + 3.2, rippe_h, align=OBEN)
rippe -= Pos(0, 0, -rippe_h) * Box(NUT_L, NUT_B, NUT_T_DECKE, align=UNTEN)
# Aussparung vor der +Y-LED (Halter), LED sitzt bei x = 0; Halter-v läuft dort in -x-Richtung
rippe -= Pos(-NUT_XM - (HALTER_V0 + HALTER_V1) / 2, 0, 0) * Box(HALTER_V1 - HALTER_V0 + 2, 20, 60)
haube += Pos(NUT_XM, PCB_Y, Z_DECKE) * rippe

# Lochraster: senkrechte Kantenführung (U-Nut) an der +X-Kante, von der Decke bis knapp über die Bodenleiste
haken_x0 = PCB_KANTE_X - HAKEN_GRIFF
haken_z0 = BODEN + NUT_T_BODEN + 1.0 + 0.8          # über Nutleiste der Bodenplatte
haken_y0, haken_y1 = PCB_Y - NUT_B / 2 - HAKEN_WAND_V, PCB_Y + NUT_B / 2 + HAKEN_WAND_H
haken = Pos((haken_x0 + HAKEN_X1) / 2, (haken_y0 + haken_y1) / 2, (haken_z0 + Z_DECKE + 0.01) / 2) * Box(
    HAKEN_X1 - haken_x0, haken_y1 - haken_y0, Z_DECKE + 0.01 - haken_z0)
haken_nut_x1 = HAKEN_X1 - 1.2                       # geschlossenes Ende (Steg zum Dom)
haken -= Pos((haken_x0 - 1 + haken_nut_x1) / 2, PCB_Y, (haken_z0 + Z_DECKE) / 2) * Box(
    haken_nut_x1 - haken_x0 + 1, NUT_B, Z_DECKE - haken_z0 + 2)
# Einführschrägen unten an der Nut
for s in (-1, 1):
    haken -= Pos(haken_x0 + 0.5 * (HAKEN_X1 - haken_x0), PCB_Y + s * NUT_B / 2, haken_z0) * Rot(45, 0, 0) * Box(
        HAKEN_X1 - haken_x0 + 2, 0.8, 0.8)
# Kabeldurchgang: Fenster x haken_x0 … haken_nut_x1 (geschlossenes Ende bleibt durchgehend = Stütze beim Druck).
# Profil in der x-z-Ebene: Oberkante waagerecht, Unterkante 45° fallend von der Stütze zur freien Kante.
if DURCHGANG:
    z_o = BODEN + DURCHGANG_OBEN
    z_u1 = z_o - DURCHGANG_H                         # Unterkante am geschlossenen Ende
    z_u0 = z_u1 - (haken_nut_x1 - (haken_x0 - 1))   # 45° bis 1 mm vor der freien Kante
    fenster_xz = Polygon((haken_x0 - 1, z_o), (haken_x0 - 1, z_u0), (haken_nut_x1, z_u1), (haken_nut_x1, z_o),
                         align=None)
    tiefe = haken_y1 - haken_y0 + 2
    # Polygon liegt in XY → um X drehen (XY → XZ), dann in y über die ganze Führung extrudieren
    fenster = Pos(0, (haken_y0 + haken_y1) / 2 + tiefe / 2, 0) * Rot(90, 0, 0) * extrude(fenster_xz, tiefe)
    haken -= fenster
haube += haken

# LEDs: Halter mit Schienen, Tasche, ggf. Lichtloch
for w in range(4):
    r = Rot(0, 0, 90 * w)
    haube += r * led_halter()
    haube -= r * led_tasche()
    if LED_DURCHBRUCH:
        haube -= r * (Pos(I2 + WAND / 2, 0, LED_ZM) * Rot(0, 90, 0) * Cylinder(LED_LICHT_D / 2, WAND * 2))

# 9-V: seitliche Finne (Richtung Taster) + Niederhalter, beide von der Decke
finne_y = -I2 + BAT[1] + 2 * BAT_SPIEL + 1.0
haube += Pos(bat_x - 4, finne_y, Z_DECKE) * Box(30, 2.0, Z_DECKE - BODEN - BAT_FINNE_UNTEN, align=OBEN)
haube += Pos(bat_x - 4, bat_y, Z_DECKE) * Box(26, 2.0, Z_DECKE - (BODEN + BAT[2] + 0.5), align=OBEN)

# ================= BODENPLATTE =================
boden = extrude(RectangleRounded(A, A, R_AUSSEN), BODEN)
# Zentrierrand (2 mm in die Haube), an den Domen ausgespart
rand = extrude(RectangleRounded(INNEN_XY - 0.6, INNEN_XY - 0.6, R_AUSSEN - WAND), 2) - extrude(
    RectangleRounded(INNEN_XY - 4.6, INNEN_XY - 4.6, R_AUSSEN - WAND - 1), 2)
for x, y in ECKEN:
    rand -= Pos(x, y, 0) * Box(BOSS_D + 4, BOSS_D + 4, 10)
# unter Batterie + Clip aussparen (Batterie liegt direkt an der -Y-Wand)
bat_bereich = BAT[0] + CLIP_L + 2
rand -= Pos(BAT_X0 - 1 + bat_bereich / 2, -I2 + 3, 0) * Box(bat_bereich, 6, 10)
boden += Pos(0, 0, BODEN) * rand
# Nutleiste für die Platinen-Unterkante
leiste = Box(NUT_L, NUT_B + 4.0, NUT_T_BODEN + 1.0, align=UNTEN)
leiste -= Pos(0, 0, 1.0) * Box(NUT_L, NUT_B, NUT_T_BODEN, align=UNTEN)
boden += Pos(NUT_XM, PCB_Y, BODEN) * leiste
# kleine Leiste vor der Batterie (rutscht nicht Richtung Taster)
boden += Pos(bat_x - 4, finne_y, BODEN) * Box(30, 2.0, 4.0, align=UNTEN)
# Senkschrauben
for x, y in ECKEN:
    boden -= Pos(x, y, 0) * Cylinder(SCHR_D / 2, BODEN * 3)
    boden -= Pos(x, y, 0) * Cone(SENK_D / 2, SCHR_D / 2, (SENK_D - SCHR_D) / 2, align=UNTEN)
# Gummifuß-Mulden
for x, y in ECKEN:
    boden -= Pos(x * 0.72, y * 0.72, 0) * Cylinder(5.5, 1.0, align=UNTEN)

# ================= Platzhalter =================
taster_block = Pos(0, 0, Z_DECKE - TASTER_TIEFE) * Box(*TASTER_KORPUS, TASTER_TIEFE, align=UNTEN)
batterie = Pos(bat_x, bat_y, BODEN) * Box(*BAT, align=UNTEN)
clip = Pos(BAT_X0 + BAT[0] + CLIP_L / 2, bat_y, BODEN) * Box(CLIP_L, BAT[1], BAT[2], align=UNTEN)
pcb_x0 = PCB_X - PCB_B / 2                          # linke Kante wie bisher, rechte Kante = PCB_KANTE_X
platine = Pos((pcb_x0 + PCB_KANTE_X) / 2, PCB_Y, PCB_UNTEN) * Box(PCB_KANTE_X - pcb_x0, PCB_T, PCB_H, align=UNTEN)
wemos = Pos(PCB_X, PCB_Y - PCB_T / 2, PCB_UNTEN + 3) * Box(
    WEMOS[0], 14.0, WEMOS[1], align=(Align.CENTER, Align.MAX, Align.MIN))
kippschalter = Pos(-I2 + KIPP_KORPUS[0] / 2, KIPP_Y, BODEN + KIPP_Z) * Box(*KIPP_KORPUS)
leds = [Rot(0, 0, 90 * w) * (Pos(I2 + (WAND - LED_FENSTER) - LED_DICKE / 2, 0, LED_ZM) * Rot(0, 90, 0)
                              * Cylinder(4.75, LED_DICKE)) for w in range(4)]
# Lötpunkte + Kabel hinter der Platine, flach nach links/rechts (je 3 Adern übereinander)
kabel = [Rot(0, 0, 90 * w) * quader(U_PLATINE, U_PLATINE + max(LED_LOET, KABEL_D) + 0.1,
                                    -KL_L / 2 - 4, KL_L / 2 + 4, LED_ZM - 3.0, LED_ZM + 3.0) for w in range(4)]

# ================= LED-Klammern (eigenes Druckteil, 4×) =================
klammer = led_klammer()
klammern = [Rot(0, 0, 90 * w) * klammer for w in range(4)]
