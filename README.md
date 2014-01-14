naza-led
========

Homepage des Projektes ist http://tinue.github.io/naza-led

Ziel des Projektes ist es, an einem DJI Flamewheel F550 unter jedem Motor einen
LED Ring anzubringen und die Farben der Ringe gezielt zu steuern.

Zur Zeit (dies kann noch ändern) möchte ich über einen Dreiweg-Schalter an der
Fernsteuerung drei verschiedene Lichtmodi wählen können:

1. Fluglicht: Dabei leuchten die vorderen beiden Arme rot, und die hinteren
   vier grün.
2. Landelicht: Alle Arme leuchten weiss.
3. Naza-LED: Alle Arme leuchten bzw. blinken genau so, wie die Naza LED.

Der dritte Leuchtmodus gibt dem Projekt den Namen.

Der Prototyp, für den dieser Code geschrieben ist, besteht aus:
- Kopter DJI F550 mit Naza Flightcontroller v2
- Arduino Uno
- Ein Adafruit NeoPixel Ring mit 16 LEDs.
- Graupner Empfänger GR-16. Kanal 8 ist empfängerseitig auf Port 5 umgemappt, da
  Port 8 durch die Ausgabe des Summensignales an die Naza belegt ist.
- Viele Kabel

Ein Diagramm zur Verdrahrung folgt hoffentlich später.


Bei der endgültigen Montage am Kopter soll der Arduino Uno durch einen
Arduino Pro Mini ersetzt werden. Zudem sollen 6 Ringe - einer
pro Arm - genutzt werden.

Hier noch zwei Videos, welche verschiedene Stadien des Prototypes zeigen:
- http://youtu.be/-5hrLwAgFbE
- http://youtu.be/aNY-5sn9t78



Quellenangabe:
Die Inspiration zu diesem Projekt stammt von https://github.com/diogeneyves/dji-naza-led
Die Software für den Arduino ist aber unabhängig entwickelt worden.