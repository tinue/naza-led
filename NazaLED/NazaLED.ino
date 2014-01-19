/*
 * Steuerprogramm für Adafruit Neopixels am Hexakopter.
 * Das Programm unterstützt drei Leucht-Modi:
 * - Naza LED duplizieren: Dabei blinken alle Ringe gleich wie die Naza LED
 * - Fluglage: Die roten Arme leuchten rot, die anderen Arme leuchten grün
 * - Landelicht: Alle Ringe leuchten weiss
 *
 * Gesteuert wird der Leuchtmodus via Dreiwegschalter an Kanal 8
 *
*/

#include <Adafruit_NeoPixel.h>
#define NAZARED A3 // Rotes LED Signal der Naza
#define NAZAGREEN A2 // Gruenes LED Signal der Naza
#define REMOTEROTARY 6 // Kanal der Fernsteuerung (3 Weg Schalter)
#define RINGOUT 9 // Datenleitung der LED Ringe
#define MOTORS 6 // Anzahl Motoren (und somit Anzahl Ringe)
#define MAXBRIGHTNESS 22 // Anpassen an Leistung der Stromversorgung! 255 = max. 5.76A; 22 = max. 0.5A (USB)
#define MINPULSE 1089 // Minimale Pule-Länge an Graupner Empfänger
#define MAXPULSE 1884 // Maximale Puls-Länge an Graupner Empfänger

// "ring" sind die LED Ringe. Der erste Paremeter gibt die Anzahl LEDs an, 16 pro Ring
Adafruit_NeoPixel ring = Adafruit_NeoPixel(MOTORS * 16, RINGOUT, NEO_GRB + NEO_KHZ800);

void setup() {
  ring.begin();
  ring.setBrightness(MAXBRIGHTNESS);
  ring.show(); // LEDs schwarz zu Beginn
  //Serial.begin(115200);
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
  if (rotaryState < 80) {
    // Naza LED Modus: LEDs zeigen dasselbe Signal wie die Naza LED
    unsigned long int colorIndex = readNazaColorIndex();
    if (lastColorIndex != colorIndex || lastRotaryState != rotaryState) {
      // Neuer Wert am Poti, und/oder neue Farbe an der Naza LED: LEDs neu malen
      ring.setBrightness(brightness);
      paintAllRings(nazaLedColors[readNazaColorIndex()]);
      ring.show();
      lastColorIndex = colorIndex;
      lastRotaryState = rotaryState;
    }
  } else if (rotaryState < 160) {
    // Flugmodus: LEDs leuchten vorne rot und hinten grün (wie beim Phantom)
    if (lastRotaryState != rotaryState) {
      // Neuer Wert am Poti: Neu malen
      ring.setBrightness(brightness);
      paintFlightLights();
      ring.show();
      lastRotaryState = rotaryState;
    }
  } else if (rotaryState < 240) {
    // Landemodus: Alles weiss
    if (lastRotaryState != rotaryState) {
      // Neuer Wert am Poti: Neu malen
      ring.setBrightness(brightness);
      paintAllRings(0xFFFFFF);
      ring.show();
      lastRotaryState = rotaryState;
    }
  } else {
    // Notprogramm, LEDs blitzen lassen in verschiedenen Farben
    // TODO: Implementieren
    if (lastRotaryState != rotaryState) {
      ring.setBrightness(brightness);
      paintAllRings(0x000000);
      ring.show();
      lastRotaryState = rotaryState;
    }
  }
}


// Hilfsfunktionen zum Malen der Ringe

/*
* Malen aller LEDs eines Motors mit derselben Farbe
*/
void paintRing(byte motor, unsigned long int color) {
  unsigned int startIndex = (motor - 1) << 4; // 16 LEDs pro Ring
    for(uint16_t i=startIndex; i<startIndex + 16; i++) {
      ring.setPixelColor(i, color);
  }
}

/*
 * Male sämtliche LEDs mit derselben Farbe
*/
void paintAllRings(unsigned long int color) {
  for (int motor=1; motor<=6; motor++) {
    paintRing(motor, color);
  }
}

/*
 * Male Flugmuster (vorne rot, hinten grün)
*/
void paintFlightLights() {
  paintRing(1, 0xFF0000); // Vorne links
  paintRing(2, 0xFF0000); // Vorne rechts
  paintRing(3, 0x00FF00); // Mitte links
  paintRing(6, 0x00FF00); // Mitte rechts
  paintRing(4, 0x00FF00); // Hinten links
  paintRing(5, 0x00FF00); // Hinten rechts
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



