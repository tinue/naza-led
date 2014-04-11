/*
 * Steuerprogramm für Adafruit Neopixels am Hexakopter.
 * Das Programm unterstützt drei Leucht-Modi:
 * - Naza LED duplizieren: Dabei blinken alle Motoren gleich wie die Naza LED
 * - Fluglage: Vorne links rot, vorne rechts grün und hinten weiss (TODO: Weiss blitzen, wie ein Flugzeug)
 * - Landelicht: Alle Arme leuchten weiss
 *
 * - Die Zusatz-LED (bei mir unten am Kopter angebracht) leuchtet immer wie die Naza LED.
 *
 * Gesteuert wird der Leuchtmodus via Potentiometer an einem freien Kanal der Fernsteuerung. Dank Potentiometer gibt es ein paar Zusatzfunktionen:
 * - Ganz links: Lichter aus (ausser Zusatz-LED)
 * - Pro Lichtmodus gibt es drei Helligkeitsstufen: 1/8, 1/4 und volle Helligkeit, je nach Stellung des Poti innerhalb des Bereiches für einen Lichtmodus.
 *
*/

// Diese Konstanten müssen auf die eigene Konfiguration hin angepasst werden
#include <Adafruit_NeoPixel.h>
#define NAZARED A0 // Rotes LED Signal der Naza
#define NAZAGREEN A1 // Gruenes LED Signal der Naza
#define REMOTEROTARY A2 // Arduino-Eingang für den Kanal der Fernsteuerung (Potentiometer)
#define REMOTESWITCH A3 // Arduino-Eingang für den 2. Kanal der Fernsteuerung (Schalter)-
#define LEDOUT 13 // Datenleitung der LED Ringe
#define MOTORS 6 // Anzahl Motoren (Annahme: Pro Motor jeweils gleichviel LEDs)
#define LEDPERMOTOR 12 // Anzahl LEDs pro Motor
#define EXTRALEDS 16 // Anzahl zusätzlicher LEDs am Ende (nach den LEDs der Motoren)
#define MAXBRIGHTNESS 255 // Anpassen an Leistung der Stromversorgung! 255 = Anzahl LEDs mal 0.06A. Hier:  max. 5.28A. Gilt aber nur, wenn alle LEDs weiss leuchten.
#define MINPULSE 984 // Minimale Pule-Länge am Empfänger in ns, muss ev. angepasst werden,
#define MAXPULSE 2003 // Maximale Puls-Länge am Empfänger in ns. Beides gemessen an Frsky Taranis mit X8R Empfänger
#define LOWERSWITCH 1200
#define HIGHERSWITCH 1700 // Grenzen für Bestimmung der Schalterstellung

// Ein paar Konstanten zur besseren Lesbarkeit
#define LIGHTOFF 0 // Licht aus
#define LIGHTNAZA 1 // Naza LED Modus
#define LIGHTFLIGHT 2 // Flugmodus
#define LIGHTFLIGHTNAZA 3
#define LIGHTPOLICE 4 // Polizeilicht
#define LIGHTRAINBOWA 5 // Regenbogen
#define LIGHTRAINBOWB 6 // Regenbogen Variante 2
#define LIGHTTHEATERCHASERAINBOW 7
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
static int maxFreeMem = 0; // Debug: Maximales freies RAM
static int minFreeMem = 32767; // Debug: Minimal freies RAM

void setup() {
  freeRam();
  pixels.begin();
  pixels.setBrightness(MAXBRIGHTNESS);
  pixels.show(); // LEDs schwarz zu Beginn
  Serial.begin(115200);  // Ent-kommentieren für Debug Zwecke.
  pinMode(NAZAGREEN, INPUT);
  pinMode(NAZARED, INPUT);
  pinMode(REMOTEROTARY, INPUT);
  pinMode(REMOTESWITCH, INPUT);
  freeRam();
}

void loop() {
  static byte lastLightMode = 99; // Letzte Werte merken. Meist muss nichts geändert werden, wenn sich keine der Werte ändert
  static byte lastBrightness = 99;
  static byte lastColorIndex = 99; // Wenn sich die Naza LED Farbe ändert muss zumindest die untere LED neu gemalt werden

  // Lesen des Potis (0-15)
  byte lightMode = readRemoteRotary();

  // Helligkeit bestimmen
  byte switchState = readRemoteSwitch();
  byte brightness;
  if (switchState == 0) {
    brightness = MAXBRIGHTNESS >> 3; // 1/8 max. Helligkeit
  } else if (switchState == 1) {
    brightness = MAXBRIGHTNESS >> 2; // 1/4 der maximalen Helligkeit
  } else {
    brightness = MAXBRIGHTNESS; // volle Helligkeit
  }
  pixels.setBrightness(brightness);

  // Aktion nun je nach Lichtmodus
  // "Statische" Modi können in einem Aufwasch gemalt werden, "dynamische" dauern länger
  // Sie werden daher unterschiedlich behandelt
  unsigned long int colorIndex = readNazaColorIndex();
  if (lightMode == LIGHTPOLICE) {
    // Der Polizeilicht-Modus ist "aktiv" und muss daher immer gemalt werden.
    // In dem Modus wird auch das malen der unteren LED direkt übernommen, dader Naza-Statusu mehrmals ausgelesen werden muss.
    paintPoliceLights();
  } else if (lightMode == LIGHTRAINBOWA) {
    paintRainbow();
  } else if (lightMode == LIGHTRAINBOWB) {
    paintRainbowCycle();
  } else if (lightMode == LIGHTTHEATERCHASERAINBOW) {
    paintTheaterChaseRainbow();
  } else if (lastColorIndex != colorIndex || lastLightMode != lightMode || lastBrightness != brightness) {
    // Statische Modi
    // Malen der Pixels nur, wenn sich gegenüber der letzten Runde etwas geändert hat.
    if (lightMode == LIGHTOFF) {
      // Lichter aus
      paintAllMotors(BLACK);
    } else if (lightMode == LIGHTNAZA) {
      // Naza LED Modus
      paintAllMotors(nazaLedColors[colorIndex]);
    } else if (lightMode == LIGHTFLIGHT) {
      // Flugmodus
      paintFlightLights();
    } else if (lightMode == LIGHTFLIGHTNAZA) {
      paintLeftMotors(RED);
      paintRightMotors(GREEN);
      paintBackMotors(nazaLedColors[colorIndex]);
    } else if (lightMode == LIGHTPOLICE) {
      paintPoliceLights();
    } else if (lightMode == LIGHTLAND) {
      // Landelicht
      paintAllMotors(WHITE);
    } else {
      // Unbekannter Lichtmodus -> Schwarz.
      paintAllMotors(BLACK);
    }
    // Unterer Ring malen, alle LEDs anzeigen
    paintExtraPixels(colorIndex);
    pixels.show();
  }
  // Letzte Modi merken für die nächste Runde
  lastLightMode = lightMode;
  lastColorIndex = colorIndex;
  lastBrightness = brightness;

  //Debug: Freies RAM ausgeben
  //int freeMem = freeRam();
  //Serial.print("Min free RAM: ");
  //Serial.print(minFreeMem);
  //Serial.print(", max. free RAM: ");
  //Serial.print(maxFreeMem);
  //Serial.print(", current free RAM: ");
  //Serial.println(freeMem);
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
  pixels.show();
  delayWithNazaLight(5);
  // Vorne schwarz, hinten langsam heller
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(PWHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delayWithNazaLight(5);
  // Vorne rechts blau, hinten weiss
  paintLeftMotors(BLACK);
  paintRightMotors(BLUE);
  paintBackMotors(DIMWHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delayWithNazaLight(5);
  // Vorne schwarz, hinten weiss
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(DIMWHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delayWithNazaLight(10);
  // Nach etwas längerer Pause vorne links rot, hinten weiss
  paintLeftMotors(RED);
  paintRightMotors(BLACK);
  paintBackMotors(DIMWHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delayWithNazaLight(5);
  // Vorne schwarz, hinten langsam dunkler
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(PWHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delayWithNazaLight(5);
  // Vorne links rot, hinten dunkler
  paintLeftMotors(RED);
  paintRightMotors(BLACK);
  paintBackMotors(PWHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delayWithNazaLight(5);
  // Alles schwarz
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(BLACK);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
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
    pixels.show();
    // Zwischendurch prüfen, ob der Lichtmodus geändert hat.
    if (checkChangedLightMode()) {
      return;
    }
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
    pixels.show();
    if (checkChangedLightMode()) {
      return;
    }
  }
}

// Rundumlaufendes Licht mit Farbwechsel
void paintTheaterChaseRainbow() {
  for (int j = 0; j < 256; j++) {   // Durch das ganze Farbrad durchlaufen
    for (int q = 0; q < 5; q++) {
      for (int i = 0; i < MOTORS * LEDPERMOTOR; i = i + 5) {
        pixels.setPixelColor(i + q, Wheel( (i + j) % 255)); // Drei von fünf LEDs werden angeschaltet
        pixels.setPixelColor(i + q + 1, Wheel( (i + j) % 255));
        pixels.setPixelColor(i + q + 2, Wheel( (i + j) % 255));
      } // Extra Lichter mit Naza Farbe malen
      paintExtraPixels(readNazaColorIndex());
      pixels.show();
      // Zwischendurch immer wieder prüfen, ob inzwischen der Lichtmodus geändert hat.
      if (checkChangedLightMode()) {
        return;
      }
      // Kurze Pause
      delayWithNazaLight(10);
      // Die LEDs wieder ablöschen (Vorbereiten für den nächsten Durchlauf)
      for (int i = 0; i < MOTORS * LEDPERMOTOR; i = i + 5) {
        pixels.setPixelColor(i + q, 0);
        pixels.setPixelColor(i + q + 1, 0);
        pixels.setPixelColor(i + q + 1, 0);
      }
    }
  }
}

/*
 * Man darf nicht zu lange Pausen machen, sonst ist das Extra-Licht unten am Kopter nicht mehr synchron zur Naza LED
 * Hilfsroutine, um die Pausen zu zerstückeln und zwischendurch, falls nötig, das Naza-Licht neu zu zeichnen.
 * Die Pausen sind max. 5ms, was ideal ist. Weniger macht die Pausen insgesamt zu lang, wodurch das Polizei-Licht
 * nicht mehr schön aussieht, mehr führt zu deutlich sichtbaren Verzögerungen am unteren Licht.
*/
void delayWithNazaLight(unsigned long numTenMillis) {
  static unsigned long int lastColor = 0;
  for (int i = 0; i < numTenMillis; i++) {
    delay(10);
    unsigned long int color = readNazaColorIndex();
    if (lastColor != color) {
      paintExtraPixels(color);
      pixels.show();
      lastColor = color;
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
  byte colorIndex = (redInputRaw > 500) | ((greenInputRaw > 500) << 1);
  return colorIndex;
}

// Hilfsfunktionen zum Auslesen der Fernsteuerungs-Signale

/*
 * Auslesen des Kanals und normalisieren der Werte auf 0-15 (für max. 16 Lichtmodi)
 * Wer will kann höher gehen, aber es wird dann zunehmend schwieriger, einen bestimmten
 * Lichtmodus mit dem Poti anzusteuern.
*/
byte readRemoteRotary() {
  // Umrechnungsfaktor
  static const float normalize = (MAXPULSE - MINPULSE) / 16.0;
  unsigned long int pulseLength = pulseIn(REMOTEROTARY, HIGH, 21000); // Alle rund 20ms kommt ein Puls. Laenger = Kein Signal
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
 * Auslesen des 3-Weg Schalters (Helligkeit), normalisieren auf Werte 0-2
*/
byte readRemoteSwitch() {
  // Umrechnungsfaktor
  unsigned long int pulseLength = pulseIn(REMOTESWITCH, HIGH, 21000); // Alle rund 20ms kommt ein Puls. Laenger = Kein Signal
  // Normalisieren Schritt 1: Start bei 0
  if (pulseLength < LOWERSWITCH) {
    return 0;
  } else if (pulseLength > HIGHERSWITCH) {
    return 2;
  } else {
    return 1;
  }
}

/*
 * Hilfsroutine um zu prüfen, ob seit dem letzten Aufruf der Lichtmodus verändert worden ist.
 * An sich sollte man auch prüfen, ob die Helligkeit geändert hat. Dies dauert aber nochmals bis zu
 * 20 Millisekunden, und stört das Regenbogenlicht noch mehr. Deshalb wird dies nicht gemacht.
 * Dies hat zur Folge, dass ein Umschalten der Helligkeit erst nach ein paar Sekunden bemerkt wird.
*/
boolean checkChangedLightMode() {
  static byte lastLightMode = 99;
  byte lightMode = readRemoteRotary();
  if (lightMode == lastLightMode) {
    return false;
  } else {
    lastLightMode = lightMode;
    return true;
  }
}

/*
 * Debug: Freies RAM
*/
int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  int retVal = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  if (retVal > maxFreeMem) {
    maxFreeMem = retVal;
  }
  if (retVal < minFreeMem) {
    minFreeMem = retVal;
  }
  return retVal;
}




