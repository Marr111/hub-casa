#ifndef NETWORK_TIME_H
#define NETWORK_TIME_H

// ============================================================================
// NETWORK TIME - Umbrella include
// Questo file include tutti i moduli separati.
// Non aggiungere codice qui: modificare i file specifici.
// ============================================================================

#include "net_wifi_time.h"   // WiFi, NTP, getTime(), getDateLong()
#include "ical_parser.h"     // iCal fetch, parse, processLine(), printEvents()
#include "calendar_month.h"  // Griglia mensile, dettaglio giorno
#include "calendar_week.h"   // Vista settimanale, printTasksTFT()
#include "ota_update.h"      // OTA firmware update
#include "weather.h"         // Meteo Open-Meteo, icone TFT, sensore DS18B20

#endif // NETWORK_TIME_H
