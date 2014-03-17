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
#define NAZARED A1 // Rotes LED Signal der Naza
#define NAZAGREEN A0 // Gruenes LED Signal der Naza
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
#define LIGHTLAND 3 // Landelicht
#define LIGHTPOLICE 4 // Polizeilicht
#define BLACK 0x000000 // Schwarz
#define DIMWHITE 0x808080 // dunkleres Weiss
#define RED  0xFF0000 // Rot
#define BLUE 0x0000FF // Blau
#define GREEN 0x00FF00 // Grün
#define YELLOW 0xFFA500 // Gelb
#define WHITE 0xFFFFFF // Weiss

// "pixels" sind alle LEDs, zusammengehängt. Der erste Paremeter gibt die Anzahl LEDs an
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(MOTORS * LEDPERMOTOR + EXTRALEDS, LEDOUT, NEO_GRB + NEO_KHZ800);
// Die vier möglichen Farben der Naza LED
static const unsigned long int nazaLedColors[] = { BLACK, GREEN, RED, YELLOW };

void setup() {
  pixels.begin();
  pixels.setBrightness(MAXBRIGHTNESS);
  pixels.show(); // LEDs schwarz zu Beginn
  //Serial.begin(115200);  // Ent-kommentieren für Debug Zwecke.
  pinMode(NAZAGREEN, INPUT);
  pinMode(NAZARED, INPUT);
  pinMode(REMOTEROTARY, INPUT);
  pinMode(REMOTESWITCH, INPUT);
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

  // Aktion nun je nach Lichtmodus
  unsigned long int colorIndex = readNazaColorIndex();
  // Malen der Pixels nur, wenn sich gegenüber der letzten Runde etwas geändert hat. Der Polizeilicht-Modus
  // ist "aktiv" und muss daher immer gemalt werden. In dem Modus wird auch das malen der unteren LED direkt übernommen.
  if (LIGHTPOLICE || lastColorIndex != colorIndex || lastLightMode != lightMode || lastBrightness != brightness) {
    pixels.setBrightness(brightness);
    if (lightMode == LIGHTOFF) {
      // Lichter aus
      paintAllMotors(BLACK);
    } else if (lightMode == LIGHTNAZA) {
      // Naza LED Modus
      paintAllMotors(nazaLedColors[colorIndex]);
    } else if (lightMode == LIGHTFLIGHT) {
      // Flugmodus
      paintFlightLights();
    } else if (lightMode == LIGHTPOLICE) {
      paintPoliceLights();
    } else if (lightMode == LIGHTLAND) {
      // Landelicht
      paintAllMotors(WHITE);
    } else {
      // Unbekannter Lichtmodus -> Schwarz.
      paintAllMotors(BLACK);
    }
  }
  // Unterer Ring malen, alle LEDs anzeigen, letzte Modi merken für die nächste Runde
  //TODO: Für "dynamische" Modi, wie Polizeilicht, müssten die nächsten zwei Zeilen nicht ausgeführt werden.
  paintExtraPixels(colorIndex);
  pixels.show();
  lastLightMode = lightMode;
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
    paintMotor(2, color);
    paintMotor(3, color);
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
  delay(50);
  // Vorne schwarz, hinten langsam heller
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(DIMWHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delay(50);
  // Vorne rechts blau, hinten weiss
  paintLeftMotors(BLACK);
  paintRightMotors(BLUE);
  paintBackMotors(WHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delay(50);
  // Vorne schwarz, hinten weiss
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(WHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delay(100);
  // Nach etwas längerer Pause vorne links rot, hinten weiss
  paintLeftMotors(RED);
  paintRightMotors(BLACK);
  paintBackMotors(WHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delay(50);
  // Vorne schwarz, hinten langsam dunkler
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(DIMWHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delay(50);
  // Vorne links rot, hinten dunkler
  paintLeftMotors(RED);
  paintRightMotors(BLACK);
  paintBackMotors(DIMWHITE);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delay(50);
  // Alles schwarz
  paintLeftMotors(BLACK);
  paintRightMotors(BLACK);
  paintBackMotors(BLACK);
  paintExtraPixels(readNazaColorIndex());
  pixels.show();
  delay(100);
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
 * Auslesen dines Kanals und normalisieren der Werte auf 0-15 (für max. 16 Lichtmodi)
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
 * Auslesen des 3-Weg Switches, normalisieren auf Werte 0-2
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




