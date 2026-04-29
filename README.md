# 🏠 Casa Casetta - Smart Home Hub v0.7

Hub domotico basato su **ESP32-S3** con display touchscreen **ST7796** (480×320) per il controllo centralizzato della casa intelligente.

![ESP32-S3](https://img.shields.io/badge/ESP32-S3-blue)
![Arduino](https://img.shields.io/badge/Arduino-IDE-00979D?logo=arduino)
![Version](https://img.shields.io/badge/Versione-0.7-green)
![Status](https://img.shields.io/badge/Status-Active%20Development-yellow)

---

## 📋 Indice

- [✨ Caratteristiche](#-caratteristiche)
- [🔧 Hardware Richiesto](#-hardware-richiesto)
- [📂 Architettura del Progetto](#-architettura-del-progetto)
- [🚀 Funzionalità Implementate](#-funzionalità-implementate)
- [📥 Installazione e Setup](#-installazione-e-setup)
- [⚙️ Configurazione](#️-configurazione)
- [📱 Utilizzo e Navigazione](#-utilizzo-e-navigazione)
- [🔄 OTA Update](#-ota-update)
- [🐛 Stato e Problemi Noti](#-stato-e-problemi-noti)
- [🗺️ Roadmap](#️-roadmap)

---

## ✨ Caratteristiche

- 🖥️ **Display Touchscreen TFT 480×320** con interfaccia fluida e design moderno.
- 📶 **Connessione WiFi** con gestione intelligente delle riconnessioni.
- 🕐 **Sincronizzazione NTP** (Fuso orario Italia CET/CEST automatico).
- 📅 **Google Calendar** — Integrazione via iCal con supporto eventi ricorrenti settimanali.
- ✅ **To-Do List** — Sincronizzata da calendario dedicato (riconoscimento tramite emoji 🎯).
- 🌤️ **Meteo Real-time** — Previsioni a 4 giorni via Open-Meteo API.
- 🌡️ **Sensore Ambiente** — Lettura temperatura tramite sensore DS18B20 (KY-001).
- 🚌 **Trasporti Pubblici** — Orari in tempo reale per GTT Torino (Metro M1 + Bus).
- 🔄 **Aggiornamenti OTA** — Firmware aggiornabile direttamente da GitHub.
- 💤 **Smart Timeout** — Ritorno automatico alla home dopo 30s di inattività.

---

## 🔧 Hardware Richiesto

| Componente | Specifiche | Note |
|---|---|---|
| **MCU** | ESP32-S3 Dev Module | Elevate prestazioni e WiFi integrato |
| **Display touch** | ST7796 3.5" TFT | Risoluzione 480×320 pixel |
| **Sensore Temp** | DS18B20 (KY-001) | Collegato al GPIO 4 |

### Pinout Consigliato (ESP32-S3)
Configurato in `User_Setup.h`:
- **MOSI:** 11 | **SCLK:** 12 | **CS:** 10 | **DC:** 13 | **RST:** 14 | **BL:** 15 | **TOUCH_CS:** 16

---

## 📂 Architettura del Progetto

Il codice è modulare per facilitare la manutenzione e l'espansione:

```text
main/
├── main.ino              # Entry point e gestione loop principale
├── config.h              # Configurazioni globali, costanti e strutture
├── gui_functions.h       # Disegno icone, widget e componenti UI
├── logic_pages.h         # Gestione stati e navigazione tra le pagine
├── ical_parser.h         # Motore di parsing per i file iCal di Google
├── calendar_month.h      # Visualizzazione mensile e dettagli giorno
├── calendar_week.h       # Visualizzazione settimanale
├── bus_schedule.h        # Integrazione API trasporti GTT
├── weather.h             # Gestione meteo e sensore temperatura
├── ota_update.h          # Gestione aggiornamenti firmware da remoto
├── touch_calibration.h   # Calibrazione e salvataggio coordinate touch
└── secrets.h.example     # Template per credenziali private
```

---

## 🚀 Funzionalità Implementate

### Griglia Home (3×3)
| Pagina | Descrizione |
|---|---|
| **Calendario** | Vista settimanale con navigazione e dettagli eventi. |
| **Task** | Lista delle cose da fare sincronizzata in tempo reale. |
| **Meteo** | Condizioni attuali e previsioni per i prossimi giorni. |
| **Trasporti** | Orari bus e metro in tempo reale per la zona di Torino. |
| **Impostazioni** | WiFi, NTP, Calibrazione Touch, OTA, Info Sistema. |

---

## 📥 Installazione e Setup

### 1. Preparazione Ambiente
- Installa [Arduino IDE](https://www.arduino.cc/en/software).
- Aggiungi il supporto per **ESP32** (URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`).

### 2. Librerie Necessarie
Assicurati di aver installato:
- **TFT_eSPI** (Configura il driver ST7796 in `User_Setup.h`)
- **ArduinoJson**
- **OneWire** & **DallasTemperature**

### 3. Configurazione Segreti
1. Copia `main/secrets.h.example` in `main/secrets.h`.
2. Inserisci il tuo **SSID**, la **Password WiFi** e gli **URL iCal** dei tuoi calendari Google.
3. Imposta l'URL del tuo repository per i check OTA.

---

## 📱 Utilizzo e Navigazione

- **Touch Anywhere (Home):** Entra nella griglia principale.
- **Icona Casa (🏠):** Torna sempre alla schermata principale.
- **Icona Ingranaggio (⚙️):** Accesso rapido alle impostazioni (angolo alto-destra nella Home).
- **Auto-Home:** Se non tocchi lo schermo per 30 secondi, il sistema torna alla home per risparmiare energia e mostrare l'orologio.

---

## 🔄 OTA Update

Il sistema controlla periodicamente la presenza di nuove versioni su GitHub.
1. Compila il binario via Arduino IDE (**Sketch > Export Compiled Binary**).
2. Carica lo ZIP del binario e aggiorna `version.json` sul tuo repository.
3. Dal menu Impostazioni sul dispositivo, avvia "Aggiornamento Codice".

---

## 🐛 Stato e Problemi Noti

- 🌡️ **Umidità:** Il sensore attuale (DS18B20) rileva solo la temperatura. L'umidità interna è attualmente disabilitata o mostra `--%`.

---

## 🗺️ Roadmap

- [ ] Supporto per sensori BME280 (Temp/Hum/Pressione).
- [ ] Pagina dedicata alla riproduzione musicale.
- [ ] Salvaschermo con cornice digitale (SPIFFS/SD).

---

## 👤 Autore
**Matteo Casetta** - [@Marr111](https://github.com/Marr111)

---
**⭐ Se ti piace questo progetto, lascia una stella su GitHub!**
