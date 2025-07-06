// #include <Vector.h>
#include "DHT.h"
// #include <Adafruit_BMP085_U.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;


// std::vector<float> dailyTemps;

// Sensor Pins
#define DHT_PIN 27
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

TwoWire I2CBMP = TwoWire(1); // Secondary I2C instance for BMP180
// Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);


// WiFi Credentials
#define WIFI_SSID "TEdataE596901"
#define WIFI_PASSWORD "10900587$Mm"

// Firebase Credentials
#define FIREBASE_HOST "smart-irrigation-system-fdc78-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "23QRbRfWumetCmRMWNs5gFXKbhwCJX9cpHJRjTqs"

FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

FirebaseData streamData;


// NTP Client Setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * 2, 60000);  // GMT+2

#define _9am 9
#define _3pm 15

int sent9amFlag = 0;
int sent3pmFlag = 0;


//app control flags
int ceilingEnableFlag = 0;  // Default = enabled
int pumpEnableFlag = 0;     // Default = enabled


// daily record variables for temp
float minTemp = 50.0;
float maxTemp = -10.0;
float temp9am, temp3pm, hum9am, hum3pm, pressure9am, pressure3pm;
float prediction;


int raintoday = 0;
int lastSentDay = -1;

// I2C Address for Arduino
const byte I2C_ARDUINO_ADDR = 8;
const String apiUrl = "https://rainpredictionflask-production.up.railway.app/";

// Function Prototypes
void update_day();
void sendAndReceiveSensorData(int prediction);
float sendPostRequest(float mintemp,float maxtemp,float hum9am,float hum3pm,float pressure9am,float pressure3pm,float temp9am,float temp3pm,int raintoday);


void updateMinMaxTemp(float temp) {
  int today = timeClient.getDay();

  if (today != lastSentDay) {
    // Reset min and max at the start of a new day
    minTemp = 60.0;
    maxTemp = -10.0;
    lastSentDay = today;
  }

  // Update min and max based on current reading
  if (temp >= -40.0f && temp <= 80.0f) { // sanity check for temp range
    if (temp < minTemp) minTemp = temp;
    if (temp > maxTemp) maxTemp = temp;
  }
}


void setup() {
  Serial.begin(115200);
  dht.begin();
  
  Wire.begin(21, 22);        // I2C for Arduino 
  I2CBMP.begin(18, 19);       //i2c for bmp
  


  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  if (!bmp.begin(BMP085_STANDARD,&I2CBMP)){
    Serial.println("BMP180 sensor not found!");
  } else {
    Serial.println("BMP sensor initialized.");
  }

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
if (!Firebase.beginStream(streamData, "/")) {
  Serial.println("[STREAM] Could not start stream");
  Serial.println("REASON: " + streamData.errorReason());
} else {
  Serial.println("[STREAM] Stream started...");
  Firebase.setStreamCallback(streamData, streamCallback, streamTimeoutCallback);
}

  timeClient.begin();
}



void loop() {
  timeClient.update();  // Update time from NTP server
  // update_day();
  int today = timeClient.getDay();
if (today != lastSentDay) {
  raintoday = 0;  // Reset only once per new day
  lastSentDay = today;
  Serial.println("New day started → raintoday reset to 0.");
}


  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();

  float temp = dht.readTemperature();

  updateMinMaxTemp(temp);

  float hum = dht.readHumidity();


  float pressure = bmp.readPressure()/1000;
  



  Firebase.setInt(firebaseData, "/did_it_rain today?", raintoday);
 // updateDailyExtremes(temp);

  Firebase.setFloat(firebaseData, "/air_temperature", temp);
  Firebase.setFloat(firebaseData, "/air_humidity", hum);
  Firebase.setFloat(firebaseData, "/air_pressure", pressure);
  Firebase.setFloat(firebaseData, "/daily_min_temp", minTemp);
  Firebase.setFloat(firebaseData, "/daily_max_temp", maxTemp);

  Serial.println("Temp: " + String(temp));
  Serial.println("Hum: " + String(hum));
  Serial.println("Pressure: " + String(pressure));
  Serial.println("Min temp: " + String(minTemp));
  Serial.println("Max temp: " + String(maxTemp));
  Serial.println("raintoday: " + String(raintoday));
  

  // Only record at 9 AM once
if (hours == _9am && minutes == 0 && sent9amFlag == 0) {
  temp9am = temp;
  hum9am = hum;
  pressure9am = pressure;
  Serial.println("9 AM values recorded.");
  sent9amFlag = 1;
}

// Only send at 3 PM once
if (hours == _3pm && minutes == 0 && sent3pmFlag == 0) {
  temp3pm = temp;
  hum3pm = hum;
  pressure3pm = pressure;
  Serial.println("3 PM values recorded and sending prediction...");

  Serial.printf("minTemp: %.2f, maxTemp: %.2f\n", minTemp, maxTemp);
  Serial.printf("hum9am: %.2f, hum3pm: %.2f\n", hum9am, hum3pm);
  Serial.printf("pressure9am: %.2f, pressure3pm: %.2f\n", pressure9am, pressure3pm);
  Serial.printf("temp9am: %.2f, temp3pm: %.2f\n", temp9am, temp3pm);
  Serial.printf("raintoday: %d\n", raintoday);

  prediction = sendPostRequest(
    minTemp, maxTemp,
    hum9am, hum3pm,
    pressure9am, pressure3pm,
    temp9am, temp3pm,
    raintoday
  );
  Firebase.setInt(firebaseData, "/prediction", prediction);

  sent3pmFlag = 1;
}

// Reset flags if minute is not 0
if (minutes != 0) {
  sent9amFlag = 0;
  sent3pmFlag = 0;
}
  sendAndReceiveSensorData(prediction);

  delay(5000);  // 5 sec delay
}

// ========== Helper Functions ========== //

// void updateDailyExtremes(float temp) {
//   int today = timeClient.getDay();
//   if (today != lastSentDay) {
//     if (!dailyTemps.empty()) {
//       minTemp = *std::min_element(dailyTemps.begin(), dailyTemps.end());
//       maxTemp = *std::max_element(dailyTemps.begin(), dailyTemps.end());
//     }
//     dailyTemps.clear();
//     lastSentDay = today;
//   }

//   if (temp >= -40.0f && temp <= 80.0f) {
//     dailyTemps.push_back(temp);
//   }
// }


float sendPostRequest(float mintemp,float maxtemp,float hum9am,float hum3pm,float pressure9am,float pressure3pm,float temp9am,float temp3pm,int raintoday) {
  int prediction = 0;

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiUrl);
    http.setTimeout(5000);  // 5 seconds timeout
    http.addHeader("Content-type", "application/json");

    StaticJsonDocument<200> doc;
    doc["mintemp"] = mintemp;
    doc["maxtemp"] = maxtemp;
    doc["hum9am"] = hum9am;
    doc["hum3pm"] = hum3pm;
    doc["pressure9am"] = pressure9am;
    doc["pressure3pm"] = pressure3pm;
    doc["temp9am"] = temp9am;
    doc["temp3pm"] = temp3pm;
    doc["raintoday"] = raintoday;

    String jsondata;
    serializeJson(doc, jsondata);
    int httpResponseCode = http.POST(jsondata);

    if (httpResponseCode > 0) {
      String response = http.getString();
      StaticJsonDocument<512> responseDoc;
      DeserializationError error = deserializeJson(responseDoc, response);

      if (!error && responseDoc.containsKey("prediction")) {
        prediction = static_cast<int>(responseDoc["prediction"]);
        Serial.print("Prediction: ");
        Serial.println(prediction);
      } else {
        Serial.println("Error: Prediction not found or JSON error.");
      }

    } else {
      Serial.print("HTTP POST failed: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi not connected for POST request.");
  }

  return prediction;
}




void sendAndReceiveSensorData(int prediction) {
  // Send Prediction to Arduino
  Wire.beginTransmission(I2C_ARDUINO_ADDR);
 Wire.write(prediction >> 8); 
 Wire.write(prediction & 0xFF);
 Wire.write(ceilingEnableFlag); // Send control for ceiling
 Wire.write(pumpEnableFlag);    // Send control for pump

  Wire.endTransmission();
  delay(10);

  // Request data from Arduino
  Wire.requestFrom(I2C_ARDUINO_ADDR, 16); // Request 12 bytes

  if (Wire.available() >= 16) {
    int rain = (Wire.read() << 8) | Wire.read();
    int soil1 = (Wire.read() << 8) | Wire.read();
    int soil2 = (Wire.read() << 8) | Wire.read();
    int soil3 = (Wire.read() << 8) | Wire.read();
    int distance = (Wire.read() << 8) | Wire.read();
    int water_pumpFlag = Wire.read();
    int ceiling_status =  Wire.read();
    int tank_flag = Wire.read();
    

    Serial.println("Received Sensor Data:");
    Serial.printf("Prediction sent: %.2f\n", (float)prediction / 100.0); // Assuming prediction might be scaled
    Serial.printf("Rain Today: %d\n", rain);
    Serial.printf("Soil Moisture: %d, %d, %d\n", soil1, soil2, soil3);
    Serial.printf("Ceiling Status: %d\n", ceiling_status);
    Serial.printf("Distance: %d\n", distance);

    Firebase.setInt(firebaseData, "/rain_today", rain);
    Firebase.setFloat(firebaseData, "/soil_sensor_1", soil1);
    Firebase.setFloat(firebaseData, "/soil_sensor_2", soil2);
    Firebase.setFloat(firebaseData, "/soil_sensor_3", soil3);
    Firebase.setInt(firebaseData, "/distance", distance); // Corrected Firebase path for distance
    Firebase.setInt(firebaseData, "/tank_flag", tank_flag);
    Firebase.setInt(firebaseData, "/Ceiling_Status", tank_flag);
    Firebase.setInt(firebaseData, "/Water_Pump_Status", water_pumpFlag);

    if (raintoday != 1) {
  raintoday = rain;
}

    Serial.printf("Rain Today (stored): %d\n", raintoday);

  } else {
    Serial.println("Not enough I2C data received.");
  }
}

void streamCallback(StreamData data) {
  String path = data.dataPath();
  int value = data.intData();

  if (path == "/enable_ceiling") {
    ceilingEnableFlag = value;
    Serial.print("[STREAM] Ceiling Enable updated: ");
    Serial.println(ceilingEnableFlag);
  } else if (path == "/enable_pump") {
    pumpEnableFlag = value;
    Serial.print("[STREAM] Pump Enable updated: ");
    Serial.println(pumpEnableFlag);
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("[STREAM] Timeout, reconnecting...");
  }
}


