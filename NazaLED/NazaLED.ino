/*
 * Steuerprogramm für Adafruit Neopixels am Hexakopter.
 * Das Programm unterstützt drei Leucht-Modi:
 * - Naza LED duplizieren: Dabei blinken alle Motoren gleich wie die Naza LED
 * - Fluglage: Die vorderen zwei Arme leuchten rot, die anderen Arme leuchten wie die Naza LED (wie Phantom 2)
 * - Landelicht: Alle Ringe leuchten weiss
 *
 * - Die zusazz-LED (bei mir unten am Kopter angebracht) leuchtet immer wie die Naza LED
 *
 * Gesteuert wird der Leuchtmodus via Dreiwegschalter an Kanal 8
 *
*/

#include <Adafruit_NeoPixel.h>
#define NAZARED A3 // Rotes LED Signal der Naza
#define NAZAGREEN A2 // Gruenes LED Signal der Naza
#define REMOTEROTARY 6 // Kanal der Fernsteuerung (3 Weg Schalter)
#define LEDOUT 9 // Datenleitung der LED Ringe
#define MOTORS 6 // Anzahl Motoren (Annahme: Pro Motor jeweils gleichviel Pixels)
#define LEDPERMOTOR 12 // Anzahl LEDs pro Motor
#define EXTRALEDS 16 // Anzahl zusätzlicher LEDs am Ende (nach den LEDs der Motoren)
#define MAXBRIGHTNESS 50 // Anpassen an Leistung der Stromversorgung! 255 = max. 5.76A; 22 = max. 0.5A (USB)
#define MINPULSE 1089 // Minimale Pule-Länge an Graupner Empfänger
#define MAXPULSE 1884 // Maximale Puls-Länge an Graupner Empfänger

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
  static byte lastRotaryState = 256; // Letzter Zustand des Potis merken: Wenn sich nichts verändert muss meist nichts gemacht werden.
  static byte lastColorIndex = 99; // Im Naza LED Modus muss doch etwas gemacht werden, wenn sich die Naza LED ändert.
  static const float brightFactor =  MAXBRIGHTNESS / 80.0; // Umrechnungsfaktor für die Helligkeit.
  static const unsigned long int nazaLedColors[] = { 0x000000, 0x00FF00, 0xFF0000, 0xFF4400 }; // schwarz, gruen, rot und gelb
  // Lesen des Potis (0-255)
  byte rotaryState = readRemoteRotary();
  byte brightness = round((rotaryState % 80) * brightFactor);
  // Aktion nun je nach Stellung des Schalters
  unsigned long int colorIndex = readNazaColorIndex();
  if (rotaryState < 80) {
    // Naza LED Modus: LEDs zeigen dasselbe Signal wie die Naza LED
    //unsigned long int colorIndex = readNazaColorIndex();
    if (lastColorIndex != colorIndex || lastRotaryState != rotaryState) {
      // Neuer Wert am Poti, und/oder neue Farbe an der Naza LED: LEDs neu malen
      pixels.setBrightness(brightness);
      paintAllMotors(nazaLedColors[readNazaColorIndex()]);
      paintExtraPixels(nazaLedColors[readNazaColorIndex()]);
      pixels.show();
      lastColorIndex = colorIndex;
      lastRotaryState = rotaryState;
    }
  } else if (rotaryState < 160) {
    // Flugmodus: LEDs leuchten vorne rot und hinten wie die Naza LED (ähnlich Phantom 2)
    if (lastColorIndex != colorIndex || lastRotaryState != rotaryState) {
      // Neuer Wert am Poti: Neu malen
      pixels.setBrightness(brightness);
      paintFlightLights(nazaLedColors[readNazaColorIndex()]);
      pixels.show();
      lastRotaryState = rotaryState;
    }
  } else if (rotaryState < 240) {
    // Landemodus: Alles weiss
    if (lastRotaryState != rotaryState) {
      // Neuer Wert am Poti: Neu malen
      pixels.setBrightness(brightness);
      paintAllMotors(0xFFFFFF);
      pixels.show();
      lastRotaryState = rotaryState;
    }
  } else {
    // Notprogramm, LEDs blitzen lassen in verschiedenen Farben
    // TODO: Implementieren
    if (lastRotaryState != rotaryState) {
      pixels.setBrightness(brightness);
      paintAllMotors(0x000000);
      pixels.show();
      lastRotaryState = rotaryState;
    }
  }
  if (lastColorIndex != colorIndex) {
    paintExtraPixels(nazaLedColors[readNazaColorIndex()]);
    pixels.show();
    lastColorIndex = colorIndex;
  }
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
// TODO: Umarbeiten, so dass es für 4 oder 8 Motoren ebenfalls funktioniert.
void paintFlightLights(unsigned long int color) {
  // Motoren vorne rot malen
  paintMotor(1, 0xFF0000); // Vorne links
  paintMotor(2, 0xFF0000); // Vorne rechts
  // Alle anderen Motoren wie Naza LED malen
  for (int motor = 3; motor <= MOTORS; motor++) {
    paintMotor(motor, color); // Mitte links
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




