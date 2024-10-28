// #include "FS.h"
// #include "SD.h"
// #include "SPI.h"
#include <WiFi.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <RTClib.h>

#include <math.h>

#define LIGHT_VOLTAGE_DIVIDER_VOUT_PIN 32
#define TEMP_VOLTAGE_DIVIDER_VOUT_PIN 33
#define HUMIDITY_VOLTAGE_DIVIDER_VOUT_PIN 34
#define VOLTAGE_DIVIDER_VIN 5.0
#define KNOWN_RESISTANCE 100000
#define ALLOWED_CONNECTION_ATTEMPTS 1

#define ANALOG_TO_VOLT 0.0008695652173913044

// #define SD_CS 5
// #define SD_SCK 18
// #define SD_MOSI 23
// #define SD_MISO 19

// #define CLOCK_SDA 21
// #define CLOCK_SCL 22


#define AOUT_PIN 

#define CYCLES_INTER_WIFI_CONNECTION_ATTEMPT 8
#define CYCLES_INTER_CLOCK_CONNECTION_ATTEMPT 4

#define LOOP_CYCLE_LENGTH 5000
#define WIFI_WAIT_FOR_CONNECTION 7000
#define BAUD_RATE 9600

bool setupWiFi();
int getServerTime();
void setupServerComunication();
String DateTime_to_String(DateTime t);

float get_voltage_divider_unknown_resistance(int voutPin, float v_in, int known_resistance);
float get_light(int voutPin, float vin, int known_resistance);
float get_temperature(int voutPin, float vin, int known_resistance);

// File collectedDataFile;
// char * data_dump_file_name = "sensorData.txt";
// int initializedSD;

WiFiClient client;
char * wifiSSID = "AltachiaraCellulare";
char * wifiPasswd = "polloooo";

uint8_t wifiCheckCycles;

RTC_DS1307 systemClock;
char * timeApiUri = "https://currentmillis.com/time/seconds-since-unix-epoch.php";
int currentUNIXTimestamp; 
uint8_t clockCheckCycles;

float v_out, light, temperature;

void setup() {
  delay(2000);

  Serial.begin(BAUD_RATE);
  pinMode(LIGHT_VOLTAGE_DIVIDER_VOUT_PIN, INPUT);
  pinMode(TEMP_VOLTAGE_DIVIDER_VOUT_PIN, INPUT);
  pinMode(HUMIDITY_VOLTAGE_DIVIDER_VOUT_PIN, INPUT);


  setupWiFi();
  systemClock.begin();
  delay(1000);
  if(systemClock.isrunning()) {
    Serial.println("Clock started succesfully");
  } else {
    Serial.println("Error in starting the clock");
  }
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to router");
  } 
  if(!systemClock.isrunning()){
    Serial.println("Absent external clock");
  }
  if((WiFi.status() == WL_CONNECTED) && systemClock.isrunning()) {
    Serial.println("Getting timestamp from api server");
    currentUNIXTimestamp = getServerTime(); 
    Serial.print("Current UNIX timestamp from server: ");
    Serial.println(currentUNIXTimestamp);
    Serial.println(DateTime_to_String(DateTime(currentUNIXTimestamp)));
    if(currentUNIXTimestamp != 0) {
      Serial.print("Adjusting clock time to ");
      Serial.println(currentUNIXTimestamp);
      DateTime dt = DateTime(currentUNIXTimestamp);
      systemClock.adjust(dt);
      delay(1000);
    }
    Serial.print("Clock setup to ");
    Serial.printf("%u\n", systemClock.now().unixtime());
    Serial.println("Setup comunication channel demetra->ade");
    setupServerComunication();
  }

  wifiCheckCycles = 0;
  clockCheckCycles = 0;
}

void loop() {

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to network");
  } else {
    Serial.printf("Not connected to network: retrying to connect in %us\n", (CYCLES_INTER_WIFI_CONNECTION_ATTEMPT - wifiCheckCycles) * (LOOP_CYCLE_LENGTH / 1000));
  }

  if(systemClock.isrunning()) {
    Serial.println("External clock up and running");
  } else {
    Serial.printf("External clock down: retrying to connect in %us\n", (CYCLES_INTER_CLOCK_CONNECTION_ATTEMPT - clockCheckCycles) * (LOOP_CYCLE_LENGTH / 1000));
  }

  if((WiFi.status() != WL_CONNECTED) && (wifiCheckCycles % CYCLES_INTER_WIFI_CONNECTION_ATTEMPT == 0)) {
    wifiCheckCycles = 0;
    Serial.println("Not connected to wifi, trying to connect");
    setupWiFi();
  }

  if(WiFi.status() == WL_CONNECTED && !client.connected()) {
    setupServerComunication();
  }

  if(!systemClock.isrunning() && (clockCheckCycles % CYCLES_INTER_CLOCK_CONNECTION_ATTEMPT == 0)) {
    clockCheckCycles = 0;
    Serial.println("System clock not running: trying to connect");
    systemClock.begin();
  } else {
    Serial.println("Date and time: " + DateTime_to_String(systemClock.now()));
  }

  // ---------------------------
  // if(CLIENT & !SERVER)
  // ---------------------------
  
  if(wifiCheckCycles == 255) {
    wifiCheckCycles = 0;
  }
  if(clockCheckCycles == 255) {
    clockCheckCycles = 0;
  }
  
  wifiCheckCycles++;
  clockCheckCycles++;
  
  Serial.print("time: ");
  Serial.println(DateTime_to_String(systemClock.now()));
  Serial.print("light: ");
  Serial.println(get_light(LIGHT_VOLTAGE_DIVIDER_VOUT_PIN, VOLTAGE_DIVIDER_VIN, KNOWN_RESISTANCE));
  Serial.print("temperature: ");
  Serial.println(get_temperature(TEMP_VOLTAGE_DIVIDER_VOUT_PIN, VOLTAGE_DIVIDER_VIN, KNOWN_RESISTANCE));
  Serial.print("humidity: ");
  Serial.println(get_humidity(HUMIDITY_VOLTAGE_DIVIDER_VOUT_PIN));  

  if(client.connected()) {
    client.print("light: ");
    client.println(get_light(LIGHT_VOLTAGE_DIVIDER_VOUT_PIN, VOLTAGE_DIVIDER_VIN, KNOWN_RESISTANCE));
    client.print("temperature: ");
    client.println(get_temperature(TEMP_VOLTAGE_DIVIDER_VOUT_PIN, VOLTAGE_DIVIDER_VIN, KNOWN_RESISTANCE));
    client.print("humidity: ");
    client.println(get_humidity(HUMIDITY_VOLTAGE_DIVIDER_VOUT_PIN));  
  }
  delay(LOOP_CYCLE_LENGTH);
}

bool setupWiFi() {

  WiFi.mode(WIFI_STA);
  for(uint8_t i = 0; i < ALLOWED_CONNECTION_ATTEMPTS && !(WiFi.status() == WL_CONNECTED); i++) {
    Serial.println(String("Attempting to connect to WPA SSID: ") + String(wifiSSID));

    WiFi.begin(wifiSSID, wifiPasswd);
    delay(WIFI_WAIT_FOR_CONNECTION);   
  
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("Attempt to connect to network failed");
    } else if(WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected succesfully to network");
      Serial.print("Local IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("Net SSID: ");
      Serial.println(WiFi.SSID());
      Serial.print("Signal strength: ");
      Serial.print(-1 * WiFi.RSSI());
      Serial.print("dBm or ");
      Serial.printf("%e", pow(10, WiFi.RSSI() / 10.0)); 
      Serial.println("mW");
    }
  }
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Allowed connection attempts ended");
  }
  return WiFi.status() == WL_CONNECTED;
}

int getServerTime() {
  HTTPClient dateClient;
  if(dateClient.begin(timeApiUri)) {
    Serial.println("Connected succesfully to time api server");
  } else {
    Serial.println("Failed connection to time api server"); 
    return 0;
  }

  int HTTPResponseCode = dateClient.GET();
  if(HTTPResponseCode <= 0) {
    return 0;
  }
  
  int result = dateClient.getString().toInt();
  dateClient.end();
  return result;
}

void setupServerComunication() {

    Serial.println("Attempting to connect to server: ");
    int port = 4020;
    char *serverIP = "192.168.22.205";

    for(int i = 0; i < 5; i++) {
      if (client.connect(serverIP, port)) {
        Serial.println("Connected to Ade started succesfully");
        client.println("Demetra just woke up");    
        break;
      } else {
        Serial.println("Connection to Ade failed miserably");
      }
    }
}

String DateTime_to_String(DateTime t) {
    return "" 
    + String(t.year()) 
    + "-"
    + String(t.month()) 
    + "-" 
    + String(t.day()) 
    + " " 
    + String((t.hour() + 2) % 24)
    + ":"
    + String(t.minute())
    + ":"
    + String(t.second());
}

float get_voltage_divider_unknown_resistance(int voutPin, float v_in, int known_resistance) {
    float v_out = analogRead(voutPin) * ANALOG_TO_VOLT;
    float unknown_resistance = (v_out * known_resistance) / (v_in - v_out);   
    return unknown_resistance;
}

float get_light(int voutPin, float vin, int known_resistance) {
  return get_voltage_divider_unknown_resistance(voutPin, vin, known_resistance);
}

float get_temperature(int voutPin, float vin, int known_resistance) {
  float r = get_voltage_divider_unknown_resistance(voutPin, vin, known_resistance);
  return r;
}

float get_humidity(int voutPin) {
  return analogRead(voutPin) * ANALOG_TO_VOLT;
}
