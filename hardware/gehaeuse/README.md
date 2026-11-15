# Gehäuse (3D-Druck)

> **Prototyp – das finale Design folgt.**
> Dieser Stand (V5) ist noch nicht gedruckt und nicht getestet. Maße und Halter können sich nach dem ersten Druck noch ändern.

![Gehäuse V5, Boden abgenommen](../../docs/img/gehaeuse-v5.png)

*Haube transparent, Boden nach unten weggezogen. Grün: Kontaktblock des Not-Aus, gelb: 9-V-Block (schwarz: Platz für den Clip),
blau: Wemos auf dem Lochraster und Kippschalter, rot: die vier LEDs.*

## Aufbau
Das Gehäuse besteht aus zwei Teilen:

- **Haube** (Decke und vier Wände in einem Stück). Sie trägt alles:
  - den Not-Aus-Pilztaster in der Decke (Loch Ø 24,4 mm),
  - je eine WS2812-LED in jeder Seitenwand: eine Tasche von innen, vor der LED bleiben 0,8 mm Wand stehen,
  - den Kippschalter in einer Seitenwand (Loch Ø 6,4 mm),
  - eine Rippe mit Nut an der Decke, die die Oberkante des Lochrasters hält,
  - zwei Rippen, die den 9-V-Block seitlich und von oben halten.
- **Bodenplatte**, abnehmbar mit 4 Senkkopfschrauben von unten. Darauf liegen die untere Nut für das Lochraster und eine kleine Leiste vor der Batterie.

Zum Arbeiten die Haube kopfüber hinlegen und den Boden abschrauben. Dann liegt alles offen, und die Kabel zum Not-Aus müssen nicht gelöst werden.

Außenmaß: **86 × 86 × 47 mm** (innen 80 × 80 × 40 mm).

## Druck
| Datei | Ausrichtung |
|---|---|
| `haube.step` / `haube.stl` | Oberseite auf dem Druckbett, kein Support |
| `boden.step` / `boden.stl` | Außenseite unten, kein Support |

PrusaSlicer liest STEP direkt; das gibt glattere Rundungen als STL.

Bewährte Einstellungen für PLA:
- Schichthöhe 0,2 mm
- 4 Umfänge
- 5 obere und untere Massivschichten
- 30 % Gyroid als Füllung

Damit die LEDs durch die 0,8-mm-Wand leuchten, braucht es **weißes oder naturfarbenes** Filament. Bei farbigem Filament im
Skript `LED_DURCHBRUCH = True` setzen. Dann bekommt jede LED ein 5-mm-Loch nach außen.

## Teile
| Teil | Maß / Hinweis |
|---|---|
| Not-Aus-Pilztaster (Typ LAY37) | Gewinde Ø 24 mm, Kontaktblock 38 × 30 mm, 35 mm tief unter der Decke, ohne Verdrehnase |
| 9-V-Block mit Clip | liegt auf der Schmalseite an der Wand, am Kontakt-Ende sind 15 mm frei für den Clip |
| Lochraster mit Wemos D1 mini | steht senkrecht in zwei Nuten; Breite beliebig bis ca. 50 mm, Höhe 29–33 mm; der Wemos zeigt nach innen |
| Kippschalter EIN/AUS/EIN | Gewinde 6 mm, Korpus 6 × 15 × 18 mm (inkl. Lötpunkte) |
| 4× WS2812B-Plättchen | Ø 9,5 mm, Leuchtfläche Ø 5 mm |
| 4× Einschmelzmutter M3 | Loch Ø 4,0 × 6 mm (z. B. M3 × 5,7), von unten in die Eckdome |
| 4× Senkkopfschraube M3 × 8 (ISO 10642) | halten den Boden |
| 4× Gummifuß Ø 10 mm | optional, Mulden in der Bodenplatte |

**Reihenfolge beim Zusammenbau:** erst die LEDs in die Taschen setzen und mit etwas Heißkleber fixieren, dann Lochraster und
Batterie einsetzen und zuletzt den Boden aufschrauben.

Der USB-Anschluss des Wemos ist im Gehäuse nicht erreichbar. Zum Flashen den Boden abnehmen und den Wemos abziehen.
Wer einen Schlitz in der Wand möchte, setzt im Skript `USB_SCHLITZ = True`.

## Anpassen
Das Gehäuse ist parametrisch in [build123d](https://github.com/gumyr/build123d) (Python) beschrieben. Alle Maße stehen
oben in `gehaeuse.py`.

```
pip install build123d
python export.py
```

`export.py` erzeugt die Druckdateien neu und prüft dabei:
- ob sich Bauteile überschneiden, gegen Platzhalter mit den Maßen aus der Tabelle oben,
- ob vor jeder LED-Tasche genug Platz zum Einsetzen bleibt.

Die Dateien `_ref_*.step` sind diese Platzhalter in Einbaulage. Mit ihnen lässt sich das Gehäuse z. B. in FreeCAD
zusammen mit den Bauteilen ansehen.

## Entwicklung
Wie das Gehäuse in fünf Schritten entstanden ist:

1. **V1:** stehende Box mit dem Taster vorne. Zu hoch, weil die alte Box beim Fotografieren nur hochkant stand.
2. **V2:** flache Box nach den Maßen der alten Kaufbox (innen 78 × 78 × 37 mm), LEDs in den Seitenwänden.
3. **V3:** Das Lochraster wird nach Bedarf zugeschnitten. Deshalb halten es Nuten statt Schienen fester Breite. Der 9-V-Block
   liegt nicht mehr in einer Wanne, damit der Clip Platz hat.
4. **V4:** Lochraster an der Wand, der Wemos zeigt nach innen; USB-Loch geschlossen; Kippschalter Richtung Ecke versetzt.
5. **V5:** umgedreht. Alles hängt an der Haube, der Boden lässt sich abnehmen. Im Slicer fiel noch auf, dass die Nut-Rippe nur
   1,3 mm vor einer LED-Tasche lag. Die Rippe ist dort jetzt ausgespart, und `export.py` prüft den Einbauraum vor den LEDs.
