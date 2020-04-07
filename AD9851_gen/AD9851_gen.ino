#include <Encoder.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <AD985XSPI.h>
#include <SPI.h>

// uncomment folowing line for AD9850 module
// #define DDS_TYPE 0
// uncomment folowing line for AD9851 module
#define DDS_TYPE 1

AD985XSPI DDS(DDS_TYPE);


unsigned long lastButoonPressTime;

const byte encUpPin = 3;
const byte encDownPin = 2;
const byte btnCH = 4;
const byte btnStepUp = A0;
const byte btnStepDn = A2;
const byte btnMode1 = A1;
const byte btnMode2 = A3;
const byte pinSyncSignal = 7;

const int W_CLK_PIN = 13;
const int FQ_UD_PIN = 9;
const int RESET_PIN = 8;

// put your frequency calibration value here
// as measured with an accurate frequency counter
const double trimFreq = 179992260; 

int activeString = 0;
int stp_exp = 5;
unsigned long stp = 100000;
unsigned long f[2] = {5000000, 10000000};
bool isChanelEnabled[3] = {true, false, false};
char mode = 'G';

int sweepStepsNumber; //0.5sec
int sweepCurrStep = 0;
unsigned long sweepFreqStep = 1;
int newEncPos;


SSD1306AsciiAvrI2c oled;
Encoder encoder(encDownPin, encUpPin);

void setup() {
  Serial.begin(9600);
  pinMode(btnCH, INPUT_PULLUP);
  pinMode(btnStepUp, INPUT_PULLUP);
  pinMode(btnStepDn, INPUT_PULLUP);
  pinMode(btnMode1, INPUT_PULLUP);
  pinMode(btnMode2, INPUT_PULLUP);
  DDS.begin(W_CLK_PIN, FQ_UD_PIN, RESET_PIN);

  // uncomment the folowing line to apply frequency calibration
  DDS.calibrate(trimFreq); 


  bool i2c_found = true;
  if(!i2c_found) {
    Serial.println("Device not found on I2C bus!");
  } 

  setFrequency();

//Serial.println("pre init disp");
  oled.begin(&Adafruit128x64, 0x3C);
//Serial.println("disp inited");
  redrawDisplay();
}



void setFrequency() {
  DDS.setfreq(f[0]);
}

unsigned long getStep(int s) {
  switch(s) {
    case 0:
      return 1;
    case 1:
      return 10;
    case 2:
      return 100;
    case 3:
      return 1000;
    case 4:
      return 10000;
    case 5:
      return 100000;
    case 6:
      return 1000000;
    case 7:
      return 10000000;
  }
}
void redrawDisplay() {

Serial.println("1");
  oled.setFont(Adafruit5x7);
  oled.clear();
  oled.set2X();

//Serial.println("2");
  oled.set1X();
  oled.print((activeString == 0 & mode != 'S') ? "*" : " ");
  oled.set2X();
  oled.println(formatFreq(f[0]));
  oled.set1X();
  oled.println(" ");
  oled.set2X();
//Serial.println("3"); 
  oled.set1X();
  oled.print((activeString == 1 & mode != 'S') ? "*" : " ");
  oled.set2X();
  oled.println(formatFreq(f[1]));

  oled.set1X();
  oled.println(" ");
//Serial.println("5");
  oled.print(" ");
  oled.set2X();
  oled.println(formatFreq(stp));
  
}

String formatFreq(unsigned long f) {
  char s[15];
  int n1 = f / 1000000L;
//Serial.println(n1);
  int n2 = (f - n1*1000000L)/1000L;
//Serial.println(n2);
  int n3 = f - n1*1000000L - n2*1000L;
  sprintf(s, "%2d.%03d.%03d", n1, n2, n3);
  return s;
}

void loop() {

  if(mode == 'S') {

Serial.println("mode=s");
    analogWrite(pinSyncSignal, 1024);
    sweepFreqStep = (f[1] - f[0])/sweepStepsNumber;
    for(sweepCurrStep = 0; sweepCurrStep < sweepStepsNumber; sweepCurrStep++) {
      if(sweepCurrStep == sweepStepsNumber/2) {
        analogWrite(pinSyncSignal, 0);
      }
      unsigned long freq = f[0] + (sweepFreqStep*sweepCurrStep);
      DDS.setfreq(freq);
      newEncPos = encoder.read();
      delay(2);
    }
  }

  newEncPos = encoder.read();
  
//Serial.println(newEncPos);
  if (newEncPos >= 4 || newEncPos <= -4) {
    encoder.write(0);
    if(mode == 'G') {
      f[activeString] = f[activeString] + newEncPos/4*stp;
      encoder.write(0);
      setFrequency();
    } else {
      mode = 'G';
    }
Serial.println("e");
    redrawDisplay();
  }
  if(digitalRead(btnCH) == 0 && (millis() - lastButoonPressTime) > 200) {
    lastButoonPressTime = millis();
    activeString = activeString == 1 ? 0 : 1;
Serial.println("c");
    redrawDisplay();
  }
  if(digitalRead(btnStepUp) == 0 && (millis() - lastButoonPressTime) > 200) {
    lastButoonPressTime = millis();
    stp_exp++;
    if(stp_exp > 7) {
      stp_exp = 7;
    }
    stp = getStep(stp_exp);
Serial.println("u");
    redrawDisplay();
  }
  if(digitalRead(btnStepDn) == 0 && (millis() - lastButoonPressTime) > 200) {
    lastButoonPressTime = millis();
    stp_exp--;
    if(stp_exp < 0) {
      stp_exp = 0;
    }
    stp = getStep(stp_exp);
Serial.println("d");
    redrawDisplay();
  }
  if(digitalRead(btnMode1) == 0 && (millis() - lastButoonPressTime) > 200) {
    lastButoonPressTime = millis();
    mode = 'S';
    sweepStepsNumber = 240; //0.5s
Serial.println("m1");
    redrawDisplay();
  }
  if(digitalRead(btnMode2) == 0 && (millis() - lastButoonPressTime) > 200) {
    lastButoonPressTime = millis();
    mode = 'S';
    sweepStepsNumber = 4789; //10s
Serial.println("m2");
    redrawDisplay();
  }

}
