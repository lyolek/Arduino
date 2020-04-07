#include <Servo.h>

int servoPin = 2;

Servo servo;

int angle = 0;

void setup() {
  Serial.begin(9600);
  servo.attach(servoPin);
}

void loop() {

  for(angle = 0; angle < 180; angle++) {
    servo.write(angle);
    Serial.println(angle);
    delay(1000);
  }
  for(angle = 180; angle > 0; angle--) {
    servo.write(angle);
    delay(10);
  }

}
