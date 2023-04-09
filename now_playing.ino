// Upload to board: DOIT ESP32 Devkit v1


#include <LiquidCrystal.h>
// Create An LCD Object. Signals: [ RS, EN, D4, D5, D6, D7 ]
LiquidCrystal LCD(13, 12, 14, 27, 26, 25);

#include <WiFi.h>
const char* ssid = "your wifi network";
const char* password = "your wifi password";

#include <HTTPClient.h>
HTTPClient http;
const char* endpoint = "https://api.kexp.org/v2/plays/?format=json&limit=1";

#include <Arduino_JSON.h>

#define SHORT_DELAY 3000
#define LONG_DELAY 30000
unsigned long lastTime = 0;
unsigned long timerDelay = 0;

#define LED_PIN 2
#define FAILURES_BEFORE_PROBLEM 3
uint8_t failureCount = FAILURES_BEFORE_PROBLEM; // Boot in error state, so that first success clears it

unsigned long playId = 0;
String song = "";
String artist = "";

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.println("Initializing LCD");
  LCD.begin(16, 2);
  LCD.clear();
 
  LCD.print("Connecting");
  LCD.setCursor(0, 1);
  LCD.print("to WiFi...");

  Serial.println("Initializing WiFi");
  initWiFi();

  LCD.clear();
  LCD.print("Connecting...");
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    lastTime = millis();
    if (timerDelay != SHORT_DELAY) {
      timerDelay = SHORT_DELAY;
    }
    //Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("No WiFi connection.");
      requestFailed();
    } else {      
      JSONVar responseObj = JSON.parse(httpGETRequest(endpoint));
  
      if (JSON.typeof(responseObj) == "undefined") {
        Serial.println("Parsing input failed!");
        requestFailed();
        return;
      }
    
      JSONVar results = responseObj["results"];
      JSONVar play = results[0];
      // Serial.println(play);

      // Success!  Reset failure count and turn off problem light.
      if (failureCount > 0) {
        failureCount = 0;
        digitalWrite(LED_PIN, LOW);
      }

      String playType = play["play_type"];
      unsigned long latestPlayId = play["id"];
      String latestPlaySong = play["song"];
      String latestPlayArtist = play["artist"];
      

      if (playId == latestPlayId) {
        // song hasn't changed; nothing to do
        return;
      }
      playId = latestPlayId;

      if (playType == "trackplay") {
        // song has changed, so extend delay before querying again
        timerDelay = LONG_DELAY;
        song = latestPlaySong;
        artist = latestPlayArtist;

        Serial.print("Now playing: ");
        Serial.print(playId);
        Serial.print(": ");
        Serial.print(artist);
        Serial.print(" - ");
        Serial.println(song);

        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print(artist);
        LCD.setCursor(0,1);
        LCD.print(song);
      } else if (playType == "airbreak") {
        Serial.print("Airbreak ");
        Serial.print(playId);

        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print("Airbreak");
      } else {
        // Unexpected playType.  Do not update display.
      }
    }
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

String httpGETRequest(const char* serverPath) {
  // Your Domain name with URL path or IP address with path
  http.begin(serverPath);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode == 200) {
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void requestFailed() {
  if (failureCount < FAILURES_BEFORE_PROBLEM) {
    failureCount++;
  }
  if (failureCount >= FAILURES_BEFORE_PROBLEM) {
    digitalWrite(LED_PIN, HIGH);
  }
}