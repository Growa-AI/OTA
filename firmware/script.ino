#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>

#define FIRMWARE_VERSION "1.0.2"
#define EEPROM_SIZE 64

const char* ssid = "";
const char* password = "";
const char* version_url = "https://raw.githubusercontent.com/Growa-AI/OTA/main/firmware/version.txt";
const char* firmware_url = "https://raw.githubusercontent.com/Growa-AI/OTA/main/firmware/firmware.ino.bin";

WiFiClientSecure secureClient;

void clearEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
    EEPROM.end();
}

void checkAndUpdate() {
   if(WiFi.status() != WL_CONNECTED) return;
   
   HTTPClient http;
   http.setReuse(false);
   
   String versionUrlWithTimestamp = String(version_url) + 
                                    "?timestamp=" + String(millis()) + 
                                    "&random=" + String(random(10000));
   
   http.begin(secureClient, versionUrlWithTimestamp);
   
   http.addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
   http.addHeader("Pragma", "no-cache");
   http.addHeader("Expires", "0");
   
   int httpCode = http.GET();
   
   if(httpCode == HTTP_CODE_OK) {
       String newVersion = http.getString();
       newVersion.trim();
       
       if(newVersion.length() > 0 && String(FIRMWARE_VERSION) != newVersion) {
           Serial.println("Update available!");
           http.end();
           
           http.begin(secureClient, firmware_url);
           httpCode = http.GET();
           
           if(httpCode == HTTP_CODE_OK) {
               int contentLength = http.getSize();
               Serial.println("Downloading " + String(contentLength) + " bytes...");
               
               // Force complete erase and prepare for update
               Update.setMD5(String(random(1000000)).c_str());
               if(Update.begin(contentLength, U_FLASH)) {
                   size_t written = Update.writeStream(http.getStream());
                   if(written == contentLength) {
                       if(Update.end(true)) {
                           clearEEPROM();  // Pulizia memoria
                           Serial.println("OTA Update Successful!");
                           delay(1000);
                           ESP.restart();
                       } else {
                           Serial.println("Update Finalization Error");
                       }
                   } else {
                       Serial.println("Incomplete Write");
                   }
               } else {
                   Serial.println("Update Initialization Failed");
               }
           } else {
               Serial.println("Firmware Download Failed");
           }
       }
   }
   http.end();
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
    if (currentMillis - lastCheck >= 1800000) {
        lastCheck = currentMillis;
        checkAndUpdate();
    }
}
