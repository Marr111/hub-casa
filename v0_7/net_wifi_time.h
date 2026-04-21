#ifndef NET_WIFI_TIME_H
#define NET_WIFI_TIME_H

#include "config.h"

// ============================================================================
// GESTIONE RETE E TEMPO
// ============================================================================

void connessioneWiFi() {
  int tentativi = 0;
  const int maxTentativi = 5;

  Serial.print("📶 Connessione al WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED && tentativi < maxTentativi) {
    delay(1000);
    Serial.print(".");
    tentativi++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅🛜 WiFi connesso!");
    Serial.print("🌐 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi non connesso, proseguo con il programma...");
  }
}

void connessioneNTP() {
  int tentativi = 0;
  const int maxTentativi = 5;

  configTime(0, 0, NTP_SERVER);
  Serial.println("⏱ Attendo sincronizzazione NTP...");

  while (!getLocalTime(&timeinfo) && tentativi < maxTentativi) {
    delay(500);
    Serial.print("⌛");
    tentativi++;
  }

  if (tentativi < maxTentativi) {
    Serial.println("\n✅🕒 Ora e data ottenute!");
  } else {
    Serial.println("\n❌ NTP non sincronizzato, proseguo con il programma...");
  }
}

String getDateLong() {
  if (!getLocalTime(&timeinfo)) return "N/A";

  const char *daysOfWeek[] = { "domenica", "lunedi", "martedi", "mercoledi",
                               "giovedi", "venerdi", "sabato" };
  const char *months[] = { "gen", "feb", "mar", "apr", "mag", "giu",
                           "lug", "ago", "set", "ott", "nov", "dic" };

  char buffer[40];
  sprintf(buffer, "%s %02d %s %04d",
          daysOfWeek[timeinfo.tm_wday],
          timeinfo.tm_mday,
          months[timeinfo.tm_mon],
          timeinfo.tm_year + 1900);
  return String(buffer);
}

String getTime() {
  if (!getLocalTime(&timeinfo)) return "N/A";

  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d",
          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(buffer);
}

#endif // NET_WIFI_TIME_H
