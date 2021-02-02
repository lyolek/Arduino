#include <Rotary.h>
#include <si5351.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


unsigned long lastButtonPressTime;

#define encUpPin    3
#define encDownPin  2
#define voltagePin  A7
#define ATTPin      6
#define PREPin      7
#define bpf80Pin    8
#define bpf40Pin    9
#define bpf20Pin    10
#define bpf15Pin    11
#define bpf10Pin    12
#define bpfBtn      A3
//#define mixModBtn A2
#define isBpfBtn    A2
//#define isVFOBtn  A1
#define AmpBtn      A1
#define SSBBtn      A0


int bpfState = 80;
int digitalVoltage = 0;
unsigned long freq = 3700000;
unsigned long freqInt = 9213700;
unsigned long freqInt1 = 9213700L;
unsigned long freqInt2 = 9216400L;
boolean isMixAdd = true;
boolean isVFO = true;
boolean isLSB = true;
boolean isBpf = true;
int isAMP = 0;

boolean changed_f = 0;
int encMoveReduce = 0;
int encMove = 0;

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
Si5351 si5351;
Rotary r = Rotary(encUpPin, encDownPin);


/**************************************/
/* Interrupt service routine for      */
/* encoder frequency change           */
/**************************************/
ISR(PCINT2_vect) {
  char result = r.process();
  if (result == DIR_CW) {
    encMoveReduce++;
  } else if (result == DIR_CCW) {
    encMoveReduce--;
  }
  if(encMoveReduce == -6) {
    encMove--;
    encMoveReduce = 0;
  } else if(encMoveReduce == 6) {
    encMoveReduce = 0;
    encMove++;
  }
}


/**************************************/
/* Change the frequency               */
/* dir = 1    Increment               */
/* dir = -1   Decrement               */
/**************************************/
void changeFrequency(int m) {
  if(isVFO) {
    freq += 10*m;
  } else {
    freqInt += 10*m;
  }
  setFrequency();
}

void setFrequency() {
  long f = 0L;
  unsigned long ff = 0L;
  if(isMixAdd) {
    f = freq + freqInt;
    ff = f*100;
  } else {
    f = freq - freqInt;
    f = abs(f);
    ff = f*100;
  }
  
//Serial.println(f);
//Serial.println(ff);
  si5351.set_freq(ff, SI5351_CLK0);
  si5351.set_freq(freqInt*100, SI5351_CLK2);
}

void changeIF() {
  if(isMixAdd) {
    if(isLSB) {
      freqInt2 = freqInt;
      freqInt = freqInt1;
    } else {
      freqInt1 = freqInt;
      freqInt = freqInt2;
    }
  } else {
    if(isLSB) {
      freqInt1 = freqInt;
      freqInt = freqInt2;
    } else {
      freqInt2 = freqInt;
      freqInt = freqInt1;
    }
  }
  setFrequency();
}

/*
void changeIF() {
  setFrequency();
}
*/


/*
String formatFreq(unsigned long f) {
  char s[10];
  int n1 = f / 1000000L;
//Serial.println(n1);
  int n2 = (f - n1*1000000L)/1000L;
//Serial.println(n2);
  int n3 = f - n1*1000000L - n2*1000L;
  sprintf(s, "%2u.%03u.%03u", n1, n2, n3);
  return s;
}
*/

String formatFreq(unsigned long f) {
  int n1 = f / 1000000L;
//Serial.println(n1);
  int n2 = (f - n1*1000000L)/1000L;
//Serial.println(n2);
  int n3 = f - n1*1000000L - n2*1000L;
  return String(n1) + "." + (n2 < 100 ? "0" : "") + (n2 < 10 ? "0" : "") + String(n2) + "." + (n3 < 100 ? "0" : "") + (n3 < 10 ? "0" : "") + String(n3);
}

void redrawDisplay() {
  u8g2.setFont(u8g2_font_profont22_mr);
  u8g2.firstPage();
  do {
    u8g2.setCursor(0, 20);
    u8g2.print(digitalVoltage*0.01907);    
    u8g2.print(F("V"));
    if(isMixAdd) {
      u8g2.print(F(" + "));
    } else {
      u8g2.print(F(" - "));
    }
    if(isBpf) {
      u8g2.print(bpfState);
    } else { 
      u8g2.print("--");
    }
    u8g2.setCursor(0, 40);
    if(isVFO) {
      u8g2.print(formatFreq(freq));
    } else {
      u8g2.print(formatFreq(freqInt));
    }

    u8g2.setCursor(0, 60);
    if(isVFO) {
      u8g2.print("V ");
    } else {
      u8g2.print("B ");
    }

    if(isLSB) {
      u8g2.print("L ");
    } else {
      u8g2.print("U ");
    }

    if(isAMP == 0) {
      u8g2.print("---");
    } else if (isAMP == 1) {
      u8g2.print("PRE");
    } else if (isAMP == 2) {
      u8g2.print("ATT");
    }
  } while ( u8g2.nextPage() );
}



/**************************************/
/*            S E T U P               */
/**************************************/
void setup() {
  Serial.begin(115200);
  Wire.begin();

Serial.println("1");
  pinMode(bpfBtn,     INPUT_PULLUP);
//  pinMode(mixModBtn,  INPUT_PULLUP);
  pinMode(isBpfBtn,  INPUT_PULLUP);
//  pinMode(isVFOBtn,   INPUT_PULLUP);
  pinMode(SSBBtn,     INPUT_PULLUP);
  pinMode(AmpBtn,     INPUT_PULLUP);
  pinMode(bpf80Pin, OUTPUT);
  pinMode(bpf40Pin, OUTPUT);
  pinMode(bpf20Pin, OUTPUT);
  pinMode(bpf15Pin, OUTPUT);
  pinMode(bpf10Pin, OUTPUT);
  pinMode(PREPin, OUTPUT);
  pinMode(ATTPin, OUTPUT);
  digitalWrite(bpf80Pin, HIGH);
  digitalWrite(bpf40Pin, LOW);
  digitalWrite(bpf20Pin, LOW);
  digitalWrite(bpf15Pin, LOW);
  digitalWrite(bpf10Pin, LOW);
  digitalWrite(PREPin, LOW);
  digitalWrite(ATTPin, LOW);
  
Serial.println("2");
//initialize the Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 116300);
  si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_2MA);
  si5351.drive_strength(SI5351_CLK2,SI5351_DRIVE_2MA);

Serial.println("3");
  
  setFrequency();

Serial.println("4");

  u8g2.begin();
  
  PCICR |= (1 << PCIE2);                       // Enable pin change interrupt for the encoder
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);

  sei();
  
// Display first time  
  redrawDisplay();  // Update the display
Serial.println("5");
}


void loop() {

  if(encMove) {
    changeFrequency(encMove);
    encMove = 0;
    redrawDisplay();
  }
  if(digitalRead(bpfBtn) == 0 && (millis() - lastButtonPressTime) > 200) {
    lastButtonPressTime = millis();
    switch (bpfState) {
      case (80) : {
        bpfState = 40;
        digitalWrite(bpf80Pin, LOW);
        digitalWrite(bpf40Pin, LOW);
        digitalWrite(bpf20Pin, LOW);
        digitalWrite(bpf15Pin, LOW);
        digitalWrite(bpf10Pin, HIGH);
        freq = 7100000L;
        setFrequency();
        break;
      }
      case (40) : {
        bpfState = 20;
        digitalWrite(bpf80Pin, LOW);
        digitalWrite(bpf40Pin, LOW);
        digitalWrite(bpf20Pin, LOW);
        digitalWrite(bpf15Pin, LOW);
        digitalWrite(bpf10Pin, HIGH);
        freq = 14100000L;
        setFrequency();
        break;
      }
      case (20) : {
        bpfState = 15;
        digitalWrite(bpf80Pin, LOW);
        digitalWrite(bpf40Pin, LOW);
        digitalWrite(bpf20Pin, LOW);
        digitalWrite(bpf15Pin, LOW);
        digitalWrite(bpf10Pin, HIGH);
        freq = 21100000L;
        setFrequency();
        break;
      }
      case (15) : {
        bpfState = 10;
        digitalWrite(bpf80Pin, LOW);
        digitalWrite(bpf40Pin, LOW);
        digitalWrite(bpf20Pin, LOW);
        digitalWrite(bpf15Pin, LOW);
        digitalWrite(bpf10Pin, HIGH);
        freq = 28100000L;
        setFrequency();
        break;
      }
      case (10) : {
        bpfState = 80;
        digitalWrite(bpf80Pin, HIGH);
        digitalWrite(bpf40Pin, LOW);
        digitalWrite(bpf20Pin, LOW);
        digitalWrite(bpf15Pin, LOW);
        digitalWrite(bpf10Pin, LOW);
        freq = 3700000L;
        setFrequency();
        break;
      }
    }
    redrawDisplay();
  } else if(digitalRead(isBpfBtn) == 0 && (millis() - lastButtonPressTime) > 200) {
    lastButtonPressTime = millis();
    if(isBpf) {;
      digitalWrite(bpf80Pin, LOW);
      digitalWrite(bpf40Pin, LOW);
      digitalWrite(bpf20Pin, LOW);
      digitalWrite(bpf15Pin, LOW);
      digitalWrite(bpf10Pin, HIGH);
      isBpf = false;
    } else {
      digitalWrite(bpf80Pin, HIGH);
      digitalWrite(bpf40Pin, LOW);
      digitalWrite(bpf20Pin, LOW);
      digitalWrite(bpf15Pin, LOW);
      digitalWrite(bpf10Pin, LOW);
      isBpf = true;
    }
    redrawDisplay();
//  } else if(digitalRead(mixModBtn) == 0 && (millis() - lastButtonPressTime) > 200) {
//    lastButtonPressTime = millis();
//    isMixAdd = !isMixAdd;
//    changeIF();
//    redrawDisplay();
//  } else if(digitalRead(isVFOBtn) == 0 && (millis() - lastButtonPressTime) > 200) {
//    lastButtonPressTime = millis();
//    isVFO = !isVFO;
//    redrawDisplay();
  } else if(digitalRead(SSBBtn) == 0 && (millis() - lastButtonPressTime) > 200) {
    lastButtonPressTime = millis();
    isLSB = !isLSB;
    changeIF();
    redrawDisplay();
  } else if(digitalRead(AmpBtn) == 0 && (millis() - lastButtonPressTime) > 200) {
    lastButtonPressTime = millis();
    if(isAMP == 0) {
      isAMP = 1;
      digitalWrite(ATTPin, LOW);
      digitalWrite(PREPin, HIGH);
    } else if (isAMP == 1) {
      isAMP = 2;
      digitalWrite(ATTPin, HIGH);
      digitalWrite(PREPin, LOW);
    } else if (isAMP == 2) {
      isAMP = 0;
      digitalWrite(ATTPin, LOW);
      digitalWrite(PREPin, LOW);
    }
    redrawDisplay();
    
  }

  if(digitalVoltage - analogRead(voltagePin) > 2 || digitalVoltage - analogRead(voltagePin) < -2) {
    digitalVoltage = analogRead(voltagePin);
    
//    redrawDisplay();
  }
Serial.println("loop");
  delay(10);

} // end while loop
