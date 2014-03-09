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
#define NAZARED A3 // Rotes LED Signal der Naza
#define NAZAGREEN A2 // Gruenes LED Signal der Naza
#define REMOTEROTARY 6 // Arduino-Eingang für den Kanal der Fernsteuerung (Potentiometer)
#define LEDOUT 9 // Datenleitung der LED Ringe
#define MOTORS 6 // Anzahl Motoren (Annahme: Pro Motor jeweils gleichviel LEDs)
#define LEDPERMOTOR 12 // Anzahl LEDs pro Motor
#define EXTRALEDS 16 // Anzahl zusätzlicher LEDs am Ende (nach den LEDs der Motoren)
#define MAXBRIGHTNESS 255 // Anpassen an Leistung der Stromversorgung! 255 = Anzahl LEDs mal 0.06A. Hier:  max. 5.28A. Gilt aber nur, wenn alle LEDs weiss leuchten.
#define MINPULSE 1089 // Minimale Pule-Länge am Empfänger in ns, muss ev. angepasst werden,
#define MAXPULSE 1884 // Maximale Puls-Länge am Empfänger in ns

// Ein paar Konstanten zur besseren Lesbarkeit
#define LIGHTOFF 0 // Licht aus
#define LIGHTNAZA 1 // Naza LED Modus
#define LIGHTFLIGHT 2 // Flugmodus
#define LIGHTLAND 3 // Landelicht
#define BLACK 0x000000 // Schwarz
#define RED  0xFF0000 // Rot
#define GREEN 0x00FF00 // Grün
#define YELLOW 0xFFA500 // Gelb
#define WHITE 0xFFFFFF // Weiss

// "pixels" sind alle LEDs, zusammengehängt. Der erste Paremeter gibt die Anzahl LEDs an
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(MOTORS * LEDPERMOTOR + EXTRALEDS, LEDOUT, NEO_GRB + NEO_KHZ800);

void setup() {
  pixels.begin();
  pixels.setBrightness(MAXBRIGHTNESS);
  pixels.show(); // LEDs schwarz zu Beginn
  //Serial.begin(115200);  // Ent-kommentieren für Debug Zwecke.
  pinMode(NAZAGREEN, INPUT);
  pinMode(NAZARED, INPUT);
  pinMode(REMOTEROTARY, INPUT);
}

void loop() {
  /**
  * Die Poti-Stellung wird auf 0-255 normalisiert.
  * Regionen Potentiometer:
  *   0- 15: Licht aus
  *  16- 95: Naza LED Modus
  *  96-175: Flugmodus
  * 176-255: Landelicht
  */
  static byte lastLightMode = 99; // Letzter Zustand des Potis merken: Wenn sich nichts verändert muss meist nichts gemacht werden.
  static byte lastColorIndex = 99; // Im Naza LED Modus muss doch etwas gemacht werden, wenn sich die Naza LED ändert.
  static byte lastBrightness = 99;
  static const unsigned long int nazaLedColors[] = { BLACK, GREEN, RED, YELLOW }; // schwarz, gruen, rot und gelb; Anpassen nach Wunsch

  // Lesen des Potis (0-255)
  byte rotaryState = readRemoteRotary();
  byte lightMode;
  // Lichtmodus bestimmen
  if (rotaryState < 16) {
    lightMode = LIGHTOFF;
  } else if (rotaryState < 96) {
    lightMode = LIGHTNAZA;
  } else if (rotaryState < 176) {
    lightMode = LIGHTFLIGHT;
  } else {
    lightMode = LIGHTLAND;
  }

  // Helligkeit bestimmen
  byte brightness;
  if (rotaryState < 16) {
    // Licht ist aus, trotzdem unterer Ring hell brennen lassen
    brightness = MAXBRIGHTNESS;
  } else {
    brightness = (rotaryState - 16) % 80; // 0-15 ist aus, danach 80 Werte pro Lichtmodus
    if (brightness < 26) {
      brightness = MAXBRIGHTNESS >> 3; // 1/8 der maximalen Helligkeit
    } else if (brightness < 54) {
      brightness = MAXBRIGHTNESS >> 2; // 1/4 der maximalen Helligkeit
    } else {
      brightness = MAXBRIGHTNESS; // volle Helligkeit
    }
  }

  // Aktion nun je nach Lichtmodus
  unsigned long int colorIndex = readNazaColorIndex();
  // Malen der Pixels nur, wenn sich gegenüber der letzten Runde etwas geändert hat.
  if (lastColorIndex != colorIndex || lastLightMode != lightMode || lastBrightness != brightness) {
    pixels.setBrightness(brightness);
    if (lightMode == LIGHTOFF) {
      // Lichter aus
      paintAllMotors(nazaLedColors[BLACK]);
    } else if (lightMode == LIGHTNAZA) {
      // Naza LED Modus
      paintAllMotors(nazaLedColors[colorIndex]);
    } else if (lightMode == LIGHTFLIGHT) {
      // Flugmodus
      paintFlightLights();
    } else {
      // Landelicht
      paintAllMotors(WHITE);
    }
  }
  // Unterer Ring malen, alle LEDs anzeigen, letzte Modi merken für die nächste Runde
  paintExtraPixels(nazaLedColors[colorIndex]);
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

/*
 * Male Flugmuster (vorne rot, hinten grün)
*/
void paintFlightLights() {
  if (MOTORS == 6) {
    // Motoren vorne links rot malen
    paintMotor(2, RED);
    paintMotor(3, RED);
    // Motoren vorne rechts grün malen
    paintMotor(1, GREEN);
    paintMotor(6, GREEN);
    // Hintere Motoren weiss malen
    paintMotor(4, WHITE);
    paintMotor(5, WHITE);
  } else if (MOTORS == 4) {
    // Vorne rot, hinten grün
    paintMotor(1, RED);
    paintMotor(2, RED);
    paintMotor(3, GREEN);
    paintMotor(4, GREEN);
  } else {
    // Keine Ahnung was sonst... Male alles rot.
    for (int i = 1; i <= MOTORS; i++) {
      paintMotor(i, RED);
    }
  }
}

/*
* Malen der Zusatzpixel
*/
void paintExtraPixels(unsigned long int color) {
  unsigned int startIndex = MOTORS * LEDPERMOTOR;
  for (uint16_t i = startIndex; i < startIndex + EXTRALEDS; i++) {
    pixels.setPixelColor(i, color);
  }
}


// Hilfsfunktionen zum Auslesen der LED und der Fernsteuerung

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

/**
 * Funktion um das Poti zur Modus-Steuerung zu ent-zittern
 * Der Wert des Potis schwankt leicht, da die Kanäle nicht 100% sauber sind
 * Deshalb wird eine Aenderung von weniger als 3 nicht als relevant angeschaut.
*/
byte getSmoothedRotaryState(byte currentRotaryState) {
  static byte lastRotaryState = 511; // Zu hoher Wert, damit er sicher beim ersten Aufruf ersetzt wird.

  byte balance = abs(lastRotaryState - currentRotaryState);
  if (balance >= 4) { // Beim mx-16 Poti 6 schwankt der Wert erheblich, deshalb erst ab 5 ändern
    lastRotaryState = currentRotaryState;
  }
  return lastRotaryState;
}



/*
 * Auslesen dines Kanals und normalisieren der Werte auf 0-255
 * entsprechend der Stellung eines Potis
*/
byte readRemoteRotary() {
  // Umrechnungsfaktor
  static const float normalize = (MAXPULSE - MINPULSE) / 256.0;
  unsigned long int pulseLength = pulseIn(REMOTEROTARY, HIGH, 21000); // Alle rund 20ms kommt ein Puls. Laenger = Kein Signal
  // Normalisieren Schritt 1: Start bei 0
  if (pulseLength >= MINPULSE) { // Zur Sicherheit: Falls MINPULSE zu hoch gewählt wurde.
    pulseLength -= MINPULSE;
  } else {
    pulseLength = 0;
  }
  // Normalisieren Schritt 2: Skalieren
  pulseLength = round(pulseLength / normalize);
  if (pulseLength > 255) pulseLength = 255; // Zur Sicherheit: Falls MAXPULSE zu tief gewählt wurde.

  return getSmoothedRotaryState(pulseLength); // Da 0..255 passt der Wert in ein byte
}




