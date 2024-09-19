#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <DNSServer.h>

// Replace with your network credentials
const char* ssid = "ESP32-AccessPoint";
const char* password = "12345678";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
DNSServer dnsServer;

// Define LED pin
const int ledPin = 2;

// DNS server settings
const byte DNS_PORT = 53;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("Serial Monitor started");

  // Set LED pin as output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Serial.println("LED pin initialized");

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // Set up Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Setup DNS server to redirect all requests to the ESP32 except cancontrol.com
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("DNS server started");

  // Serve static files from SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  Serial.println("Serving static files from SPIFFS");

  // Handle LED on/off
  server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);
    request->send(200, "text/plain", "LED is ON");
    Serial.println("LED turned ON");
  });

  server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);
    request->send(200, "text/plain", "LED is OFF");
    Serial.println("LED turned OFF");
  });

  // Respond to connectivity check URLs
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(204, "text/plain", "");
    Serial.println("Responded to /generate_204");
  });

  server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "No internet");
    Serial.println("Responded to /fwlink");
  });

  // Redirect all other requests to the index.html page
  server.onNotFound([](AsyncWebServerRequest *request){
    if (request->host() == "cancontrol.com") {
      request->send(SPIFFS, "/index.html", "text/html");
      Serial.println("Serving index.html for cancontrol.com");
    } else {
      request->send(404, "text/plain", "Not Found");
      Serial.println("Responding with 404 Not Found");
    }
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle DNS requests
  dnsServer.processNextRequest();
}