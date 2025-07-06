#include <Wire.h>
#include <SoftwareSerial.h>
SoftwareSerial SLAVE_SERVO_SERIAL(2, 3); // RX, TX

const byte I2C_ADDRESS = 8;

#define IN1 6
#define IN2 7
#define IN3 8
#define IN4 9
#define ENA 5
#define ENB 10

const int soil_moist_pin1 = A0;
const int soil_moist_pin2 = A1;
const int soil_moist_pin3 = A2;

#define water_motor_pin 4
#define rain_sens 12
#define TRIGGER_PIN 13
#define ECHO_PIN 11
#define tank_threshold 12

int tank_flag = 0;
int waterPump_flag = 0;
int ceilingEnable = 0;
int pumpEnable = 0;

long duration;
int distance;

int ceilingStatus = 0;
int dry = 0;
int ceilingActionTaken = 0;

volatile int receivedPrediction = 0;
int rainToday = 0;

float soil_moist(int pin) {
  return analogRead(pin);
}

void send_sensors_data() {
  int soil1 = analogRead(soil_moist_pin1);
  int soil2 = analogRead(soil_moist_pin2);
  int soil3 = analogRead(soil_moist_pin3);
  Wire.write(rainToday >> 8); Wire.write(rainToday & 0xFF);
  Wire.write(soil1 >> 8); Wire.write(soil1 & 0xFF);
  Wire.write(soil2 >> 8); Wire.write(soil2 & 0xFF);
  Wire.write(soil3 >> 8); Wire.write(soil3 & 0xFF);
  Wire.write(distance >> 8); Wire.write(distance & 0xFF);
  Wire.write(waterPump_flag);
  Wire.write(ceilingStatus);
  Wire.write(tank_flag);
}

void receive_data(int data_size) {
  if (data_size >= 4) {
    int highByte = Wire.read();
    int lowByte = Wire.read();
    receivedPrediction = (highByte << 8) | lowByte;
    ceilingEnable = Wire.read();
    pumpEnable = Wire.read();
  }
  while (Wire.available()) Wire.read(); // Flush any extra
}

void openCeiling() {
  analogWrite(ENA, 255); analogWrite(ENB, 255);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); // A+B: Push
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); // C+D: Pull
  delay(2000);
  stopCeiling();
  ceilingStatus = 1;
}

void closeCeiling() {
  analogWrite(ENA, 255); analogWrite(ENB, 255);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); // A+B: Pull
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); // C+D: Push
  delay(2000);
  stopCeiling();
  ceilingStatus = 0;
}

void stopCeiling() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW); digitalWrite(ENA, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW); digitalWrite(ENB, LOW);
}

int tank_Check() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration * 0.0343) / 2;
  return distance;
}

void checkSoilDryStatus() {
  dry = 0;
  const int DRY_THRESHOLD = 600;
  float moist[3] = { soil_moist(A0), soil_moist(A1), soil_moist(A2) };
  for (int i = 0; i < 3; i++) {
    if (moist[i] > DRY_THRESHOLD) {
      dry = 1;
      break;
    }
  }
}

void irregate() {
  if (pumpEnable == 1 && dry == 1 && rainToday == 0) {
    digitalWrite(water_motor_pin, LOW);
    Serial.println("Water pump ON");
    waterPump_flag = 1;
  } else {
    digitalWrite(water_motor_pin, HIGH);
    Serial.println("Water pump OFF");
    waterPump_flag = 0;
  }

  if (ceilingEnable == 1 && ceilingActionTaken == 0 && ceilingStatus != 1) {
    openCeiling();
    Serial.println("Ceiling is opened by user command.");
    ceilingActionTaken = 1;
  } else if (ceilingEnable == 0 && ceilingActionTaken == 0 && ceilingStatus != 0) {
    closeCeiling();
    Serial.println("Ceiling is closed by user command.");
    ceilingActionTaken = 1;
  }

  // AI-based actions if no manual override
  if (ceilingActionTaken == 0) {
    if (receivedPrediction == 1 && dry == 0) {
      closeCeiling();
      Serial.println("AI: Rain predicted, soil wet → ceiling closed.");
      ceilingActionTaken = 1;
    } else if (receivedPrediction == 1 && dry == 1) {
      openCeiling();
      Serial.println("AI: Rain predicted, soil dry → ceiling opened.");
      ceilingActionTaken = 1;
    } else if (receivedPrediction == 0 && dry == 1) {
      openCeiling();
      Serial.println("AI: No rain predicted, soil dry → ceiling opened.");
      ceilingActionTaken = 1;
    }
  }
}

void setup() {
  Serial.begin(115200);
  SLAVE_SERVO_SERIAL.begin(9600);
  pinMode(soil_moist_pin1, INPUT);
  pinMode(soil_moist_pin2, INPUT);
  pinMode(soil_moist_pin3, INPUT);
  pinMode(rain_sens, INPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(water_motor_pin, OUTPUT);
  pinMode(TRIGGER_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(receive_data);
  Wire.onRequest(send_sensors_data);
}

void loop() {
  ceilingActionTaken = 0;

  checkSoilDryStatus();

  int rain_value = digitalRead(rain_sens);
  Serial.print("Prediction: "); Serial.print(receivedPrediction);
  Serial.print(" | Rain sensor: "); Serial.print(rain_value);

  // Rain sensor logic
  if (rain_value == 0) {
    rainToday = 1;
    digitalWrite(water_motor_pin, HIGH);
    waterPump_flag = 0;

    if (ceilingStatus != 0 && ceilingActionTaken == 0) {
      closeCeiling();
      Serial.println("Ceiling closed due to rain sensor.");
      ceilingActionTaken = 1;
    }
  } else {
    rainToday = 0;
  }

  Serial.print(" | rainToday: "); Serial.println(rainToday);

  if (rainToday != 1) {
    irregate();
  }

  Serial.print("Ceiling status: ");
  Serial.println(ceilingStatus == 1 ? "OPEN" : "CLOSED");

  int tank_lvl = tank_Check();
  Serial.print("Tank distance: "); Serial.print(distance); Serial.println(" cm");

  if (tank_lvl > tank_threshold) {
    if (receivedPrediction == 1 || rain_value == 0) {
      SLAVE_SERVO_SERIAL.println("OPEN_SLIDER");
      SLAVE_SERVO_SERIAL.println(" ");
    } else if (receivedPrediction == 0) {
      Serial.println("[ALERT] Tank is low and no rain predicted.");
      tank_flag = 1;
    }
  } else {
    SLAVE_SERVO_SERIAL.println("CLOSE_SLIDER");
    SLAVE_SERVO_SERIAL.println(" ");
    tank_flag = 0;
  }

  delay(3000);
}
