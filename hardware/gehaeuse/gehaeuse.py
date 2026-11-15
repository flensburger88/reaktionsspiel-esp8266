"""ButtonSpiel – Gehäuse V5 (parametrisch, build123d).

PROTOTYP – noch nicht gedruckt/getestet, ein finales Design folgt.

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
PCB_WANDABSTAND = 5.0           # Luft hinter der Platine: Lötstellen + +Y-LED mit Kabeln
PCB_Y = INNEN_XY / 2 - PCB_WANDABSTAND - PCB_T / 2
PCB_X = 5.5
NUT_X0, NUT_X1 = -20.0, 31.0
NUT_B = PCB_T + 0.4
NUT_T_BODEN = 2.0
NUT_T_DECKE = 4.0           # gleicht Höhentoleranz aus (Platine 29–33 mm)
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

# ================= HAUBE =================
haube = Pos(0, 0, BODEN) * extrude(RectangleRounded(A, A, R_AUSSEN), INNEN_H + DECKE)
haube -= Pos(0, 0, BODEN) * extrude(RectangleRounded(INNEN_XY, INNEN_XY, R_AUSSEN - WAND), INNEN_H)

# Eckdome mit Einschmelzmuttern von unten
for x, y in ECKEN:
    haube += Pos(x, y, BODEN) * Cylinder(BOSS_D / 2, INNEN_H, align=UNTEN)
    haube -= Pos(x, y, BODEN) * Cylinder(INSERT_D / 2, INSERT_T, align=UNTEN)

# Not-Aus
haube -= Pos(0, 0, Z_DECKE) * Cylinder(TASTER_LOCH / 2, DECKE * 3)

# LEDs
for w in range(4):
    r = Rot(0, 0, 90 * w)
    tiefe = WAND - LED_FENSTER
    haube -= r * (Pos(I2 + tiefe / 2 - 0.01, 0, BODEN + LED_Z) * Rot(0, 90, 0) * Cylinder(LED_D / 2, tiefe + 0.02))
    if LED_DURCHBRUCH:
        haube -= r * (Pos(I2 + WAND / 2, 0, BODEN + LED_Z) * Rot(0, 90, 0) * Cylinder(LED_LICHT_D / 2, WAND * 2))

# Kippschalter
haube -= Pos(-A / 2 + WAND / 2, KIPP_Y, BODEN + KIPP_Z) * Rot(0, 90, 0) * Cylinder(KIPP_D / 2, WAND * 3)
if USB_SCHLITZ:
    haube -= Pos(A / 2 - WAND / 2, USB_Y, BODEN + USB_Z) * Box(WAND * 3, USB_B, USB_H)

# Lochraster: Rippe mit Nut an der Decke (Wand Richtung Wemos = -Y nur 1,2 mm)
rippe_h = Z_DECKE - (PCB_OBEN - 1.0)
rippe = Pos(0, (2.0 - 1.2) / 2, 0) * Box(NUT_L, NUT_B + 3.2, rippe_h, align=OBEN)
rippe -= Pos(0, 0, -rippe_h) * Box(NUT_L, NUT_B, NUT_T_DECKE, align=UNTEN)
# Aussparung vor der +Y-LED (Einbauraum für Plättchen + Kabel), LED sitzt bei x = 0
rippe -= Pos(-NUT_XM, 0, 0) * Box(LED_D + 6, 20, 60)
haube += Pos(NUT_XM, PCB_Y, Z_DECKE) * rippe

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
platine = Pos(PCB_X, PCB_Y, PCB_UNTEN) * Box(PCB_B, PCB_T, PCB_H, align=UNTEN)
wemos = Pos(PCB_X, PCB_Y - PCB_T / 2, PCB_UNTEN + 3) * Box(
    WEMOS[0], 14.0, WEMOS[1], align=(Align.CENTER, Align.MAX, Align.MIN))
kippschalter = Pos(-I2 + KIPP_KORPUS[0] / 2, KIPP_Y, BODEN + KIPP_Z) * Box(*KIPP_KORPUS)
leds = [Rot(0, 0, 90 * w) * (Pos(I2 + (WAND - LED_FENSTER) - 0.8, 0, BODEN + LED_Z) * Rot(0, 90, 0) * Cylinder(4.75, 1.6))
        for w in range(4)]
