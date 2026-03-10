# 🏠 Casa Casetta - Smart Home Hub v0.7

Hub domotico basato su **ESP32-S3** con display touchscreen **ST7796** (480×320) per il controllo centralizzato della casa intelligente.

![ESP32-S3](https://img.shields.io/badge/ESP32-S3-blue)
![Arduino](https://img.shields.io/badge/Arduino-IDE-00979D?logo=arduino)
![Version](https://img.shields.io/badge/Versione-0.7-green)
![Status](https://img.shields.io/badge/Status-In%20Development-yellow)

## 📋 Indice

- [Caratteristiche](#-caratteristiche)
- [Hardware Richiesto](#-hardware-richiesto)
- [Architettura del Codice](#-architettura-del-codice)
- [Funzionalità Implementate](#-funzionalità-implementate)
- [Installazione](#-installazione)
- [Configurazione](#-configurazione)
- [Utilizzo](#-utilizzo)
- [OTA Update](#-ota-update)
- [Bug Noti](#-bug-noti)
- [Roadmap](#-roadmap)
- [Contribuire](#-contribuire)

## ✨ Caratteristiche

- 🖥️ **Display touchscreen TFT 480×320** con interfaccia grafica personalizzata e sfondi sfumati
- 📶 **Connessione WiFi** con riconnessione dalle impostazioni
- 🕐 **Sincronizzazione NTP** con timezone Italia (CET/CEST automatico)
- 📅 **Google Calendar** — parsing iCal con supporto eventi ricorrenti (RRULE WEEKLY)
- ✅ **To-Do List** — sincronizzata da un calendario Google dedicato (filtro emoji 🎯)
- 📆 **Vista settimanale** del calendario con navigazione per settimane e dettaglio giorno
- 🌤️ **Meteo** — previsioni 4 giorni da Open-Meteo API (Moncalieri, TO)
- 🌡️ **Sensore temperatura** — DS18B20 (KY-001) per la temperatura ambiente
- 🚌 **Orari trasporti** — Metro M1 Torino (Bengasi → Vinzaglio) + Bus 2014 (fermata 3064) con dati real-time da API GPA
- 🔄 **OTA Update** — aggiornamento firmware over-the-air da GitHub
- ⚙️ **Pagina impostazioni** con scroll bar e 5 opzioni (WiFi, NTP, Touch, OTA, Info dispositivo)
- 💤 **Timeout inattività** (30s) con ritorno automatico alla home
- 🏠 **Griglia home 3×3** con icone personalizzate per ogni pagina
- 📊 **Pagina Info Dispositivo** — mostra chip, frequenza, flash, RAM libera, driver display, uptime live

## 🔧 Hardware Richiesto

| Componente | Modello |
|---|---|
| **Microcontrollore** | ESP32-S3 |
| **Display** | ST7796 TFT 480×320 pixel |
| **Touch Controller** | Resistivo (TFT_eSPI) |
| **Sensore Temperatura** | DS18B20 (KY-001) su GPIO 4 |
| **Alimentazione** | 5V via USB-C |

### Pinout Display

Configurazione in `User_Setup.h` della libreria TFT_eSPI:

```cpp
#define ST7796_DRIVER
#define TFT_WIDTH  320
#define TFT_HEIGHT 480
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   13
#define TFT_RST  14
#define TFT_BL   15
#define TOUCH_CS 16
```

Per maggiori informazioni consultare il file [User_Setup.h](file_configurazine_libreria_tft_espi).

## 📂 Architettura del Codice

Il progetto è organizzato in moduli header separati per funzionalità:

```
v0_7/
├── v0_7.ino               # Entry point: setup(), loop(), variabili globali
├── config.h               # Librerie, #define, strutture dati, forward declarations
├── gui_functions.h        # Icone (WiFi, ingranaggio, casa, calendario, task, meteo),
│                          # widget (caricamento, scroll bar, gradiente), griglia home
├── logic_pages.h          # Navigazione pagine, loading screen, impostazioni,
│                          # scroll bar states, info dispositivo, inactivity check
├── network_time.h         # Umbrella include per tutti i moduli di rete
├── net_wifi_time.h        # Connessione WiFi, sincronizzazione NTP, getTime(), getDateLong()
├── ical_parser.h          # Fetch e parsing iCal (eventi + task), supporto RRULE WEEKLY
├── calendar_month.h       # Griglia calendario mensile, dettaglio giorno
├── calendar_week.h        # Vista settimanale, navigazione frecce, To-Do List TFT
├── weather.h              # Meteo Open-Meteo API, icone TFT, lettura sensore DS18B20
├── bus_schedule.h         # Orari metro M1 + bus 2014, fetch API GPA, badge feriale/festivo
├── ota_update.h           # OTA firmware update da GitHub (version.json)
├── touch_calibration.h    # Calibrazione touch con salvataggio SPIFFS, test touch
└── informazioni_e_poblemi.txt  # Note, bug, e to-do per lo sviluppatore
```

### Flusso di inclusione

```
v0_7.ino
  └── config.h             (librerie, definizioni, strutture, prototipi)
  └── gui_functions.h      (dipende da config.h)
  └── bus_schedule.h        (dipende da config.h + gui_functions.h)
  └── logic_pages.h         (dipende da tutti i precedenti)
  └── network_time.h        (umbrella include)
        ├── net_wifi_time.h
        ├── ical_parser.h
        ├── calendar_month.h
        ├── calendar_week.h
        ├── ota_update.h
        └── weather.h
  └── touch_calibration.h
```

## 🚀 Funzionalità Implementate

### Pagine della Griglia Home (3×3)

| Posizione | Pagina | Stato |
|---|---|---|
| Riga 1, Col 1 | 📆 **Calendario** — vista settimanale + dettaglio giorno | ✅ Completa |
| Riga 1, Col 2 | ✅ **Task** — To-Do list con paginazione | ✅ Completa |
| Riga 1, Col 3 | 🌤️ **Meteo** — Oggi + 3 giorni di previsione | ✅ Completa |
| Riga 2, Col 1 | 🚌 **Pullman** — Orari metro e bus real-time | ✅ Completa |
| Riga 2, Col 2 | Pagina 5 | 🚧 Work in progress |
| Riga 2, Col 3 | Pagina 6 | 🚧 Work in progress |
| Riga 3, Col 1 | Pagina 7 | 🚧 Work in progress |
| Riga 3, Col 2 | Pagina 8 | 🚧 Work in progress |
| Riga 3, Col 3 | ⚙️ **Impostazioni** | ✅ Completa |

### Pagina Impostazioni (con scroll bar a 3 stati)

| # | Opzione | Descrizione |
|---|---|---|
| 1 | Collegamento Wi-Fi | Riconnessione alla rete |
| 2 | Sincronizzazione NTP | Aggiornamento ora/data |
| 3 | Calibrazione Touch | Ricalibrazione touchscreen (salvata su SPIFFS) |
| 4 | Aggiornamento Codice | Controlla e installa aggiornamenti OTA da GitHub |
| 5 | Info Dispositivo | Chip, frequenza, flash, RAM, driver, uptime live |

## 📥 Installazione

### 1. Prerequisiti

- [Arduino IDE](https://www.arduino.cc/en/software) 1.8.x o 2.x
- [ESP32 Board Support](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)

### 2. Librerie Necessarie

Installare tramite Library Manager di Arduino IDE:

| Libreria | Note |
|---|---|
| **TFT_eSPI** | Configurata per ST7796 |
| **ArduinoJson** | Parsing JSON per meteo, bus, OTA |
| **OneWire** | Comunicazione sensore DS18B20 |
| **DallasTemperature** | Lettura temperatura DS18B20 |
| WiFi, HTTPClient, WiFiClientSecure | Incluse in ESP32 |
| FS, SPIFFS, SPI, Update | Incluse in ESP32 |

### 3. Configurazione TFT_eSPI

Modificare il file `User_Setup.h` nella cartella della libreria TFT_eSPI (vedi sezione [Pinout Display](#pinout-display)).

### 4. Upload del Codice

```bash
git clone https://github.com/Marr111/hub-casa.git
cd hub-casa
```

1. Apri `v0_7/v0_7.ino` in Arduino IDE
2. Seleziona Board: **ESP32S3 Dev Module**
3. Porta: COMx (Windows) oppure /dev/ttyUSB0 (Linux)
4. Upload!

## ⚙️ Configurazione

Tutte le configurazioni si trovano in `config.h`:

### WiFi

```cpp
#define WIFI_SSID     "TUO_SSID"
#define WIFI_PASSWORD "TUA_PASSWORD"
```

### Google Calendar

1. Rendi pubblico il tuo calendario Google
2. Ottieni l'URL iCal da "Impostazioni e condivisione"
3. Sostituisci in `config.h`:

```cpp
#define ICAL_URL "https://calendar.google.com/calendar/ical/tuo_calendario/public/basic.ics"
```

### Task (To-Do List)

Per le attività, crea un calendario Google separato e aggiungi il suo URL iCal:

```cpp
#define TASK_ICAL_URL "https://calendar.google.com/calendar/ical/tuo_calendario_task/public/basic.ics"
```

I task vengono riconosciuti tramite l'emoji **🎯** all'inizio del titolo dell'evento. Vengono mostrati solo i task del giorno corrente.

### Timezone

Il fuso orario è configurato automaticamente per l'Italia (CET/CEST):

```cpp
// Nel setup()
setenv("TZ", "CET-1CEST,M3.5.0/02:00,M10.5.0/03:00", 1);

// Per NTP
#define GMT_OFFSET_SEC       3600
#define DAYLIGHT_OFFSET_SEC  3600
```

### Limiti

```cpp
#define MAX_EVENTS          60    // Massimo eventi calendario
#define MAX_TASKS           50    // Massimo task
#define TASKS_PER_PAGE       5    // Task per pagina nella lista
#define INACTIVITY_TIMEOUT 30000  // Timeout inattività (30s)
```

## 📱 Utilizzo

### Navigazione

- **Home** → mostra ora e data in tempo reale. Tocca per accedere alla griglia
- **Griglia 3×3** → tocca una cella per aprire la pagina corrispondente
- **Icona Casa** 🏠 → presente su ogni pagina, riporta alla home
- **Icona Ingranaggio** ⚙️ → dalla home, angolo in basso a destra, apre le impostazioni
- **Frecce ◀ ▶** → nella vista settimanale, per navigare tra le settimane
- **Timeout** → dopo 30 secondi di inattività si torna automaticamente alla home

### Coordinate Touch

Il display usa l'asse Y invertita rispetto alle coordinate touch:

```
(480,320)  ---------------------------------- (0,320)
|                                               |
|                                               |
|                                               |
(480,0) ------------------------------------- (0,0)
```

## 🔄 OTA Update

Il sistema supporta aggiornamenti firmware over-the-air tramite GitHub:

1. Il file `version.json` sul repository contiene la versione attuale e l'URL del binario
2. Dalla pagina impostazioni → "Aggiornamento codice" → controlla nuove versioni ogni 24h
3. Se disponibile, scarica e installa automaticamente il firmware

### Compilazione del binario

1. Da Arduino IDE: **Sketch → Export Compiled Binary**
2. Zippare il file `v0_7.ino.bin` generato
3. Push su GitHub:
   ```bash
   git add .
   git commit -m "Descrizione modifiche"
   git push origin main
   ```

## 🐛 Bug Noti

- ⚠️ **Calibrazione touch non caricata all'avvio** — nel `setup()` non viene letto il file di calibrazione SPIFFS e applicato con `tft.setTouch(calData)`
- ⚠️ **Sensore temperatura/umidità** — il DS18B20 (KY-001) misura solo la temperatura; l'umidità appare sempre come `--％`
- ⚠️ **`page4()` irraggiungibile** — la funzione Lampadina RGB non è mai chiamata (il case 4 nella griglia apre `pageBus()` invece)
- ⚠️ **Parsing iCal** — non gestisce tutti i tipi di ricorrenze (solo FREQ=WEEKLY)

## 🗺️ Roadmap

### ✅ Versione 0.7 (Attuale)

- ✅ Interfaccia multi-pagina con griglia home 3×3
- ✅ Connessione WiFi e NTP con timezone automatica
- ✅ Calendario Google con vista settimanale e dettaglio giorno
- ✅ To-Do List sincronizzata da Google Calendar (emoji 🎯)
- ✅ Meteo 4 giorni da Open-Meteo
- ✅ Sensore temperatura DS18B20
- ✅ Orari trasporti real-time (Metro M1 + Bus 2014)
- ✅ Badge feriale/festivo toggle manuale
- ✅ OTA Update da GitHub
- ✅ Pagina Info Dispositivo con uptime live
- ✅ Pagine WIP segnalate con "nastro cantiere"
- ✅ Timeout inattività su tutte le pagine

### 🔮 Prossime Versioni

- [ ] Pagina Lampadina RGB Bluetooth
- [ ] Pagina Musica
- [ ] Pagina Qualità dell'Aria
- [ ] Pagina Timer
- [ ] Cornice Fotografia / Salvaschermo
- [ ] Integrazione sensore umidità (DHT22/BME280)
- [ ] Integrazione MQTT
- [ ] Controllo tapparelle

## 🤝 Contribuire

I contributi sono benvenuti!

1. Fai un fork del progetto
2. Crea un branch per la tua feature (`git checkout -b feature/NuovaFeature`)
3. Committa le modifiche (`git commit -m 'Aggiungi NuovaFeature'`)
4. Push al branch (`git push origin feature/NuovaFeature`)
5. Apri una Pull Request

## 📄 Licenza

Questo progetto è distribuito sotto licenza MIT. Vedi il file `LICENSE` per maggiori dettagli.

## 👤 Autore

**Matteo Casetta**

- GitHub: [@Marr111](https://github.com/Marr111)
- Email: casettamatteo1@gmail.com

## 🙏 Ringraziamenti

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) di Bodmer
- [ArduinoJson](https://arduinojson.org/) di Benoît Blanchon
- [Open-Meteo](https://open-meteo.com/) per le previsioni meteo gratuite
- [GPA MadBob](https://gpa.madbob.org/) per le API trasporti GTT Torino
- Community ESP32 Arduino

---

**⭐ Se questo progetto ti è utile, lascia una stella su GitHub!**
