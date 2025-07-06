#include <Servo.h>

Servo servo1;
Servo servo2;

int sliderStatus = 0;

int sliderActionTaken = 0;

#define SERVO1_PIN 9
#define SERVO2_PIN 10
#define LED_PIN 7

void setup() {
  Serial.begin(9600); // UART from Arduino Uno #1

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  pinMode(LED_PIN, OUTPUT);

  // Set initial positions: CLOSED (servo1 = 0, servo2 = 70)
  servo1.write(0);
  servo2.write(70);
  digitalWrite(LED_PIN, LOW);
}

void loop() {

  sliderActionTaken = 0;
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove any trailing newline or whitespace

    if (command == "OPEN_SLIDER") {
      if (sliderActionTaken == 0 && sliderStatus != 1) {
      openSlider();
      Serial.print("slider is open\n");
      sliderActionTaken = 1;
    }
    } 
    else if (command == "CLOSE_SLIDER") {
      if (sliderActionTaken == 0 && sliderActionTaken != 0) {
      closeSlider();
      Serial.print("slider is Closed \n");
      sliderActionTaken = 1;
    }
    }
  }

Serial.print("Current ceiling status: ");
Serial.println(sliderStatus == 1 ? "OPEN" : "CLOSED"); /////tracing 

}

void openSlider() {
  // Move to OPEN position: servo1 = 70, servo2 = 0
  digitalWrite(LED_PIN, HIGH);
  servo1.write(70);
  servo2.write(0);
sliderStatus = 1;

}

void closeSlider() {
  // Move to CLOSED position: servo1 = 0, servo2 = 70
  digitalWrite(LED_PIN, LOW);
  servo1.write(0);
  servo2.write(70);
  sliderStatus = 0;

 
}
