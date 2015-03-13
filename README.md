naza-led
========

Homepage des Projektes ist http://tinue.github.io/naza-led

Der Simulator ist hier beschrieben: http://blog.erzberger.ch/multikopter-led-simulator

Versionsnachweis:

- 13. März 2015
 - NazaLED kompiliert (und läuft) nun für ATtiny85, und auch wieder für ATmega328
- 1. März 2015
 - NazaLEDSimulator eingecheckt.
 - Keine Änderungen beim NazaLED
- 10. Februar 2015
 - Umgestellt auf ATtiny85 als Mikroprozessor. Arduino Pro Mini läuft nicht mehr ohne Änderungen.
 - Abfrage der Fernsteuerung via Interrupt, anstelle von Polling (pulseIn). Die Naza LED wird so auch
  bei schnellen Abfolgen (rasches grünes Blinken, Rot blinken bei Akku leer) sauber nachgezeichnet.
 - Einige Fehler behoben (z.B. waren ron und grün Input vertauscht).
- 11. April 2014
 - Änderungen von Pascal eingepflegt (vor allem Flugmodus mit Naza)
 - Weitere Farbmodi (Regenbogen, basierend auf den Adafruit "Strandtest" Beispielen)
- 19. März 2014
 - Fehler bei 4 Motoren behoben
- 18. März 2014
 - Extra-Ring funktioniert jetzt im Polizei-Modus
 - Rot/Grün war vertauscht beim Naza Licht
- 17. März 2014
 - DISCLAIMER: Einige Funktionen sind nicht getestet (Extra-Ring im Polizeilicht-Modus; Schalter für Helligkeit)
   Test wird folgen, sobald die Hardware (Patch-Kabel) eingetroffen sind.
 - Neu: Polizeilicht-Modus
 - Neu: Helligkeit via Dreiwegschalter; Poti wird nur noch für Lichtmodus gebraucht. Davon kann man jetzt 16 statt 4 einstellen.
 - Code-Cleanup v.a. bei Unterstützung für unterschiedliche Motorenzahl.
- 9. März 2014 2014
 - Fehler beseitigt: readNazaColorIndex wurde zu häufig aufgerufen, so dass sich der Wert zur Unzeit ändern konnte
 - Neu: Drei Helligkeitsstufen, statt stufenlos. Vorteil: Poti muss für volle Helligkeit nicht mehr möglichst knapp
        an den nächsten Lichtmodus herangedreht werden. Es reicht, zum nächsten Modus und dann etwas zurück zu drehen,
        was schneller geht.
 - Neu: Flugmodus sieht jetzt aus wie ein Flugzeug: Vorne rechts rot, vorne links grüs und hinten weiss (funktioniert
        nur bei Hexakopter).
 - Allgemein: Code bereinigt, lesbarer und robuster gemacht.
- Januar 2014: Erste Version
