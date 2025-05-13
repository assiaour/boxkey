#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server details
const char* serverUrl = "https://box-key.onrender.com/api/verify";
const int serverPort = 80;

// Pin definitions
const int lockPin = D1; // Pin connected to the lock mechanism
const int ledPin = D4;  // Built-in LED for status indication

// Password buffer
String enteredPassword = "";
const int maxPasswordLength = 6; // Adjust based on your password length

void setup() {
  Serial.begin(115200);
  delay(10);

  // Initialize pins
  pinMode(lockPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(lockPin, LOW); // Ensure lock is initially closed
  digitalWrite(ledPin, HIGH); // Turn off LED (inverted logic)

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, !digitalRead(ledPin)); // Blink LED while connecting
  }
  
  Serial.println("\nConnected to WiFi");
  digitalWrite(ledPin, HIGH); // Turn off LED
}

void loop() {
  // Check for serial input (simulating keypad input)
  if (Serial.available() > 0) {
    char key = Serial.read();
    
    if (key == '\n' || key == '\r') {
      // Enter key pressed, verify password
      verifyPassword();
      enteredPassword = ""; // Clear password buffer
    } else if (enteredPassword.length() < maxPasswordLength) {
      // Add character to password buffer
      enteredPassword += key;
      Serial.print("*"); // Echo asterisk for security
    }
  }
}

void verifyPassword() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  Serial.println("\nVerifying password...");
  
  // Create HTTP client
  HTTPClient http;
  WiFiClient client;
  
  // Prepare JSON payload
  StaticJsonDocument<200> doc;
  doc["password"] = enteredPassword;
  String jsonString;
  serializeJson(doc, jsonString);

  // Make POST request
  http.begin(client, serverUrl);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.POST(jsonString);
  
  if (httpCode > 0) {
    String payload = http.getString();
    StaticJsonDocument<200> responseDoc;
    deserializeJson(responseDoc, payload);
    
    bool isValid = responseDoc["valid"];
    String message = responseDoc["message"];
    
    Serial.println(message);
    
    if (isValid) {
      // Valid password - open lock
      digitalWrite(lockPin, HIGH);
      digitalWrite(ledPin, LOW); // Turn on LED
      Serial.println("Lock opened");
      delay(5000); // Keep lock open for 5 seconds
      digitalWrite(lockPin, LOW);
      digitalWrite(ledPin, HIGH); // Turn off LED
      Serial.println("Lock closed");
    } else {
      // Invalid password - blink LED
      for (int i = 0; i < 3; i++) {
        digitalWrite(ledPin, LOW);
        delay(200);
        digitalWrite(ledPin, HIGH);
        delay(200);
      }
    }
  } else {
    Serial.println("Error on HTTP request");
  }
  
  http.end();
} 