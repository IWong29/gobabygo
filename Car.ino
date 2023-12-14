#include <SPI.h>
#include <Ultrasonic.h>
#include <RFM69.h>

#define NETWORKID 5  // Must be the same for all nodes
#define MYNODEID 1   // My node ID
#define TONODEID 2   // Destination node ID

#define FREQUENCY RF69_915MHZ

#define ENCRYPT true                   // Set to "true" to use encryption
#define ENCRYPTKEY "TOPSECRETPASSWRD"  // Use the same 16-byte key on all nodes

#define USEACK true  // Request ACKs or not

#define SCK 13
#define MISO 12
#define MOSI 11
#define CS 10
#define INTERRUPT A0
#define TRIG 2
#define ECHO 3
#define LED1 4
#define LED2 5
#define LED3 6
#define PEDAL 7
#define DRIVE 8
#define LIGHT 9
#define STOP A5

RFM69 radio;
Ultrasonic ultrasonic(TRIG, ECHO);
bool driving = false;
double timeBuffer;

void setup() {
  // Open a serial port so we can send keystrokes to the module:

  Serial.begin(9600);
  Serial.print("Node ");
  Serial.print(MYNODEID, DEC);
  Serial.println(" ready");

  pinMode(INTERRUPT, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(PEDAL, OUTPUT);
  pinMode(DRIVE, INPUT);
  pinMode(STOP, INPUT);
  pinMode(LIGHT, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);

  // Initialize the RFM69HCW:
  // radio.setCS(10);  //uncomment this if using Pro Micro
  radio.initialize(FREQUENCY, MYNODEID, NETWORKID);
  radio.setHighPower();  // Always use this for RFM69HCW

  // Turn on encryption if desired:

  if (ENCRYPT)
    radio.encrypt(ENCRYPTKEY);

  digitalWrite(LIGHT, HIGH);

  PCICR |= (1 << PCIE0);
  PCMSK0 |= (1 << PB0);
  PCMSK1 |= (1 << PC5) & (1 << PC0);
}

void loop() {
  // ULTRASONIC CODE
  if (driving && ultrasonic.read(INC) < 30) { //Polling is minimal in this program because all other inputs are consolidated to hardware interrupts.
    digitalWrite(PEDAL, LOW);
    driving = false;
    Flicker(LIGHT, 3, 400);
    delay(4000);
  }
}

// LED BLINK

void Blink(byte PIN, int DELAY_MS) {
  digitalWrite(PIN, HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN, LOW);
}

// LED FLICKER

void Flicker(byte PIN, int COUNT, int DURATION_MS) {
  for (int i = 0; i < COUNT; i++) {
    digitalWrite(PIN, LOW);
    delay(DURATION_MS);
    digitalWrite(PIN, HIGH);
    delay(DURATION_MS);
  }
}

// SENDING - From RFM69HCW Examples

void sendRFM(String data) {
  char sendbuffer[62];
  int counter = 0;
  while (counter < data.length() && counter < 62) {
    sendbuffer[counter] = data[counter];
    counter++;
  }

  if (USEACK) {
    if (radio.sendWithRetry(TONODEID, sendbuffer, counter))
      Serial.println("ACK received!");
    else
      Serial.println("no ACK received");
  }

  else  // don't use ACK
  {
    radio.send(TONODEID, sendbuffer, counter);
  }
  Blink(LED1, 10);
  return;
}

// RECEIVING - From RFM69HCW Examples

char* scanRFM() {
  if (radio.receiveDone())  // Got one!
  {
    char* data = new char[62];
    for (byte i = 0; i < radio.DATALEN; i++)
      data[i] = (char)radio.DATA[i];

    // RSSI is the "Receive Signal Strength Indicator",
    // smaller numbers mean higher power.

    //Serial.print("], RSSI ");
    //Serial.println(radio.RSSI);

    // Send an ACK if requested.
    // (You don't need this code if you're not using ACKs.)

    if (radio.ACKRequested()) {
      radio.sendACK();
      Serial.println("ACK sent");
    }
    Blink(LED2, 10);
    return data;
  }
}

ISR(PCINT0_vect) {
  // DRIVE BUTTON CODE
  if (!digitalRead(DRIVE)) { //Testing for low state because of pullup resistors
    if (!driving) Flicker(LIGHT, 1, 200);
    driving = true;
    digitalWrite(PEDAL, HIGH);
  }
}

ISR(PCINT1_vect) {
  if (!digitalRead(STOP)) { //Testing for low state because of pullup resistors
    if (driving) Flicker(LIGHT, 1, 200);
    driving = false;
    digitalWrite(PEDAL, LOW);
  }
  if(digitalRead(A0)) { //RFM69HCW board's interrupt line
  // EMERGENCY STOP CODE
    if (driving && scanRFM() == 'STOP') {
      digitalWrite(PEDAL, LOW);
      driving = false;
      Flicker(LIGHT, 4, 400);
      Flicker(LIGHT, 1, 1000);
      delay(10000);
      while (scanRFM() == 'STOP') {
        digitalWrite(PEDAL, LOW);
      }
    }
  }
}
