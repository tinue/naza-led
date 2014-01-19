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
#define REMOTESWITCH 6 // Kanal der Fernsteuerung (3 Weg Schalter)
#define RINGOUT 9 // Datenleitung der LED Ringe
#define MOTORS 6 // Anzahl Motoren (und somit Anzahl Ringe)

// "ring" sind die LED Ringe. Der erste Paremeter gibt die Anzahl LEDs an, 16 pro Ring
Adafruit_NeoPixel ring = Adafruit_NeoPixel(MOTORS * 16, RINGOUT, NEO_GRB + NEO_KHZ800);

void setup() {
  ring.begin();
  ring.setBrightness(40); // Testbetrieb: Eher dunkel, da der Strom vom Arduino genommen wird
  ring.show(); // LEDs schwarz zu Beginn
  //Serial.begin(115200);
  pinMode(NAZAGREEN, INPUT);
  pinMode(NAZARED, INPUT);
  pinMode(REMOTESWITCH, INPUT);
}

void loop() {
  static const unsigned long int nazaLedColors[] = { 0x000000, 0x00FF00, 0xFF0000, 0xFF4400 }; // schwarz, gruen, rot und gelb
  // Lesen des Dreiwege-Schalters (0-2)
  byte switchState = readRemoteSwitch();
  // Aktion nun je nach Stellung des Schalters
  if (switchState == 0) {
    // LEDs zeigen dasselbe Signal wie die Naza LED
    paintAllRings(nazaLedColors[readNazaColorIndex()]);
    ring.show();
  } else if (switchState == 1) {
    // LEDs leuchten vorne rot und hinten grün (wie beim Phantom)
    paintFlightLights();
    ring.show();
  } else {
    // LEDs leuchten alle weiss
    paintAllRings(0xFFFFFF);
    ring.show();
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

/*
 * Auslesen dines Kanals und umrechnen in einen Index 0-2
 * entsprechend der Stellung eines Dreiweg-Schalters.
*/
byte readRemoteSwitch() {
  unsigned long int pulseLength = pulseIn(REMOTESWITCH, HIGH, 21000); // Alle rund 20ms kommt ein Puls. Laenger = Kein Signal
  if (pulseLength < 1100) {
    return 0;
  } else if (pulseLength < 1700) {
    return 1;
  } else {
    return 2;
  }
}




