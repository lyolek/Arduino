#include <GyverStepper.h>
#include <max6675.h>
#include <Adafruit_MCP23017.h>


#include <Rotary.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


#define encUpPin        3
#define encDownPin      2
#define thermoDO        4
#define thermoCS1       7
#define thermoCS2       8
#define thermoCS3       9
#define thermoCLK       12
#define motor1Step      10
#define motor1Dir       11
#define motor2Step      5
#define motor2Dir       6
#define endSwitchOuter  A3
#define endSwitchInner  A2
#define endSwitchDone   A1

#define mcpRelay1       7
#define mcpRelay2       6
#define mcpRelay3       5

GStepper<STEPPER2WIRE> motor1(200, motor1Step, motor1Dir);
GStepper<STEPPER2WIRE> motor2(200, motor2Step, motor2Dir);


MAX6675 thermocouple1(thermoCLK, thermoCS1, thermoDO);
MAX6675 thermocouple2(thermoCLK, thermoCS2, thermoDO);
MAX6675 thermocouple3(thermoCLK, thermoCS3, thermoDO);


Adafruit_MCP23017 mcp;

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Rotary r = Rotary(encUpPin, encDownPin);


int dT = 1;
int setTemp1 = 28;
int setTemp2 = 28;
int setTemp3 = 28;
float realTemp1 = 300;
float realTemp2 = 300;
float realTemp3 = 300;
int moveLength = 200;

unsigned long lastTempTime;
boolean isFirstIteration = true;

const byte STOPPED      = 0;
const byte FILL_PLAST   = 1;
const byte MOVE_BACK    = 2;
const byte MOVE_FORWARD = 3;
const byte TEST_MCP     = 201;
//byte state = TEST_MCP;
//byte state = STOPPED;
byte state = MOVE_BACK;

void setup() {
  Serial.begin(115200);
  Serial.println("==================Booting=========================");

  mcp.begin();
  mcp.pinMode(mcpRelay1, OUTPUT);
  mcp.pinMode(mcpRelay2, OUTPUT);
  mcp.pinMode(mcpRelay3, OUTPUT);

  
  pinMode(endSwitchOuter,     INPUT_PULLUP);
  pinMode(endSwitchInner,     INPUT_PULLUP);
  pinMode(endSwitchDone,      INPUT_PULLUP);
  
  // режим следования к целевй позиции
  motor1.setRunMode(FOLLOW_POS);
  // установка макс. скорости в шагах/сек
  motor1.setMaxSpeed(100);
  // установка ускорения в шагах/сек/сек
  motor1.setAcceleration(300);
  // отключать мотор при достижении цели
  motor1.autoPower(true);
}

boolean checkTemp1() {
  boolean isSet = true;
  realTemp1 = thermocouple1.readCelsius();
  if(realTemp1 >= setTemp1 + dT) {
    Serial.print("1 - ");
    mcp.digitalWrite(mcpRelay1, LOW);
    Serial.print(mcp.digitalRead(mcpRelay1));
    isSet = false;
  } else if(realTemp1 <= setTemp1 - dT) {
    Serial.print("1 - ");
    mcp.digitalWrite(mcpRelay1, HIGH);
    Serial.print(mcp.digitalRead(mcpRelay1));
    isSet = false;
  }
  Serial.print(" - ");
  Serial.println(realTemp1);
  return isSet;
}

boolean checkTemp2() {
  boolean isSet = true;
  realTemp2 = thermocouple2.readCelsius();
  if(realTemp2 >= setTemp2 + dT) {
    Serial.print("2 - ");
    mcp.digitalWrite(mcpRelay2, LOW);
    Serial.print(mcp.digitalRead(mcpRelay2));
    isSet = false;
  } else if(realTemp2 <= setTemp2 - dT) {
    Serial.print("2 - ");
    mcp.digitalWrite(mcpRelay2, HIGH);
    Serial.print(mcp.digitalRead(mcpRelay2));
    isSet = false;
  }
  Serial.print(" - ");
  Serial.println(realTemp2);
  return isSet;
}

boolean checkTemp3() {
  boolean isSet = true;
  realTemp3 = thermocouple3.readCelsius();
  if(realTemp3 >= setTemp3 + dT) {
    Serial.print("3 - ");
    mcp.digitalWrite(mcpRelay3, LOW);
    Serial.print(mcp.digitalRead(mcpRelay3));
    isSet = false;
  } else if(realTemp3 <= setTemp3 - dT) {
    Serial.print("3 - ");
    mcp.digitalWrite(mcpRelay3, HIGH);
    Serial.print(mcp.digitalRead(mcpRelay3));
    isSet = false;
  }
  Serial.print(" - ");
  Serial.println(realTemp3);
  return isSet;
}

boolean isTempSet() {
  if(millis() - lastTempTime > 500) {
    Serial.println("isTempSet");
    checkTemp1();
    checkTemp2();
    checkTemp3();
    lastTempTime = millis();
  }
  return true;
}

void loop() {

//Serial.println("loop");
  switch (state) {
    case (TEST_MCP) : {
      mcp.digitalWrite(mcpRelay1, LOW);
      Serial.println(mcp.digitalRead(mcpRelay1));
      delay(1000);
      mcp.digitalWrite(mcpRelay1, HIGH);
      Serial.println(mcp.digitalRead(mcpRelay1));
      delay(1000);
      break;
    }
    case (STOPPED) : {
      if(!isTempSet()) {
        
      }
      /*
      Serial.print(mcp.digitalRead(mcpRelay1));
      Serial.print("-");
      Serial.println(realTemp1);
      
      Serial.print(mcp.digitalRead(mcpRelay2));
      Serial.print("-");
      Serial.println(realTemp2);
      
      Serial.print(mcp.digitalRead(mcpRelay3));
      Serial.print("-");
      Serial.println(realTemp3);
      */
      break;
    }
    case (FILL_PLAST) : {
      if (!motor1.tick()) {
        delay(100);
        static bool dir;
        dir = !dir;
        motor1.setTarget(dir ? -50 : 50);
      }
      break;
    }
    case (MOVE_BACK) : {
      isTempSet();
      if (isFirstIteration) {
        Serial.println("Start BACK");
        isFirstIteration = false;
        motor1.setTarget(0);
      }
      if(digitalRead(endSwitchOuter) == 0) {
        motor1.brake();
        motor1.reset();
      }
      if (!motor1.tick()) {
        isFirstIteration = true;
        state = MOVE_FORWARD;
        Serial.println("MOVE_FORWARD");
      }
      break;
    }
    case (MOVE_FORWARD) : {
      isTempSet();
      if (isFirstIteration) {
        Serial.println("Start FORWARD");
        isFirstIteration = false;
        motor1.setTarget(moveLength);
      }
      if(digitalRead(endSwitchInner) == 0) {
        motor1.brake();
      }
      if (!motor1.tick()) {
        isFirstIteration = true;
        state = MOVE_BACK;
        Serial.println("MOVE_BACK");
      }
      break;
    }
    
  }
}
