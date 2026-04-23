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

#include <mbedtls/md.h>

// Variabile per il hash atteso (popolata da checkForUpdate)
extern String expectedSha256;

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
    expectedSha256 = doc["sha256"].as<String>();  // campo hash atteso
    updateAvailable = isNewerVersion(String(FIRMWARE_VERSION), availableVersion);

    if (updateAvailable) {
      Serial.println("Aggiornamento disponibile: " + availableVersion);
      if (expectedSha256.length() < 64) {
        Serial.println("[OTA] ⚠️ SHA256 mancante nel version.json — aggiornamento bloccato per sicurezza.");
        updateAvailable = false;
      }
    }
  }
  http.end();
  lastVersionCheck = millis();
}

void performOTAUpdate() {
  if (!updateAvailable || downloadURL == "") return;
  if (expectedSha256.length() < 64) {
    Serial.println("[OTA] SHA256 non disponibile — aggiornamento annullato.");
    return;
  }

  Serial.println("Avvio aggiornamento...");

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(downloadURL);
  http.setTimeout(30000); // timeout download globale 30s
  int httpCode = http.GET();

  if (httpCode == 200) {
    int contentLength = http.getSize();
    if (Update.begin(contentLength)) {
      WiFiClient *client = http.getStreamPtr();
      size_t written = 0;
      uint8_t buff[256];

      // Contesto SHA256
      mbedtls_md_context_t mdCtx;
      mbedtls_md_init(&mdCtx);
      mbedtls_md_setup(&mdCtx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
      mbedtls_md_starts(&mdCtx);

      unsigned long dlStart = millis();
      while (http.connected() && written < (size_t)contentLength) {
        if (millis() - dlStart > 60000) { // timeout globale 60s
          Serial.println("[OTA] Timeout download — annullo.");
          Update.abort();
          mbedtls_md_free(&mdCtx);
          http.end();
          return;
        }
        size_t available = client->available();
        if (available) {
          int bytesRead = client->readBytes(buff, min(available, sizeof(buff)));
          mbedtls_md_update(&mdCtx, buff, bytesRead); // aggiorna hash
          written += Update.write(buff, bytesRead);
          Serial.println("Progresso: " + String((written * 100) / contentLength) + "%");
        }
        delay(1);
      }

      // Calcola hash finale
      uint8_t hash[32];
      mbedtls_md_finish(&mdCtx, hash);
      mbedtls_md_free(&mdCtx);

      // Converti in stringa esadecimale
      char hashHex[65];
      for (int i = 0; i < 32; i++) snprintf(hashHex + i * 2, 3, "%02x", hash[i]);
      hashHex[64] = '\0';

      Serial.println("[OTA] SHA256 calcolato:  " + String(hashHex));
      Serial.println("[OTA] SHA256 atteso:     " + expectedSha256);

      if (String(hashHex) != expectedSha256) {
        Serial.println("[OTA] ❌ SHA256 non corrisponde — firmware RIFIUTATO.");
        Update.abort();
        http.end();
        return;
      }

      if (Update.end(true)) {
        Serial.println("✅ Aggiornamento completato! Riavvio...");
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
