#include <RFM69.h>
#include <RFM69_ATC.h>
#include <RFM69_OTA.h>
#include <RFM69registers.h>
#include <avr/sleep.h>
#include <SPI.h>

#define NETWORKID 5  // Must be the same for all nodes
#define MYNODEID 2   // My node ID
#define TONODEID 1   // Destination node ID

#define FREQUENCY RF69_915MHZ

#define ENCRYPT true                   // Set to "true" to use encryption
#define ENCRYPTKEY "TOPSECRETPASSWRD"  // Use the same 16-byte key on all nodes

#define USEACK true

#define DEBUG1 1
#define DEBUG2 2
#define STOP 3 //THIS IS BOTH AN INTERRUPT PIN NUMBER AND DIGITAL PIN NUMBER
#define MOSI 6
#define MISO 5
#define SCK 4
#define CS 7
#define INTERRUPT 8

RFM69 radio;

void setup() {
  // put your setup code here, to run once:
  pinMode(DEBUG1, OUTPUT);
  pinMode(DEBUG2, OUTPUT);
  pinMode(STOP, INPUT);

  radio.initialize(FREQUENCY, MYNODEID, NETWORKID);
  radio.setHighPower();

  if (ENCRYPT)
    radio.encrypt(ENCRYPTKEY);

  attachInterrupt(STOP, stopISR, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
}

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

// SENDING

void sendRFM(String data) {
  char sendbuffer[62];
  int counter = 0;
  while (counter < data.length() && counter < 62) {
    sendbuffer[counter] = data[counter];
    counter++;
  }

  if (USEACK) {
    if (radio.sendWithRetry(TONODEID, sendbuffer, counter)) {
      Serial.println("ACK received!");
      Blink(DEBUG2, 1000);
    } else {
      Serial.println("no ACK received");
      for (int i = 0; i < 3; i++) {
        Blink(DEBUG2, 200);
        delay(200);
      }
    }
  }

  else  // don't use ACK
  {
    radio.send(TONODEID, sendbuffer, counter);
  }
  Blink(DEBUG1, 10);
  return;
}

// RECEIVING

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
    Blink(DEBUG2, 10);
    return data;
  }
}

void stopISR() {
  if (STOP) {
    sendRFM("STOP");
    Blink(DEBUG1, 200);
    delay(800);
  }
}
