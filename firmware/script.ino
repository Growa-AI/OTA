#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>

#define FIRMWARE_VERSION "1.0.1"

const char* ssid = "";
const char* password = "";
const char* version_url = "https://raw.githubusercontent.com/Growa-AI/OTA/main/firmware/version.txt";
const char* firmware_url = "https://raw.githubusercontent.com/Growa-AI/OTA/main/firmware/firmware.ino.bin";

WiFiClientSecure secureClient;

void checkAndUpdate() {
   if(WiFi.status() != WL_CONNECTED) return;
   
   HTTPClient http;
   http.setReuse(false);
   
   // Aggressive cache-busting
   String versionUrlWithTimestamp = String(version_url) + 
                                    "?timestamp=" + String(millis()) + 
                                    "&random=" + String(random(10000));
   
   // Use secure client
   http.begin(secureClient, versionUrlWithTimestamp);
   
   // Multiple cache control headers
   http.addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
   http.addHeader("Pragma", "no-cache");
   http.addHeader("Expires", "0");
   
   int httpCode = http.GET();
   
   if(httpCode == HTTP_CODE_OK) {
       String newVersion = http.getString();
       newVersion.trim();
       
       // Validate version
       if(newVersion.length() > 0) {
           Serial.println("Current version: " + String(FIRMWARE_VERSION));
           Serial.println("Available version: " + newVersion);
           
           if(String(FIRMWARE_VERSION) != newVersion) {
               Serial.println("Update available!");
               http.end();
               
               // Download firmware
               http.begin(secureClient, firmware_url);
               httpCode = http.GET();
               
               if(httpCode == HTTP_CODE_OK) {
                   int contentLength = http.getSize();
                   Serial.println("Downloading " + String(contentLength) + " bytes...");
                   
                   if(Update.begin(contentLength)) {
                       size_t written = Update.writeStream(http.getStream());
                       if(written == contentLength) {
                           Serial.println("Written : " + String(written) + " successfully");
                           if(Update.end(true)) {
                               Serial.println("OTA done!");
                               ESP.restart();
                           } else {
                               Serial.println("Update end error: " + String(Update.getError()));
                           }
                       } else {
                           Serial.println("Write failed: " + String(written) + "/" + String(contentLength));
                       }
                   } else {
                       Serial.println("Not enough space to begin OTA");
                   }
               } else {
                   Serial.println("Firmware download failed");
               }
           } else {
               Serial.println("Already on latest version");
           }
       } else {
           Serial.println("Invalid version received");
       }
   } else {
       Serial.println("Version check failed");
   }
   http.end();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Current firmware version: " + String(FIRMWARE_VERSION));
    
    // Allow insecure connection (use cautiously)
    secureClient.setInsecure();
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    
    delay(1000);  // Delay before first check
    checkAndUpdate();
}

void loop() {
    delay(20000);  // Wait 20 seconds between checks
    if(WiFi.status() == WL_CONNECTED) {
        checkAndUpdate();
        delay(1000);  // Additional small delay after check
    }
}
