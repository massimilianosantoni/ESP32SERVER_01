#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "ESP32-Access-Point";
char password[32] = "12345678";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
DNSServer dnsServer;

// Define LED pin and reset pin
const int ledPin = 2;
const int resetPin = 0; // Change to the appropriate pin for your board

// DNS server settings
const byte DNS_PORT = 53;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Set LED pin as output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Set reset pin as input
  pinMode(resetPin, INPUT_PULLUP);

  // Check for factory reset
  if (digitalRead(resetPin) == LOW) {
    strcpy(password, "12345678");
    Serial.println("Factory reset: Password reset to 12345678");
  }

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Set up Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Setup DNS server to redirect all requests to the ESP32
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Serve static files from SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.serveStatic("/config", SPIFFS, "/config.html");

  // Handle LED on/off
  server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);
    request->send(200, "text/plain", "LED is ON");
  });

  server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);
    request->send(200, "text/plain", "LED is OFF");
  });

  // Handle configuration changes
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("body", true)) {
      String body = request->getParam("body", true)->value();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, body);
      const char* newPassword = doc["password"];
      if (strlen(newPassword) > 0 && strlen(newPassword) < 32) {
        strcpy(password, newPassword);
        request->send(200, "text/plain", "Password updated. Please reconnect to the AP.");
        ESP.restart();
      } else {
        request->send(400, "text/plain", "Invalid password length.");
      }
    } else {
      request->send(400, "text/plain", "No password provided.");
    }
  });

  // Start server
  server.begin();
}

void loop() {
  // Handle DNS requests
  dnsServer.processNextRequest();
}