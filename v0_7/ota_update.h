#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include "config.h"

// ============================================================================
// SINCRONIZZAZIONE CODICE
// ============================================================================

bool isNewerVersion(String currentVer, String newVer) {
  currentVer.replace("v", "");
  newVer.replace("v", "");

  int currMajor = 0, currMinor = 0;
  int newMajor = 0, newMinor = 0;

  int dot = currentVer.indexOf('.');
  if (dot > 0) {
    currMajor = currentVer.substring(0, dot).toInt();
    currMinor = currentVer.substring(dot + 1).toInt();
  }
  dot = newVer.indexOf('.');
  if (dot > 0) {
    newMajor = newVer.substring(0, dot).toInt();
    newMinor = newVer.substring(dot + 1).toInt();
  }

  if (newMajor > currMajor) return true;
  if (newMajor == currMajor && newMinor > currMinor) return true;
  return false;
}

void checkForUpdate() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(VERSION_CHECK_URL);
  http.setTimeout(10000);
  int httpCode = http.GET();

  if (httpCode == 200) {
    JsonDocument doc;
    deserializeJson(doc, http.getString());
    availableVersion = doc["version"].as<String>();
    downloadURL = doc["download_url"].as<String>();
    updateAvailable = isNewerVersion(String(FIRMWARE_VERSION), availableVersion);

    if (updateAvailable) {
      Serial.println("Aggiornamento disponibile: " + availableVersion);
    }
  }
  http.end();
  lastVersionCheck = millis();
}

void performOTAUpdate() {
  if (!updateAvailable || downloadURL == "") return;

  Serial.println("Avvio aggiornamento...");

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(downloadURL);
  http.setTimeout(30000);
  int httpCode = http.GET();

  if (httpCode == 200) {
    int contentLength = http.getSize();
    if (Update.begin(contentLength)) {
      WiFiClient *client = http.getStreamPtr();
      size_t written = 0;
      uint8_t buff[256];

      while (http.connected() && written < contentLength) {
        size_t available = client->available();
        if (available) {
          int bytesRead = client->readBytes(buff, min(available, sizeof(buff)));
          written += Update.write(buff, bytesRead);
          Serial.println("Progresso: " + String((written * 100) / contentLength) + "%");
        }
        delay(1);
      }

      if (Update.end(true)) {
        Serial.println("Aggiornamento completato! Riavvio...");
        delay(1000);
        ESP.restart();
      } else {
        Serial.println("Errore installazione: " + String(Update.errorString()));
      }
    }
  } else {
    Serial.println("Errore download: " + String(httpCode));
  }
  http.end();
}

#endif // OTA_UPDATE_H
