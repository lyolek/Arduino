#include <Rotary.h>
#include <si5351.h> 
#include <SPI.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"


unsigned long lastButtonPressTime;

#define encUpPin    3
#define encDownPin  2
#define btnStep     4
#define voltagePin  A6
#define bpf80Pin    7
#define bpf40Pin    8
#define bpf20Pin    6
#define bpf10Pin    5
#define bpfBtn      A3


unsigned long stp = 250;
String stpS = "250";
int bpfState = 80;
int digitalVoltage = 0;
long freq = 3700000L;

boolean changed_f = 0;
boolean encUp = false;
boolean encDown = false;

String encoderMode = "F";

SSD1306AsciiAvrI2c oled;
Si5351 si5351;
Rotary r = Rotary(encUpPin, encDownPin);

/**************************************************************************
*  Declarations
**************************************************************************/
//volatile uint32_t vfo_r = 370000000ULL;                      // CLK1 start Rx freq


/**************************************/
/* Interrupt service routine for      */
/* encoder frequency change           */
/**************************************/
ISR(PCINT2_vect) {
  char result = r.process();
  if (result == DIR_CW) {
    encUp = true;
  } else if (result == DIR_CCW) {
    encDown = true;
  }
}


/**************************************/
/* Change the frequency               */
/* dir = 1    Increment               */
/* dir = -1   Decrement               */
/**************************************/
void set_frequency(short dir) {
  
  if (dir == 1) {
    freq += stp;
  }
  if (dir == -1) {
    freq -= stp;
  }
  si5351.set_freq(freq*100, SI5351_CLK1);
}





String formatFreq(long f) {
  char s[10];
  int n1 = f / 1000000L;
//Serial.println(n1);
  int n2 = (f - n1*1000000L)/1000L;
//Serial.println(n2);
  int n3 = f - n1*1000000L - n2*1000L;
  sprintf(s, "%2u.%03u.%03u", n1, n2, n3);
  return s;
}

void redrawDisplay() {

//Serial.println("1");
  oled.setFont(Adafruit5x7);
  oled.clear();
  oled.set2X();

  oled.print(digitalVoltage*0.01907);
  oled.print("V  ");
  oled.println(bpfState);
//Serial.println("2");
  oled.println(formatFreq(freq));


//Serial.println("2");
  if(encoderMode == "S") {
    oled.print(">");
  } else {
    oled.print(" ");
  }
  oled.print(stpS);

//Serial.println("7"); 

}



/**************************************/
/*            S E T U P               */
/**************************************/
void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(btnStep, INPUT_PULLUP);
  pinMode(bpfBtn, INPUT_PULLUP);
  pinMode(bpf80Pin, OUTPUT);
  pinMode(bpf40Pin, OUTPUT);
  pinMode(bpf20Pin, OUTPUT);
  pinMode(bpf10Pin, OUTPUT);
  digitalWrite(bpf80Pin, HIGH);
  digitalWrite(bpf40Pin, HIGH);
  digitalWrite(bpf20Pin, LOW);
  digitalWrite(bpf10Pin, LOW);
  
//initialize the Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0); //If you're using a 27Mhz crystal, put in 27000000 instead of 0
  si5351.set_correction(67400); // Set to zero because I'm using an other calibration method
  si5351.drive_strength(SI5351_CLK1,SI5351_DRIVE_8MA);

  
  si5351.set_freq(freq*100, SI5351_CLK1);


  oled.begin(&Adafruit128x64, 0x3C);
  
  PCICR |= (1 << PCIE2);                       // Enable pin change interrupt for the encoder
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);

  sei();
  
// Display first time  
  redrawDisplay();  // Update the display
}


void loop() {

  if(encDown) {
    if(encoderMode == "F") {
      set_frequency(-1);
    } else if(encoderMode == "S") {
      switch (stp) {
        case 250: {
          stp = 10;
          stpS = "10";
          break;
        }
        case 1000: {
          stp = 250;
          stpS = "250";
          break;
        }
        case 10000: {
          stp = 1000;
          stpS = "1k";
          break;
        }
        case 100000: {
          stp = 10000;
          stpS = "10k";
          break;
        }
      }
    }
    encDown = false;
    encUp= false;
    redrawDisplay();
  } else if(encUp) {
    if(encoderMode == "F") {
      set_frequency(1);
    } else if(encoderMode == "S") {
      switch (stp) {
        case 10: {
          stp = 250;
          stpS = "250";
          break;
        }
        case 250: {
          stp = 1000;
          stpS = "1k";
          break;
        }
        case 1000: {
          stp = 10000;
          stpS = "10k";
          break;
        }
        case 10000: {
          stp = 100000;
          stpS = "100k";
          break;
        }
      }
    }
    encDown = false;
    encUp= false;
    redrawDisplay();
  }
  if(digitalRead(btnStep) == 0 && (millis() - lastButtonPressTime) > 200) {
    lastButtonPressTime = millis();
    if(encoderMode != "S"){
      encoderMode = "S";
    } else {
      encoderMode = "F";
    }
    redrawDisplay();
  }
  else if(digitalRead(bpfBtn) == 0 && (millis() - lastButtonPressTime) > 200) {
    lastButtonPressTime = millis();
    switch (bpfState) {
      case (80) : {
        bpfState = 40;
        digitalWrite(bpf80Pin, LOW);
        digitalWrite(bpf40Pin, HIGH);
        digitalWrite(bpf20Pin, LOW);
        digitalWrite(bpf10Pin, LOW);
        break;
      }
      case (40) : {
        bpfState = 20;
        digitalWrite(bpf80Pin, LOW);
        digitalWrite(bpf40Pin, LOW);
        digitalWrite(bpf20Pin, HIGH);
        digitalWrite(bpf10Pin, HIGH);
        break;
      }
      case (20) : {
        bpfState = 10;
        digitalWrite(bpf80Pin, LOW);
        digitalWrite(bpf40Pin, LOW);
        digitalWrite(bpf20Pin, LOW);
        digitalWrite(bpf10Pin, HIGH);
        break;
      }
      case (10) : {
        bpfState = 80;
        digitalWrite(bpf80Pin, HIGH);
        digitalWrite(bpf40Pin, HIGH);
        digitalWrite(bpf20Pin, LOW);
        digitalWrite(bpf10Pin, LOW);
        break;
      }
    }
    redrawDisplay();
  }

  if(digitalVoltage - analogRead(voltagePin) > 2 || digitalVoltage - analogRead(voltagePin) < -2) {
    digitalVoltage = analogRead(voltagePin);
    
    redrawDisplay();
  }
  delay(10);

} // end while loop
