/*
 * Simulator fuer NAZA LED Leuchtsequenzen.
 * Dient der Entwicklung des "NazaLED" Programmes, damit nicht jedes Mal
 * der Kopter aufgeschraubt werden muss.
 *
*/

// Anpassen, falls andere Ports gewuenscht sind
#define REDPIN 2
#define GREENPIN 1
#define BLUEPIN 0
#define NAZALEDRED 3
#define NAZALEDGREEN 4

#define RED 'R'
#define GREEN 'G'
#define YELLOW 'Y'

// Nicht viel zu tun hier
void setup() {
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  pinMode(NAZALEDRED, OUTPUT);
  pinMode(NAZALEDGREEN, OUTPUT);  
}

// Kann beliebig angepasst werden. Die schnellste Sequenz ist z.B. "batteryEmpty". Wemm man also
// testen will, ob "NazaLED" mit auslesen und zeichnen nachkommt, dann kann man hier auch
// ausschliesslich "batteryEmpty()" loopen lassen.
void loop() {
  startup();
  for (int i = 0; i < 5; i++) {
    noGPSLink();
  }
  for (int i = 0; i < 2; i++) {
    noGPSLinkStickNotMiddle();
  }
  lockAcquired();
  for (int i = 0; i < 2; i++) {
    noGPSLink();
  }
  for (int i = 0; i < 30; i++) {
    failsafe();
  }
  for (int i = 0; i < 15; i++) {
    batteryEmpty();
  }
}

void batteryEmpty() {
  blink(RED, 84, 17);
  blink(RED, 84, 17);
  blink(RED, 84, 17);
  blink(RED, 84, 617);
}

void startup() {
  blink(RED, 286, 0);
  blink(GREEN, 315, 0);
  blink(YELLOW, 424, 0);
  blink(RED, 277, 0);
  blink(GREEN, 315, 0);
  blink(YELLOW, 483, 500);
  blink(GREEN, 239, 256);
  blink(GREEN, 239, 252);
  blink(GREEN, 244, 256);
  blink(GREEN, 244, 798);
}

void lockAcquired() {
  for (int i = 0; i < 30; i++) {
    blink(GREEN, 42, 59);
  }
  blink(GREEN, 42, 676);
}

void failsafe() {
  blink(YELLOW, 105, 97);
}

void noGPSLink() {
  blink (GREEN, 63, 718);
  blink (RED, 63, 176);
  blink (RED, 63, 185);
  blink (RED, 105, 1037);
}

void noGPSLinkStickNotMiddle() {
  blink (GREEN, 63, 59);
  blink (GREEN, 63, 596);
  blink (RED, 63, 176);
  blink (RED, 63, 185);
  blink (RED, 105, 1037);
}


void blink(char color, unsigned long lightMillis, unsigned long pauseMillis) {
  if (color == RED) {
    red(lightMillis, pauseMillis);
  } else if (color == GREEN) {
    green(lightMillis, pauseMillis);
  } else if (color == YELLOW) {
    yellow(lightMillis, pauseMillis);
  }
}

void red(unsigned long lightMillis, unsigned long pauseMillis) {
  analogWrite(REDPIN, 0);
  analogWrite(BLUEPIN, 255);
  analogWrite(GREENPIN, 255);
  digitalWrite(NAZALEDRED, 1);
  digitalWrite(NAZALEDGREEN, 0);
  delay(lightMillis);
  black();
  delay(pauseMillis);
}

void green(unsigned long lightMillis, unsigned long pauseMillis) {
  analogWrite(REDPIN, 255);
  analogWrite(BLUEPIN, 255);
  analogWrite(GREENPIN, 0);
  digitalWrite(NAZALEDRED, 0);
  digitalWrite(NAZALEDGREEN, 1);
  delay(lightMillis);
  black();
  delay(pauseMillis);
}

void yellow(unsigned long lightMillis, unsigned long pauseMillis) {
  analogWrite(REDPIN, 0);
  analogWrite(BLUEPIN, 255);
  analogWrite(GREENPIN, 50);
  digitalWrite(NAZALEDRED, 1);
  digitalWrite(NAZALEDGREEN, 1);
  delay(lightMillis);
  black();
  delay(pauseMillis);
}

void black() {
  analogWrite(REDPIN, 255);
  analogWrite(GREENPIN, 255);
  analogWrite(BLUEPIN, 255);
  digitalWrite(NAZALEDRED, 0);
  digitalWrite(NAZALEDGREEN, 0);
}



