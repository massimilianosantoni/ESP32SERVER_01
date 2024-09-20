#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <DNSServer.h>

// Replace with your network credentials
const char* ssid = "SPAPPERI-6548373";
char password[] = "12345678";

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
  delay(1000);

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

  // Set up Access Point with a specific IP address
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  if(!WiFi.softAPConfig(local_IP, gateway, subnet)){
    Serial.println("AP Config Failed");
  }
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start DNS server
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Serve static files from SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.serveStatic("/config", SPIFFS, "/config.html");

  // Handle LED on/off
  server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);
    Serial.println("LED turned ON");
    request->send(200, "text/plain", "LED is ON");
  });

  server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);
    Serial.println("LED turned OFF");
    request->send(200, "text/plain", "LED is OFF");
  });

  // Handle configuration changes
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("password", true)) {
      String newPassword = request->getParam("password", true)->value();
      if (newPassword.length() > 0 && newPassword.length() < 32) {
        newPassword.toCharArray(password, 32);
        Serial.println("Password updated via /config");
        request->send(200, "text/plain", "Password updated. Please reconnect to the AP.");
        ESP.restart();
      } else {
        Serial.println("Invalid password length attempted");
        request->send(400, "text/plain", "Invalid password length.");
      }
    } else {
      Serial.println("No password provided in /config");
      request->send(400, "text/plain", "No password provided.");
    }
  });

  // Handle Android captive portal check
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Android captive portal check");
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Handle iOS captive portal check
  server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("iOS captive portal check");
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Handle any other routes by serving index.html
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.print("Unhandled request: ");
    Serial.println(request->url());
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Start server
  server.begin();
  Serial.println("Web Server Started");
}

void loop() {
  // Handle DNS requests
  dnsServer.processNextRequest();
}
