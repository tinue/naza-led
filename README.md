naza-led
========

Homepage des Projektes ist http://tinue.github.io/naza-led

Versionsnachweis:
- 18. März
 - Extra-Ring funktioniert jetzt im Polizei-Modus
 - Rot/Grün war vertauscht beim Naza Licht
- 17. März
 - DISCLAIMER: Einige Funktionen sind nicht getestet (Extra-Ring im Polizeilicht-Modus; Schalter für Helligkeit)
   Test wird folgen, sobald die Hardware (Patch-Kabel) eingetroffen sind.
 - Neu: Polizeilicht-Modus
 - Neu: Helligkeit via Dreiwegschalter; Poti wird nur noch für Lichtmodus gebraucht. Davon kann man jetzt 16 statt 4 einstellen.
 - Code-Cleanup v.a. bei Unterstützung für unterschiedliche Motorenzahl.
- 9. März 2014
 - Fehler beseitigt: readNazaColorIndex wurde zu häufig aufgerufen, so dass sich der Wert zur Unzeit ändern konnte
 - Neu: Drei Helligkeitsstufen, statt stufenlos. Vorteil: Poti muss für volle Helligkeit nicht mehr möglichst knapp
        an den nächsten Lichtmodus herangedreht werden. Es reicht, zum nächsten Modus und dann etwas zurück zu drehen,
        was schneller geht.
 - Neu: Flugmodus sieht jetzt aus wie ein Flugzeug: Vorne rechts rot, vorne links grüs und hinten weiss (funktioniert
        nur bei Hexakopter).
 - Allgemein: Code bereinigt, lesbarer und robuster gemacht.

- Januar 2014: Erste Version
