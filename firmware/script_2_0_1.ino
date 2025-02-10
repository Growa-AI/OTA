#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>

#define FIRMWARE_VERSION "1.0.5"
#define MAX_UPDATE_ATTEMPTS 3

const char* ssid = "";
const char* password = "";
const char* version_url = "https://raw.githubusercontent.com/Growa-AI/OTA/main/firmware/version.txt";
const char* firmware_url = "https://raw.githubusercontent.com/Growa-AI/OTA/main/firmware/firmware.ino.bin";

WiFiClientSecure secureClient;

// Struttura per lo stato dell'aggiornamento
struct UpdateStatus {
  bool updateInProgress = false;
  int failCount = 0;
  unsigned long lastAttemptTime = 0;
};

UpdateStatus updateStatus;

void logDebug(const String& message) {
  Serial.println("[DEBUG] " + message);
}

void checkAndUpdate() {
  logDebug("Starting OTA Update Process");
  
  // Controllo connessione WiFi
  if(WiFi.status() != WL_CONNECTED) {
    logDebug("WiFi not connected");
    return;
  }
  
  // Limitazione tentativi
  if(updateStatus.failCount >= MAX_UPDATE_ATTEMPTS) {
    logDebug("Max update attempts reached");
    return;
  }
  
  // Configurazione client sicuro
  secureClient.setInsecure();
  
  HTTPClient http;
  http.setReuse(false);
  
  // URL con timestamp per evitare cache
  String versionUrlWithTimestamp = String(version_url) + 
                                   "?t=" + String(millis()) + 
                                   "&r=" + String(random(10000));
  
  logDebug("Version Check URL: " + versionUrlWithTimestamp);
  
  // Connessione per check versione
  if(!http.begin(secureClient, versionUrlWithTimestamp)) {
    logDebug("HTTP Connection Failed");
    updateStatus.failCount++;
    return;
  }
  
  // Header anti-cache
  http.addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  http.addHeader("Pragma", "no-cache");
  http.addHeader("Expires", "0");
  
  // Richiesta versione
  int httpCode = http.GET();
  if(httpCode != HTTP_CODE_OK) {
    logDebug("Version Check Failed: " + String(httpCode));
    http.end();
    updateStatus.failCount++;
    return;
  }
  
  // Elaborazione versione
  String newVersion = http.getString();
  newVersion.trim();
  
  logDebug("Current Version: " + String(FIRMWARE_VERSION));
  logDebug("Available Version: " + newVersion);
  
  // Verifica aggiornamento
  if(newVersion.length() == 0 || String(FIRMWARE_VERSION) == newVersion) {
    logDebug("No update needed");
    updateStatus.failCount = 0;
    http.end();
    return;
  }
  
  // Download firmware
  http.end();
  HTTPClient firmwareHttp;
  firmwareHttp.setReuse(false);
  
  if(!firmwareHttp.begin(secureClient, firmware_url)) {
    logDebug("Firmware Download Connection Failed");
    updateStatus.failCount++;
    return;
  }
  
  int firmwareHttpCode = firmwareHttp.GET();
  if(firmwareHttpCode != HTTP_CODE_OK) {
    logDebug("Firmware Download Failed: " + String(firmwareHttpCode));
    firmwareHttp.end();
    updateStatus.failCount++;
    return;
  }
  
  // Preparazione aggiornamento
  int contentLength = firmwareHttp.getSize();
  logDebug("Firmware Size: " + String(contentLength) + " bytes");
  
  if(!Update.begin(contentLength)) {
    logDebug("Update Initialization Failed");
    firmwareHttp.end();
    updateStatus.failCount++;
    return;
  }
  
  // Scrittura firmware
  size_t written = Update.writeStream(firmwareHttp.getStream());
  if(written != contentLength) {
    logDebug("Incomplete Firmware Write: " + 
             String(written) + "/" + String(contentLength));
    Update.abort();
    firmwareHttp.end();
    updateStatus.failCount++;
    return;
  }
  
  // Finalizzazione aggiornamento
  if(!Update.end(true)) {
    logDebug("Update Finalization Error: " + String(Update.getError()));
    firmwareHttp.end();
    updateStatus.failCount++;
    return;
  }
  
  // Aggiornamento riuscito
  logDebug("OTA UPDATE SUCCESSFUL!");
  updateStatus.failCount = 0;
  firmwareHttp.end();
  
  // Riavvio
  delay(1000);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  logDebug("Firmware Version: " + String(FIRMWARE_VERSION));
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  logDebug("WiFi Connected");
  
  // Primo controllo immediato
  checkAndUpdate();
}

void loop() {
  static unsigned long lastCheck = 0;
  unsigned long currentMillis = millis();
  
  // Controllo ogni 30 minuti
  if (currentMillis - lastCheck >= 1800000) {
    lastCheck = currentMillis;
    checkAndUpdate();
  }
  
  delay(1000);
}
