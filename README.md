# 🏠 Casa Casetta - Smart Home Hub

Hub domotico basato su **ESP32-S3** con display touchscreen **ST7789** per il controllo centralizzato della casa intelligente.

![ESP32-S3](https://img.shields.io/badge/ESP32-S3-blue)
![Arduino](https://img.shields.io/badge/Arduino-IDE-00979D?logo=arduino)
![Status](https://img.shields.io/badge/Status-In%20Development-yellow)

## 📋 Indice

- [Caratteristiche](#-caratteristiche)
- [Hardware Richiesto](#-hardware-richiesto)
- [Funzionalità Implementate](#-funzionalità-implementate)
- [Installazione](#-installazione)
- [Configurazione](#-configurazione)
- [Utilizzo](#-utilizzo)
- [Struttura del Progetto](#-struttura-del-progetto)
- [Roadmap](#-roadmap)
- [Contribuire](#-contribuire)

## ✨ Caratteristiche

- 🖥️ **Display touchscreen TFT 480x320** con interfaccia grafica personalizzata
- 📶 **Connessione WiFi** per integrazione con servizi cloud
- 🕐 **Sincronizzazione NTP** per ora e data accurate
- 📅 **Integrazione Google Calendar** (iCal) per visualizzare eventi
- ✅ **To-Do List** integrata
- 🎨 **Interfaccia multi-pagina** con navigazione touch
- ⚙️ **Pagina impostazioni** con calibrazione touch e configurazione
- 💤 **Timeout inattività** con ritorno automatico alla home
- 🏠 **Navigazione intuitiva** con icona home sempre accessibile

## 🔧 Hardware Richiesto

| Componente | Modello |
|-----------|---------|
| **Microcontrollore** | ESP32-S3 |
| **Display** | ST7789 TFT 480x320 pixel |
| **Touch Controller** | Resistivo (compatibile TFT_eSPI) |
| **Alimentazione** | 5V via USB-C |

### Pinout Display

```cpp
// Configurazione pin in User_Setup.h (TFT_eSPI)
#define TFT_CS   10
#define TFT_DC   11
#define TFT_RST  12
#define TFT_BL   13
#define TOUCH_CS 14
```

per maggiori informazioni consultare il file premi [qui](file configurazine libreria tft_espi) 

## 🚀 Funzionalità Implementate

### ✅ Completate

- [x] Interfaccia grafica base con 7 pagine navigabili
- [x] Connessione WiFi con gestione tentativi
- [x] Sincronizzazione ora/data via NTP
- [x] Calibrazione touchscreen con salvataggio su SPIFFS
- [x] Timeout inattività (30 secondi)
- [x] Icone WiFi, Impostazioni, Home
- [x] Scroll bar nella pagina impostazioni
- [x] Parsing calendario Google (iCal/ICS)
- [x] Visualizzazione eventi futuri
- [x] Sistema di navigazione con frecce laterali

### 🔨 In Sviluppo

- [ ] Controllo lampadina RGB Bluetooth
- [ ] Integrazione sensori ambientali
- [ ] Gestione scene personalizzate
- [ ] Grafici storici temperatura/umidità
- [ ] Notifiche push

## 📥 Installazione

### 1. Prerequisiti

- [Arduino IDE](https://www.arduino.cc/en/software) 1.8.x o 2.x
- [ESP32 Board Support](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)

### 2. Librerie Necessarie

Installare tramite Library Manager di Arduino IDE:

```
- TFT_eSPI (configurata per ST7789)
- WiFi (inclusa in ESP32)
- HTTPClient (inclusa in ESP32)
- FS e SPIFFS (incluse in ESP32)
```

### 3. Configurazione TFT_eSPI

Modificare il file `User_Setup.h` nella cartella della libreria TFT_eSPI:

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

### 4. Upload del Codice

```bash
# Clone del repository
git clone https://github.com/tuousername/casa-casetta.git
cd casa-casetta

# Apri il file .ino in Arduino IDE
# Seleziona Board: ESP32S3 Dev Module
# Porta: /dev/ttyUSB0 (o COMx su Windows)
# Upload!
```

## ⚙️ Configurazione

### WiFi

Modificare le credenziali nel codice:

```cpp
const char *ssid = "TUO_SSID";
const char *password = "TUA_PASSWORD";
```

### Google Calendar

1. Rendi pubblico il tuo calendario Google
2. Ottieni l'URL iCal dal menu "Impostazioni e condivisione"
3. Sostituisci l'URL nel codice:

```cpp
const char *ICAL_URL = "https://calendar.google.com/calendar/ical/tuo_calendario/public/basic.ics";
```

### Timezone

Modifica il fuso orario (default: Italia UTC+1):

```cpp
const long gmtOffset_sec = 3600;      // GMT+1
const int daylightOffset_sec = 3600;  // Ora legale
```

## 📱 Utilizzo

### Navigazione

- **Home**: Tocca lo schermo per accedere al menu
- **Frecce laterali**: Scorri tra le pagine (1-7)
- **Icona Home** 🏠: Ritorna alla schermata principale
- **Icona Impostazioni** ⚙️: Accedi alle configurazioni

### Pagine Disponibili

| Pagina | Contenuto |
|--------|-----------|
| **0** | Home - Ora e Data |
| **1** | Pagina vuota (personalizzabile) |
| **2** | Eventi Google Calendar |
| **3** | To-Do List |
| **4** | Controllo Lampadina RGB (WIP) |
| **5-7** | Pagine future |

### Impostazioni

Dalla pagina impostazioni puoi:

1. **Riconnettere WiFi** - Aggiorna connessione di rete
2. **Risincronizzare NTP** - Aggiorna ora/data
3. **Ricalibrare Touch** - Ricalibra il touchscreen

## 📂 Struttura del Progetto

```
casa-casetta/
├── casa_casetta.ino          # Codice principale
├── README.md                 # Questo file
├── docs/
│   ├── pinout.png           # Schema collegamenti
│   └── screenshots/         # Screenshot interfaccia
└── libraries/               # Librerie custom (se necessarie)
```

## 🗺️ Roadmap

### Versione 1.0 (Attuale)
- ✅ Interfaccia base funzionante
- ✅ Connessione WiFi e NTP
- ✅ Calendario Google

### Versione 1.5
- [ ] Controllo lampadine Bluetooth
- [ ] Widget meteo
- [ ] Grafici temperatura

### Versione 2.0
- [ ] Integrazione MQTT
- [ ] Controllo tapparelle
- [ ] Sistema multilingua
- [ ] OTA Updates

## 🐛 Bug Noti

- ⚠️ Calibrazione touch richiede riavvio manuale
- ⚠️ Eventi Google Calendar limitati a 3 (MAX_EVENTS)
- ⚠️ Parsing iCal base (non gestisce ricorrenze)

## 🤝 Contribuire

I contributi sono benvenuti! Per favore:

1. Fai un fork del progetto
2. Crea un branch per la tua feature (`git checkout -b feature/AmazingFeature`)
3. Committa le modifiche (`git commit -m 'Add some AmazingFeature'`)
4. Push al branch (`git push origin feature/AmazingFeature`)
5. Apri una Pull Request

### Workflow Git Consigliato

```bash
git add .
git commit -m "Descrizione modifiche"
git push
```

## 📄 Licenza

Questo progetto è distribuito sotto licenza MIT. Vedi il file `LICENSE` per maggiori dettagli.

## 👤 Autore

**Matteo Casetta**

- GitHub: [@tuousername](https://github.com/tuousername)
- Email: casettamatteo1@gmail.com

## 🙏 Ringraziamenti

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) di Bodmer
- Community ESP32 Arduino
- Documentazione Google Calendar API

---

**⭐ Se questo progetto ti è utile, lascia una stella su GitHub!**
