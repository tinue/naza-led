/*
 * Steuerprogramm fuer Adafruit Neopixels am Multikopter.
 * Mikrocontroller ist der ATtiny85. Grundstzlich funktioniert das Programm auch auf anderen
 * ATMEL Mikrocontrollern, aber es muss angepasst werden (z.B. Name der Interrupt Service Routine,
 * oder die Maskenbits fuer das Steuern der Interrupts).
 *
 * Das Programm unterstuetzt verschiedene Leucht-Modi (Regenbogen, Polizeilicht, etc.)
 *
 * Bei einigen der Modi blinken auch LEDs im Takt und Farbe der Naza-LED
 *
 * Die Zusatz-LED (z.B. unten am Kopter angebracht) leuchtet in allen Modi synchron zur Naza LED.
 *
 * Gesteuert wird der Leuchtmodus via Potentiometer an einem freien Kanal der Fernsteuerung.
 * - Ganz links: Lichter aus (ausser Zusatz-LED)
 * - Drehen nach rechts schaltet weiter um zwischen bis zu 16 Programmen
 *
 * Die Helligkeit der LEDs kann auf einen Schalter der Fernsteuerung gelegt werden.
 *
*/

// Diese Konstanten müssen auf die eigene Konfiguration hin angepasst werden
#include <Adafruit_NeoPixel.h>
#include "avr/interrupt.h"
#define NAZARED 3 // Rotes LED Signal der Naza. ATtiny85 Pin 2
#define NAZAGREEN 2 // Gruenes LED Signal der Naza. ATTiny85 Pin 3
#define REMOTEROTARY 0 // Arduino-Eingang für den Kanal der Fernsteuerung (Potentiometer). ATtiny85 Pin 5
#define REMOTESWITCH 1 // Arduino-Eingang für den 2. Kanal der Fernsteuerung (Schalter). ATtiny85 Pin 6
#define LEDOUT 2 // Datenleitung der LED Ringe. ATtiny85 Pin 7
#define MOTORS 4 // Anzahl Motoren (Annahme: Pro Motor jeweils gleichviel LEDs)
#define LEDPERMOTOR 16 // Anzahl LEDs pro Motor
#define EXTRALEDS 16 // Anzahl zusätzlicher LEDs am Ende (nach den LEDs der Motoren)
#define MAXBRIGHTNESS 255 // Anpassen an Leistung der Stromversorgung! 255 = Anzahl LEDs mal 0.06A. Gilt aber nur, wenn alle LEDs weiss leuchten.
#define MINPULSE 984 // Minimale Pule-Länge am Empfänger in Mikrosekunden, muss ev. angepasst werden,
#define MAXPULSE 2003 // Maximale Puls-Länge am Empfänger in Mikrosekunden. Beides gemessen an Frsky Taranis mit X8R Empfänger
#define LOWERSWITCH 1200
#define HIGHERSWITCH 1700 // Grenzen für Bestimmung der Schalterstellung

// Ein paar Konstanten zur besseren Lesbarkeit
#define LIGHTOFF 0 // Licht aus
#define LIGHTNAZA 1 // Naza LED Modus
#define LIGHTFLIGHT 2 // Flugmodus
#define LIGHTFLIGHTNAZA 3 // Naza/Flug gemischt
#define LIGHTPOLICE 4 // Polizeilicht
#define LIGHTRAINBOWA 5 // Regenbogen
#define LIGHTRAINBOWB 6 // Regenbogen Variante 2
#define LIGHTTHEATERCHASERAINBOW 7 // Lauflicht
#define LIGHTLAND 8 // Landelicht
#define BLACK 0x000000 // Schwarz
#define PWHITE 0x606060 // dunkles Weiss
#define DIMWHITE 0x808080 // fast helles Weiss
#define RED  0xFF0000 // Rot
#define BLUE 0x0000FF // Blau
#define GREEN 0x00FF00 // Grün
#define YELLOW 0xFFA500 // Gelb
#define WHITE 0xFFFFFF // Weiss

// "pixels" sind alle LEDs, zusammengehängt. Der erste Paremeter gibt die Anzahl LEDs an
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(MOTORS * LEDPERMOTOR + EXTRALEDS, LEDOUT, NEO_GRB + NEO_KHZ800);
// Die vier möglichen Farben der Naza LED
static const unsigned long int nazaLedColors[] = { BLACK, GREEN, RED, YELLOW };
// Globale Variablen
static byte lastLightMode = 99; // Letzte Werte merken. Dies, damit nur bei einer Aenderung neu gemalt werden muss
static byte lastBrightness = 99;
// Die naechsten Variablen werden in der Interrupt Service Routine gendert, deshalb sind sie global und volatile
volatile static unsigned int rotaryPulse = 0;
volatile static unsigned int switchPulse = 0;
static boolean ignoreInterrupts = false;  // Setzen, solange Interrupts ignoriert werden muessen (wird in ISR geprueft)

void setup() {
  pixels.begin();
  pixels.setBrightness(MAXBRIGHTNESS);
  showPixels(); // LEDs schwarz zu Beginn
  // Die folgenden Zeilen gelten nur fuer ATtiny85! Andere Mikrocontroller benoetigen andere Masken.
  GIMSK = 0b00100000;    // ATtiny85: "pin change interrupts" aktivieren
  PCMSK = 0b00000011;    // Fuer PB0 und PB1 (Fernsteuerung)
}

void loop() {
  static byte lastColorIndex = 99;

  // Lesen des Potis (0-15)
  byte lightMode = readLightMode();
  // Helligkeit bestimmen
  byte brightness = readBrightness();
  // Helligkeit schon mal setzen
  pixels.setBrightness(brightness);

  // Aktion nun je nach Lichtmodus
  // Es gibt zwei Sorten von Lichtmodi:
  // - Statisch: Die LEDs sind immer gleich (z.B. alle weiss, oder links rot, rechts gruen und hinten weiss)
  // - Dynamisch: Die LEDs durchlaufen eine Sequenz (z.B. Regenbogen oder Polizeilicht). Die Sequenz kann eher kurz sein
  //              (Polizeilicht, Lauflicht) oder mehrere Sekunden dauern (Regenbogen)
  // Zunaechst die aktuelle Farbe der Naza LED bestimmen
  unsigned long int colorIndex = readNazaColorIndex();
  // Jetzt pruefen, ob einer der aktiven Modi eingestellt ist. Wenn ja, eine Sequenz durchlaufen.
  // Bei allen aktiven Modi werden die Zusatz-LEDs vom Modus selber gemalt. Dies deshalb, weil sich der Naza LED
  // Status waehrend dem Durchlaufen von einer Sequenz mehrmals aendern kann.
  if (lightMode == LIGHTPOLICE) {
    paintPoliceLights();
  } else if (lightMode == LIGHTRAINBOWA) {
    paintRainbow();
  } else if (lightMode == LIGHTRAINBOWB) {
    paintRainbowCycle();
  } else if (lightMode == LIGHTTHEATERCHASERAINBOW) {
    paintTheaterChaseRainbow();
  } else if (checkChangedLightMode() || lastColorIndex != colorIndex || lastBrightness != brightness) {
    // Bei den statischen Modi wird nur gemalt, wenn sich gegenüber der letzten Runde etwas geändert hat.
    if (lightMode == LIGHTOFF) {
      paintAllMotors(BLACK);
    } else if (lightMode == LIGHTNAZA) {
      paintAllMotors(nazaLedColors[colorIndex]);
    } else if (lightMode == LIGHTFLIGHT) {
      paintFlightLights();
    } else if (lightMode == LIGHTFLIGHTNAZA) {
      paintLeftMotors(RED);
      paintRightMotors(GREEN);
      paintBackMotors(nazaLedColors[colorIndex]);
    } else if (lightMode == LIGHTLAND) {
      paintAllMotors(WHITE);
    } else {
      // Unbekannter Lichtmodus -> Schwarz.
      paintAllMotors(BLACK);
    }
    // Zuletzt die Zusatz-LEDs malen, und alle LEDs anzeigen
    paintExtraPixels(colorIndex);
    showPixels();
  }
  // Letzte Modi merken für die nächste Runde
  lastColorIndex = colorIndex;
  lastBrightness = brightness;
}


// Hilfsfunktionen zum Malen der Ringe

/*
* Malen aller LEDs eines Motors mit derselben Farbe
*/
void paintMotor(byte motor, unsigned long int color) {
  unsigned int startIndex = (motor - 1) * LEDPERMOTOR;
  for (uint16_t i = startIndex; i < startIndex + LEDPERMOTOR; i++) {
    pixels.setPixelColor(i, color);
  }
}

/*
 * Male sämtliche LEDs mit derselben Farbe
*/
void paintAllMotors(unsigned long int color) {
  for (int motor = 1; motor <= MOTORS; motor++) {
    paintMotor(motor, color);
  }
}

// Hilfsfunktionen zum Malen von mehreren Motoren
// Achtung: Funktionieren nur für X bzw. V-Konfiguration!

/*
 * Male die rechten voreren Motoren
*/
void paintRightMotors(unsigned long int color) {
  if (MOTORS == 8) {
    paintMotor(1, color);
    paintMotor(8, color);
  } else if (MOTORS == 6) {
    paintMotor(1, color);
    paintMotor(6, color);
  } else if (MOTORS == 4) {
    paintMotor(1, color);
  }
}

/*
 * Male die linken voreren Motoren
*/
void paintLeftMotors(unsigned long int color) {
  if (MOTORS == 8 || MOTORS == 6) {
    paintMotor(2, color);
    paintMotor(3, color);
  } else if (MOTORS == 4) {
    paintMotor(2, color);
  }
}

/*
 * Male die hinteren Motoren
*/
void paintBackMotors(unsigned long int color) {
  if (MOTORS == 8) {
    paintMotor(4, color);
    paintMotor(5, color);
    paintMotor(6, color);
    paintMotor(7, color);
  } else if (MOTORS == 6) {
    paintMotor(4, color);
    paintMotor(5, color);
  } else if (MOTORS == 4) {
    paintMotor(3, color);
    paintMotor(4, color);
  }
}


/*
 * Male Flugmuster (vorne rot, hinten grün)
*/
void paintFlightLights() {
  // Motoren vorne links rot malen
  paintLeftMotors(RED);
  // Motoren vorne rechts grün malen
  paintRightMotors(GREEN);
  // Hintere Motoren weiss malen
  paintBackMotors(WHITE);
}

/*
 * Male Polizeigeblinke
 * Die ganze Routine dauert 0.5 Sekunden. In der Zeit wird kein Signal der Fernbedienung gelesen!
 * Es kann also eine kleine Verzögerung geben, wenn der Lichtmodus gewechselt werden soll.
*/
void paintPoliceLights() {
  // Polizeimodus. Rechts zweimal blau blinken, 50ms Pause zwischen hell/dunkel. Dann links rot dasselbe. Hinten weiss blinken, etwas langsamer

  // Vorne rechts blau, sonst schwarz
  paintLeftMotors(BLACK);
  paintRightMotors(BLUE);
  paintBackMotors(BLACK);
  paintExtraPixels(readNazaColorIndex());
  showPixels();
  delayWithNazaLight(5);
  // Vorne schwarz, hinten langsam heller
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(PWHITE);
  paintExtraPixels(readNazaColorIndex());
  showPixels();
  delayWithNazaLight(5);
  // Vorne rechts blau, hinten weiss
  paintLeftMotors(BLACK);
  paintRightMotors(BLUE);
  paintBackMotors(DIMWHITE);
  paintExtraPixels(readNazaColorIndex());
  showPixels();
  delayWithNazaLight(5);
  // Vorne schwarz, hinten weiss
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(DIMWHITE);
  paintExtraPixels(readNazaColorIndex());
  showPixels();
  delayWithNazaLight(10);
  // Nach etwas längerer Pause vorne links rot, hinten weiss
  paintLeftMotors(RED);
  paintRightMotors(BLACK);
  paintBackMotors(DIMWHITE);
  paintExtraPixels(readNazaColorIndex());
  showPixels();
  delayWithNazaLight(5);
  // Vorne schwarz, hinten langsam dunkler
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(PWHITE);
  paintExtraPixels(readNazaColorIndex());
  showPixels();
  delayWithNazaLight(5);
  // Vorne links rot, hinten dunkler
  paintLeftMotors(RED);
  paintRightMotors(BLACK);
  paintBackMotors(PWHITE);
  paintExtraPixels(readNazaColorIndex());
  showPixels();
  delayWithNazaLight(5);
  // Alles schwarz
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(BLACK);
  paintExtraPixels(readNazaColorIndex());
  showPixels();
  delayWithNazaLight(10);
}

/*
 * Regenbogenlicht. Bei 256 LEDs würde der ganze Regenbogen aufs Mal angezeigt. Bei weniger LEDs wird nur ein Ausschnitt
 * des Regenbogens angezeigt. Der gezeigte Ausschnitt verschiebt sich durch das Regenbogenspektrum.
*/
void paintRainbow() {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {  // Durch das ganze Farbrad laufen
    for (i = 0; i < MOTORS * LEDPERMOTOR; i++) {  // Bis zu 256 LEDs hat jede eine eigene Farbe, darüber wiederholt es sich
      pixels.setPixelColor(i, Wheel((i + j) & 255));
    }
    // Extra LEDs mit Naza Farben malen
    paintExtraPixels(readNazaColorIndex());
    showPixels();
    // Zwischendurch prüfen, ob der Lichtmodus geändert hat.
    if (checkChangedLightMode()) {
      return;
    }
    delayWithNazaLight(5);
  }
}

/*
 * Regenbogen, etwas anders. Der Regenbogen wird gestaucht, so dass immer 5 Mal der ganze Regenbogen mit
 * den verfügbaren LEDs angezeigt wird. Die 5 Regenbögen "drehen" sich dabei um den Kopter rum.
*/
void paintRainbowCycle() {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) {
    for (i = 0; i < MOTORS * LEDPERMOTOR; i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    paintExtraPixels(readNazaColorIndex());
    showPixels();
    if (checkChangedLightMode()) {
      return;
    }
    delayWithNazaLight(2);
  }
}

// Rundumlaufendes Licht mit Farbwechsel
void paintTheaterChaseRainbow() {
  for (int j = 0; j < 256; j++) {   // Durch das ganze Farbrad durchlaufen
    for (int q = 0; q < 7; q++) {
      for (int i = 0; i < MOTORS * LEDPERMOTOR; i = i + 7) {
        pixels.setPixelColor(i + q - 5, Wheel( (i + j) % 255)); // Drei von fünf LEDs werden angeschaltet
        pixels.setPixelColor(i + q - 4, Wheel( (i + j) % 255));
        pixels.setPixelColor(i + q - 3, Wheel( (i + j) % 255));
        pixels.setPixelColor(i + q - 2, Wheel( (i + j) % 255));
        pixels.setPixelColor(i + q - 1, Wheel( (i + j) % 255));
        pixels.setPixelColor(i + q, BLACK);
        pixels.setPixelColor(i + q + 1, BLACK);
      } // Extra Lichter mit Naza Farbe malen
      paintExtraPixels(readNazaColorIndex());
      // Ev. genderte Helligkeit nachfuehren
      pixels.setBrightness(readBrightness());
      showPixels();
      // Zwischendurch immer wieder prüfen, ob inzwischen der Lichtmodus geändert hat.
      if (checkChangedLightMode()) {
        return;
      }
      // Kurze Pause
      delayWithNazaLight(5);
    }
  }
}

/*
 * Man darf nicht zu lange Pausen machen, sonst ist das Extra-Licht unten am Kopter nicht mehr synchron zur Naza LED
 * Hilfsroutine, um die Pausen zu zerstückeln und zwischendurch, falls nötig, das Naza-Licht neu zu zeichnen.
 * Die Pausen sind max. 10ms, was ideal ist. Weniger macht die Pausen insgesamt zu lang, wodurch das Polizei-Licht
 * nicht mehr schön aussieht, mehr führt zu deutlich sichtbaren Verzögerungen am unteren Licht.
*/
void delayWithNazaLight(unsigned int numTenMillis) {
  static byte lastColor = 0;
  for (int i = 0; i < numTenMillis; i++) {
    delay(10);
    byte color = readNazaColorIndex();
    if (lastColor != color) {
      paintExtraPixels(color);
      lastColor = color;
      // Ev. geaenderte Helligkeit nachfuehren
      pixels.setBrightness(readBrightness());
      showPixels();
    }
  }
}

// Hilfsroutine, um eine Farbe aus einem "Farbrad" zu bekommen
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    // Erstes Drittel: Rot hoch, Grün runter, Blau off
    return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    // Zweites Drittel: Rot wieder runter, Grün bleibt off, Blau hoch
    WheelPos -= 85;
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    // Letztes Drittel: Rot bleibt off, Grün wieder hoch, Blau wieder runter
    WheelPos -= 170;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


/*
* Malen der Zusatzpixel (z.B. Ring unten am Kopter)
*/
void paintExtraPixels(unsigned long int colorIndex) {
  unsigned int startIndex = MOTORS * LEDPERMOTOR;
  for (uint16_t i = startIndex; i < startIndex + EXTRALEDS; i++) {
    pixels.setPixelColor(i, nazaLedColors[colorIndex]);
  }
}


// Hilfsfunktion zum Auslesen der LED

/*
 * Lesen der Farbe des LEDs
 * 0 = Schwarz
 * 1 = Gruen
 * 2 = Rot
 * 3 = Gelb
*/
byte readNazaColorIndex() {
  // Lese die rohen Sensorwerte (analog)
  int redInputRaw = analogRead(NAZARED);
  int greenInputRaw = analogRead(NAZAGREEN);
  // Umrechnen in den Index 0-3
  byte colorIndex = ((redInputRaw > 500) << 1) | (greenInputRaw > 500);
  return colorIndex;
}

// Hilfsfunktionen zum Auslesen der Fernsteuerungs-Signale

/*
 * Auslesen des Kanals und normalisieren der Werte auf 0-15 (für max. 16 Lichtmodi)
 * Wer will kann höher gehen, aber es wird dann zunehmend schwieriger, einen bestimmten
 * Lichtmodus mit dem Poti zu treffen.
*/
byte readLightMode() {
  // Umrechnungsfaktor
  static const float normalize = (MAXPULSE - MINPULSE) / 16.0;
  // Puls wurde von der Interrupt Routine geschrieben
  cli();
  unsigned int pulseLength = rotaryPulse;
  sei();
  byte lightMode;
  // Normalisieren Schritt 1: Start bei 0
  if (pulseLength >= MINPULSE) { // Zur Sicherheit: Falls MINPULSE zu hoch gewählt wurde.
    pulseLength -= MINPULSE;
  } else {
    pulseLength = 0;
  }
  // Normalisieren Schritt 2: Skalieren
  lightMode = round(pulseLength / normalize); // Da 0..16 passt der Wert in ein byte
  if (lightMode > 15) lightMode = 15; // Zur Sicherheit: Falls MAXPULSE zu tief gewählt wurde.

  return lightMode;
}

/*
 * Auslesen des 3-Weg Schalters (Helligkeit), daraus die Helligkeit ausrechnen
*/
byte readBrightness() {
  cli();
  unsigned int pulseLength = switchPulse;
  sei();
  // Schalter unten
  if (pulseLength < LOWERSWITCH) {
    return MAXBRIGHTNESS >> 3; // 1/8 max. Helligkeit;
  } else if (pulseLength > HIGHERSWITCH) {
    // Schalter oben
    return MAXBRIGHTNESS;
  } else {
    // Schalter in der Mitte
    return MAXBRIGHTNESS >> 2;
  }
}


/*
 * Die Interrupt Service Routine ist spezifisch fuer den ATtiny85. Fuer einen anderen
 * Mikroprozessor muss dies geaendert werden.
 * PB0 oder PB1 haben einen Interrupt ausgeloest
 *  Dies passiert 2 Mal pro 20ms. Deshalb muss die Routine sehr schnell ablaufen und
 *  darf nicht blockieren.
*/
ISR(PCINT0_vect) {
  static unsigned long rotaryStart = 0; // Zaehler fuer Poti-Impulslaenge
  static unsigned long switchStart = 0; // Zaehler fuer Schalter-Impulslaenge
  static boolean lastPB0 = false; // Letzter Status von PB0 (Pin 5)
  static boolean lastPB1 = false; // Letzter Status von PB1 (Pin 6)

  // Ein Interrupt der waehrend pixels.show() aufgetreten ist waere zu spaet und muss deshalb ignoriert werden.
  if (ignoreInterrupts) {
    return;  // Der Interrupt wird so geloescht
  }

  boolean currentPB0 = ((PINB & 0b00000001) != 0); // Aktueller Status von PB0
  boolean currentPB1 = ((PINB & 0b00000010) != 0); // Aktueller Status von PB1

  // zu Beginn die Zeit nehmen
  unsigned long currentTime = micros();

  // PB0
  if (currentPB0 != lastPB0) {
    // Pin PB0 hat gendert
    lastPB0 = currentPB0;
    if (currentPB0) {
      // Port PB0 0 -> 1, Messung starten
      rotaryStart = currentTime;
    } else {
      // Port PB0 1 -> 0
      if (currentTime < rotaryStart) {
        // micros() ist ueberlaufen, daher nichts veraendern
        return;
      }
      unsigned int pulseLength = (unsigned int)(currentTime - rotaryStart);
      if (pulseLength > (MAXPULSE + 500)) {
        // Wir haben Interrupts verpasst
        // Nichts machen und bei der naechsten Runde neu anfangen
        return;
      }
      // Hier sollte der Puls stimmen, daher setzen
      rotaryPulse = pulseLength;
    }
  }
  // PB1
  if (currentPB1 != lastPB1) {
    // Pin PB1 hat gendert
    lastPB1 = currentPB1;
    if (currentPB1) {
      // Port PB1 0 -> 1, Messung starten
      switchStart = currentTime;
    } else {
      // Port PB1 1 -> 0
      if (currentTime < switchStart) {
        // micros() ist ueberlaufen, daher nichts veraendern
        return;
      }
      unsigned int pulseLength = (unsigned int)(currentTime - switchStart);
      if (pulseLength > (MAXPULSE + 500)) {
        // Wir haben Interrupts verpasst
        // Nichts machen und bei der naechsten Runde neu anfangen
        return;
      }
      // Hier sollte der Puls stimmen, daher setzen
      switchPulse = pulseLength;
    }
  }
}

/*
 * Hilfsroutine um die Pixels anzuzeigen, und waehrenddessen die Interrupts zu ingorieren
 *
 * Dies muss sein, da die NeoPixel Library "cli" und "sei" rund um den "show()" call hat.
 * Dies fuehrt dazu, dass die Interrupts fuer eine Weile ausgesetzt werden. Am Ende, nachdem
 * "sei" aufgerufen wurde von der Library wird ein ev. faelliger Interrupt gleich ausgefuehrt.
 * Dies ist aber nicht gut: Da die Laenge eines Pulses gemessen werden muss darf der Interrupt
 * nicht verzoegert aufgerufen werden: Der Puls wuerde so zu kurz oder zu lang gemmessen.
 * Deshalb soll ein Interrupt, der waehrend show() aufgetreten ist, ignoriert werden.
*/
void showPixels() {
  ignoreInterrupts = true;
  pixels.show();  // suspends and possibly caches interrupts
  ignoreInterrupts = false;
}

/*
 * Hilfsroutine um zu prüfen, ob seit dem letzten Aufruf der Lichtmodus veraendert worden ist.
 * Dazu wird eine allenfalls geaenderte Helligkeit direkt angepasst.
 * Die Routine wird von den "langen" Sequenzen (z.B: Regenbogen) zwischendurch aufgerufen um
 * zu sehen, ob sie Sequenz abgebrochen werden muss.
 *
 * Trotz allem "aufpassen" bei den Interrupts gibt es immer wieder Faelle, bei dem ein Mal
 * ein falscher Lichtmodus gelesen wird. Dies stoert, weil dann z.B. der Regenbogen neu angefangen
 * wird. Dies sieht nicht schoen aus. Weshalb dieser eine falsche Wert kommt konnte ich bisher
 * nicht herausfinden. Deshalb hat diese Pruefroutine eine Umgehungsloesung implementiert:
 * Erst wenn zwei Mal hintereinander derselbe Wert gelesen wird, wird der Wert ueberhaupt
 * ausgewertet.
 *
*/
boolean checkChangedLightMode() {
  static byte lastReading = 99;
  // Allenfalls geaenderte Helligkeit nachfuehren
  byte brightness = readBrightness();
  if (brightness != lastBrightness) {
    lastBrightness = brightness;
    pixels.setBrightness(brightness);
  }
  byte lightMode = readLightMode();
  if (lastReading == lightMode) {
    // Zweimal hintereinander dasselbe gelesen, deshalb den Wert verwenden
    if (lightMode == lastLightMode) {
      // Keine Aenderung
      return false;
    } else {
      // Lichtmodus hat geaendert
      lastLightMode = lightMode;
      return true;
    }
  } else {
    // Einmal geaendert gegenueber letzem Mal, vorerst ignorieren
    lastReading = lightMode;
    return false;
  }
}




