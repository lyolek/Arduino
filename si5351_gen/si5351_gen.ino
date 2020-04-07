#include <si5351.h>
#include <Encoder.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

unsigned long lastButoonPressTime;

const byte encUpPin = 8;
const byte encDownPin = 7;
const byte btnCH = 9;
const byte btnStepUp = 6;
const byte btnStepDn = 10;
const byte btnMode1 = 12;
const byte btnMode2 = 11;
const byte pinSyncSignal = A0;

//bool intFire = false;
int activeString = 0;
int stp_exp = 5;
unsigned long stp = 10000000;
unsigned long f[3] = {710000000L, 370000000L, 2800000000L};
bool isChanelEnabled[3] = {true, false, false};
char mode = 'G';

int sweepStepsNumber; //0.5sec
int sweepCurrStep = 0;
unsigned long sweepFreqStep = 1;
int newEncPos;


Si5351 si5351;
SSD1306AsciiWire oled;
Encoder encoder(encUpPin, encDownPin);

void setup() {
  Serial.begin(9600);
Serial.println("init");
  pinMode(btnCH, INPUT_PULLUP);
  pinMode(btnStepUp, INPUT_PULLUP);
  pinMode(btnStepDn, INPUT_PULLUP);
  pinMode(btnMode1, INPUT_PULLUP);
  pinMode(btnMode2, INPUT_PULLUP);
//Serial.println("pinMod Ok");

  

  bool i2c_found = true;
Serial.println("si pre init");
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
Serial.println("si init");
  if(!i2c_found) {
    Serial.println("Device not found on I2C bus!");
  } 
//Serial.println("si inited");
si5351.set_correction(117800, SI5351_PLL_INPUT_XO);


si5351.output_enable(SI5351_CLK0, 1);
si5351.output_enable(SI5351_CLK1, 1);
si5351.output_enable(SI5351_CLK2, 1);

//Serial.println("si corrected");
  setFrequency();

//Serial.println("pre init disp");
  oled.begin(&Adafruit128x64, 0x3C);
  oled.set400kHz();
//Serial.println("disp inited");
  redrawDisplay();
}

unsigned long getStep(int s) {
  switch(s) {
    case 0:
      return 100;
    case 1:
      return 1000;
    case 2:
      return 10000;
    case 3:
      return 100000;
    case 4:
      return 1000000;
    case 5:
      return 10000000;
    case 6:
      return 100000000L;
    case 7:
      return 1000000000L;
  }
  
}

void setFrequency() {

//Serial.println("setF");
  si5351.set_freq(f[0], SI5351_CLK0);
//Serial.println("setF1");
  si5351.set_freq(f[1], SI5351_CLK1);
//Serial.println("setF2");
  si5351.set_freq(f[2], SI5351_CLK2);
//Serial.println("setF3");
  si5351.update_status();
//Serial.println("setF");

}

void redrawDisplay() {

Serial.println("1");
  oled.setFont(Adafruit5x7);
  oled.clear();

//Serial.println("2");
  oled.print(activeString == 0 ? "*" : " ");
  oled.println(formatFreq(f[0]));
  oled.println(" ");
//Serial.println("3"); 
  oled.print(activeString == 1 ? "*" : " ");
  oled.println(formatFreq(f[1]));
  oled.println(" ");

//Serial.println("4"); 
  if(mode == 'G') { 
    oled.print(activeString == 2 ? "*" : " ");
    oled.println(formatFreq(f[2]));
  }
  oled.println(" ");
//Serial.println("5");
  oled.print(" ");
  oled.println(formatFreq(stp));

Serial.println("7"); 
  
}

String formatFreq(unsigned long f) {
  f = f/100;
  char s[15];
  int n1 = f / 1000000L;
//Serial.println(n1);
  int n2 = (f - n1*1000000L)/1000L;
//Serial.println(n2);
  int n3 = f - n1*1000000L - n2*1000L;
  sprintf(s, "%2d.%3d.%3d", n1, n2, n3);
  return s;
}

void loop() {

  if(mode == 'S') {
    analogWrite(pinSyncSignal, 1024);
    sweepFreqStep = (f[1] - f[0])/sweepStepsNumber;
    for(sweepCurrStep = 0; sweepCurrStep < sweepStepsNumber; sweepCurrStep++) {
      if(sweepCurrStep == sweepStepsNumber/2) {
        analogWrite(pinSyncSignal, 0);
      }
      unsigned long freq = f[0] + (sweepFreqStep*sweepCurrStep);
      si5351.set_freq(freq, SI5351_CLK1);
      newEncPos = encoder.read();
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
      si5351.output_enable(SI5351_CLK0, 1);
      si5351.output_enable(SI5351_CLK2, 1);
    }
Serial.println("e");
    redrawDisplay();
  }
  if(digitalRead(btnCH) == 0 && (millis() - lastButoonPressTime) > 200) {
    lastButoonPressTime = millis();
    activeString++;
    if(activeString > 2) {
      activeString = 0;
    }
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
    sweepStepsNumber = 156; //0.2s
    si5351.output_enable(SI5351_CLK0, 0);
    si5351.output_enable(SI5351_CLK2, 0);
Serial.println("m");
    redrawDisplay();
  }
  if(digitalRead(btnMode2) == 0 && (millis() - lastButoonPressTime) > 200) {
    lastButoonPressTime = millis();
    mode = 'S';
    sweepStepsNumber = 7800; //10s
    si5351.output_enable(SI5351_CLK0, 0);
    si5351.output_enable(SI5351_CLK2, 0);
Serial.println("m");
    redrawDisplay();
  }
//  delay(1);
}
