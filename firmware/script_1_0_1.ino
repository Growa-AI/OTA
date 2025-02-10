#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>

#define FIRMWARE_VERSION "1.0.1"
#define EEPROM_SIZE 64

const char* ssid = "";
const char* password = "";
const char* version_url = "https://raw.githubusercontent.com/Growa-AI/OTA/main/firmware/version.txt";
const char* firmware_url = "https://raw.githubusercontent.com/Growa-AI/OTA/main/firmware/firmware.ino.bin";

WiFiClientSecure secureClient;

void checkAndUpdate() {
   Serial.println("--------- Start checkAndUpdate ---------");
   
   // WiFi Connection Check
   if(WiFi.status() != WL_CONNECTED) {
     Serial.println("ERROR: WiFi not connected");
     return;
   }
   Serial.println("WiFi Connected: OK");

   // Secure Client Configuration
   secureClient.setInsecure(); // Disable certificate verification
   Serial.println("Secure Client Configured");

   HTTPClient http;
   http.setReuse(false);
   
   // Dynamic URL with timestamp and random
   String versionUrlWithTimestamp = String(version_url) + 
                                    "?t=" + String(millis()) + 
                                    "&r=" + String(random(10000));
   
   Serial.println("Version Check URL: " + versionUrlWithTimestamp);

   // Attempt to start HTTP connection
   bool beginStatus = http.begin(secureClient, versionUrlWithTimestamp);
   if (!beginStatus) {
     Serial.println("ERROR: HTTP Begin Failed");
     return;
   }
   
   // Add cache-busting headers
   http.addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
   http.addHeader("Pragma", "no-cache");
   http.addHeader("Expires", "0");
   
   // Perform GET request
   int httpCode = http.GET();
   Serial.print("HTTP Response Code: ");
   Serial.println(httpCode);

   if(httpCode == HTTP_CODE_OK) {
       String newVersion = http.getString();
       newVersion.trim();
       
       Serial.println("Received Version: " + newVersion);
       Serial.println("Current Version: " + String(FIRMWARE_VERSION));
       
       if(newVersion.length() > 0 && String(FIRMWARE_VERSION) != newVersion) {
           Serial.println("UPDATE AVAILABLE!");
           http.end();
           
           // Prepare for firmware download
           HTTPClient firmwareHttp;
           firmwareHttp.setReuse(false);
           firmwareHttp.begin(secureClient, firmware_url);
           
           int firmwareHttpCode = firmwareHttp.GET();
           Serial.print("Firmware Download HTTP Code: ");
           Serial.println(firmwareHttpCode);
           
           if(firmwareHttpCode == HTTP_CODE_OK) {
               int contentLength = firmwareHttp.getSize();
               Serial.println("Firmware Size: " + String(contentLength) + " bytes");
               
               // Prepare Update
               if(Update.begin(contentLength)) {
                   size_t written = Update.writeStream(firmwareHttp.getStream());
                   
                   Serial.print("Bytes Written: ");
                   Serial.print(written);
                   Serial.print(" / ");
                   Serial.println(contentLength);
                   
                   if(written == contentLength) {
                       if(Update.end(true)) {
                           Serial.println("OTA UPDATE SUCCESSFUL!");
                           delay(1000);
                           ESP.restart();
                       } else {
                           Serial.print("Update Finalization Error: ");
                           Serial.println(Update.getError());
                       }
                   } else {
                       Serial.println("Incomplete firmware write");
                   }
               } else {
                   Serial.println("Update Begin Failed");
               }
           } else {
               Serial.println("Firmware Download Failed");
           }
           
           firmwareHttp.end();
       } else {
           Serial.println("No update needed");
       }
   } else {
       Serial.println("Version Check Failed");
   }
   
   http.end();
   Serial.println("--------- End checkAndUpdate ---------");
}

void setup() {
    Serial.begin(115200);
    
    // Inizializzazioni aggiuntive
    secureClient.setInsecure();
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connesso");
    
    checkAndUpdate();
}

void loop() {
    static unsigned long lastCheck = 0;
    unsigned long currentMillis = millis();
    
    // Check ogni 30 minuti
    if (currentMillis - lastCheck >= 10000) {
        lastCheck = currentMillis;
        checkAndUpdate();
    }
}
