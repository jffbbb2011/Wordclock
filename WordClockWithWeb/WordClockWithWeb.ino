// ###########################################################################################################################################
// #
// # WordClock code for the thingiverse WordClock project: https://www.thingiverse.com/thing:4693081
// #
// # Code by https://github.com/N1cls and https://github.com/AWSW-de
// #
// # Released under license: GNU General Public License v3.0 https://github.com/N1cls/Wordclock/blob/master/LICENSE.md
// #
// ###########################################################################################################################################
// # [FORK] Fork of jffbbb2011 (based on N1cls V6.1.0). Own version numbers have 4 parts (V6.1.0.1, V6.1.0.2, ...), see
// #   parseVersion() / compareVersions(). The update source is UPDATE_REPO in settings.h. Changes are marked "[FORK]" or "[V2]".
// # [FORK] V6.1.0.2: WiFi reset button (BUTTON_PIN / BUTTON_HOLD_MS in settings.h): checkResetButton() is called from serviceDelay().
// #
// # [V2] CHANGES compared to the original WordClockWithWeb.ino:
// # - Temperature / humidity: the old text style 2 "two lines" (big 5x7 number + small unit) is replaced by
// #     style 2 = "compact":              small 3x5 digits (no decimals) + small unit below, all in ONE window.
// #                                       3 characters (100 %, -12 C) now fit as well (3 x 3 columns + 2 gaps = 11 columns).
// #     style 3 = "compact + colour bar": same, but the two left columns show a bar (height and colour depend on the value).
// #                                       Needs 8 columns for the number: with 3 characters the bar is skipped (style 2 layout).
// #   The stored values 0/1/2 keep working (2 simply looks different now), so the EEPROM layout did NOT change.
// # - New: TXT_BAR_TEMP_MIN / TXT_BAR_TEMP_MAX (range of the temperature bar), see txtBarColor().
// ###########################################################################################################################################
/*
      ___           ___           ___           ___           ___           ___       ___           ___           ___     
     /\__\         /\  \         /\  \         /\  \         /\  \         /\__\     /\  \         /\  \         /\__\    
    /:/ _/_       /::\  \       /::\  \       /::\  \       /::\  \       /:/  /    /::\  \       /::\  \       /:/  /    
   /:/ /\__\     /:/\:\  \     /:/\:\  \     /:/\:\  \     /:/\:\  \     /:/  /    /:/\:\  \     /:/\:\  \     /:/__/     
  /:/ /:/ _/_   /:/  \:\  \   /::\~\:\  \   /:/  \:\__\   /:/  \:\  \   /:/  /    /:/  \:\  \   /:/  \:\  \   /::\__\____ 
 /:/_/:/ /\__\ /:/__/ \:\__\ /:/\:\ \:\__\ /:/__/ \:|__| /:/__/ \:\__\ /:/__/    /:/__/ \:\__\ /:/__/ \:\__\ /:/\:::::\__\
 \:\/:/ /:/  / \:\  \ /:/  / \/_|::\/:/  / \:\  \ /:/  / \:\  \  \/__/ \:\  \    \:\  \ /:/  / \:\  \  \/__/ \/_|:|~~|~   
  \::/_/:/  /   \:\  /:/  /     |:|::/  /   \:\  /:/  /   \:\  \        \:\  \    \:\  /:/  /   \:\  \          |:|  |    
   \:\/:/  /     \:\/:/  /      |:|\/__/     \:\/:/  /     \:\  \        \:\  \    \:\/:/  /     \:\  \         |:|  |    
    \::/  /       \::/  /       |:|  |        \::/__/       \:\__\        \:\__\    \::/  /       \:\__\        |:|  |    
     \/__/         \/__/         \|__|         ~~            \/__/         \/__/     \/__/         \/__/         \|__|    
*/


// ###########################################################################################################################################
// # Includes:
// #
// # You will need to add the following libraries to your Arduino IDE to use the project:
// # - Adafruit BusIO                 // by Adafruit:                     https://github.com/adafruit/Adafruit_BusIO
// # - Adafruit NeoPixel              // by Adafruit:                     https://github.com/adafruit/Adafruit_NeoPixel
// # - RTClib                         // by Adafruit:                     https://github.com/adafruit/RTClib
// # - WiFiManager                    // by tablatronix / tzapu:          https://github.com/tzapu/WiFiManager
// # - ESPping                        // by dvarrel:                      https://github.com/dvarrel/ESPping
// # - ArduinoJson (version 7)        // by Benoit Blanchon:              https://github.com/bblanchon/ArduinoJson
// #
// # Boards: ESP8266 (NodeMCU, D1 mini) with ESP8266 core 3.1.x, or ESP32 / ESP32-S3 / ESP32-C3 with ESP32 core 3.x.
// # ESP32: select the partition scheme "Minimal SPIFFS (1.9MB APP with OTA)", the automatic update needs it.
// #
// ###########################################################################################################################################
#if defined(ESP32)
// ESP32: the libraries have other names, but the same functions. The aliases below let the rest of the code use the ESP8266 names.
#include <WiFi.h>                                   // Used to connect the ESP32 to your WiFi
#include <WebServer.h>                              // Used for the internal webserver
#include <ESPmDNS.h>                                // Used for the internal update function
#include <HTTPUpdate.h>                             // Used for the http update function
#include <HTTPUpdateServer.h>                       // Used for the internal update function
#include <HTTPClient.h>                             // Used to check for updates
#include <esp_sntp.h>                               // Used to notice successful NTP time syncs
#include <driver/gpio.h>                            // Used to check the configured GPIO pins
typedef WebServer ESP8266WebServer;                 // Same interface as on the ESP8266
typedef HTTPUpdateServer ESP8266HTTPUpdateServer;   // Same interface as on the ESP8266
#define ESPhttpUpdate httpUpdate                    // Same interface as on the ESP8266
#define ESP_WDT_FEED() yield()                      // The ESP32 has no ESP.wdtFeed()
#else
#include <ESP8266WiFi.h>              // Used to connect the ESP8266 NODE MCU to your WiFi
#include <ESP8266WebServer.h>         // Used for the internal webserver
#include <ESP8266mDNS.h>              // Used for the internal update function
#include <ESP8266httpUpdate.h>        // Used for the http update function
#include <ESP8266HTTPUpdateServer.h>  // Used for the internal update function
#include <ESP8266HTTPClient.h>        // Used to check for updates
#include <coredecls.h>                // Used to notice successful NTP time syncs
#define ESP_WDT_FEED() ESP.wdtFeed()  // Reset watchdog timer
#endif
#include <WiFiClientSecure.h>         // Used for the HTTPS connection to the GitHub releases (update function)
#include <DNSServer.h>                // Used for name resolution for the internal webserver
#include <WiFiManager.h>              // Used for the WiFi Manager option to be able to connect the WordClock to your WiFi without code changes
#include <EEPROM.h>                   // Used to store the in the internal configuration page set configuration on the ESP internal storage
#include <Adafruit_NeoPixel.h>        // Used to drive the NeoPixel LEDs
#include <time.h>                     // Used to get the time from the internet
#include <sys/time.h>                 // Used to set the time from the RTC
#include <Wire.h>                     // Used to connect the RTC board
#include "Adafruit_HTU21DF.h"         // Used to read temperature and humidity from the GY-21 / HTU21D sensor board (I2C, shares the bus with the RTC)
#include "RTClib.h"                   // Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <ESPping.h>                  // Used to send ping requests to a IP-address (of your smart phone) to detect if you have left your home
#include <ArduinoJson.h>              // Used for the data exchange between the configuration page and the WordClock
#include "settings.h"                 // Settings are stored in a seperate file to make to code better readable and to be able to switch to other settings faster
#include "webpage.h"                  // Configuration page (gzipped), generated from web/index.html by tools/build_webpage.py

#if defined(ESP32)
// Adafruit NeoPixel puts the whole RMT buffer for the 114 LEDs (about 11 KB) on the stack of the calling task,
// more than the default 8 KB of the Arduino loop task. The HTTPS connection to GitHub needs stack as well.
SET_LOOP_TASK_STACK_SIZE(24 * 1024);
#endif


// ###########################################################################################################################################
// # Version number of the code:
// ###########################################################################################################################################
const char* WORD_CLOCK_VERSION = "V6.1.0.2";  // [FORK] was "V6.1.0". The release tag on GitHub must be exactly this text. (V6.1.0.1 -> V6.1.0.2: WiFi reset button)


// ###########################################################################################################################################
// # Declartions and variables used in the functions:
// ###########################################################################################################################################
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);  // LED strip settings (the pin can be changed on the configuration page)
RTC_DS3231 rtc;                                                                      // rtc communication object
unsigned long lastRequest = 0;                                                       // Variable to control RTC requests: millis() when the time was last read from the RTC
Adafruit_HTU21DF htu = Adafruit_HTU21DF();  // GY-21 / HTU21D communication object (temperature & humidity)
int htuStarted = 0;                          // Variable to control whether the HTU21D has been initialized
String bootText1 = DEFAULT_BOOTTEXT1;        // 1st scrolling text shown at boot (configurable via web page)
String bootText2 = DEFAULT_BOOTTEXT2;        // 2nd scrolling text shown at boot (configurable via web page)
bool triggerShowDateNow = false;             // Flag set by the "Show now" button on the web page - Date
bool triggerShowTempNow = false;             // Flag set by the "Show now" button on the web page - Temperature
bool triggerShowHumidityNow = false;         // Flag set by the "Show now" button on the web page - Humidity
int rtcStarted = 0;                                                                  // Variable to control whether RTC has been initialized
bool rtcTimeValid = false;                                                           // RTC holds a usable time (battery ok)
bool timeValid = false;                                                              // The clock knows the time (NTP, RTC or set on the configuration page)
const time_t MIN_VALID_TIME = 1704067200;                                            // 2024-01-01 - anything earlier means "time not set yet"
volatile bool ntpSyncPending = false;                                                // Set by the NTP callback, handled in handleTime()
unsigned long lastNtpSync = 0;                                                       // millis() of the last successful NTP sync (0 = never)
int delayval = 250;                                                                  // delay in milliseconds
int iYear, iMonth, iDay, iHour, iMinute, iSecond, iWeekDay;                          // variables for RTC-module read time:
String timeZone = DEFAULT_TIMEZONE;                                                  // Time server settings
String ntpServer = DEFAULT_NTP_SERVER;                                               // Time server settings
String UpdatePath = "-";                                                             // Update via Hostname
String UpdatePathIP = "-";                                                           // Update via IP-address
ESP8266HTTPUpdateServer httpUpdater;                                                 // Update server
std::unique_ptr<ESP8266WebServer> server1;                                           // REST function web server
WiFiManager wifiManager;                                                             // WiFi Manager (setup hotspot "WordClock")
bool portalActive = false;                                                           // WiFi Manager setup hotspot is open
bool servicesStarted = false;                                                        // Configuration page, REST and update functions are running
bool wasConnected = false;                                                           // WiFi state of the last loop pass
bool ipShown = false;                                                                // IP-address was shown after the first connection
int lastColorMinute = -1;                                                            // Minute of the last random colors (RainBow variants)
uint32_t rainbowSeed = 0;                                                            // RainBow variant 1: random colors of the words, new every minute
int rainbowWordNr = -1;                                                              // RainBow variant 1: number of the word set by showCurrentTime(), -1 = not in showCurrentTime()
uint16_t rainbowWordHues[16];                                                        // RainBow variant 1: colors of the words set before in this minute
int PING_ATTEMPTSIP1 = PING_TIMEOUTNUM;                                              // 1st IP-addres attempts of missed PING requests
int PING_ATTEMPTSIP2 = PING_TIMEOUTNUM;                                              // 2nd IP-addres attempts of missed PING requests
int PING_ATTEMPTSIP3 = PING_TIMEOUTNUM;                                              // 3rd IP-addres attempts of missed PING requests
bool PingStatusIP1 = true;                                                           // Status flag 1st IP-address - PING function
bool PingStatusIP2 = true;                                                           // Status flag 2nd IP-address - PING function
bool PingStatusIP3 = true;                                                           // Status flag 3rd IP-address - PING function
bool LEDsON = true;                                                                  // Global flag to turn LEDs on or off - Used for the PING function
bool RESTmanLEDsON = true;                                                           // Global flag to turn LEDs manually on or off - Used for the REST function
bool UpdateAvailable = false;                                                        // Global flag to check for avaiable updates
String AvailableVersion = "-";                                                       // Global string to check for avaiable updates
bool NightModeActive = false;                                                        // Global flag to track if Night Mode is currently active - used for PING function


// ###########################################################################################################################################
// # Parameter record to store to the EEPROM of the ESP:
// ###########################################################################################################################################
struct parmRec {
  int pRed;
  int pGreen;
  int pBlue;
  int pIntensity;
  int pIntensityNight;
  int pShowDate;
  char pdisplayonmaxMO;
  char pdisplayonminMO;
  char pdisplayonmaxTU;
  char pdisplayonminTU;
  char pdisplayonmaxWE;
  char pdisplayonminWE;
  char pdisplayonmaxTH;
  char pdisplayonminTH;
  char pdisplayonmaxFR;
  char pdisplayonminFR;
  char pdisplayonmaxSA;
  char pdisplayonminSA;
  char pdisplayonmaxSU;
  char pdisplayonminSU;
  int pdisplayoff;
  int puseNightLEDs;
  int puseupdate;
  int puseresturl;
  int ppowersupply;
  int puseledtest;
  int pusesetwlan;
  int puseshowip;
  int pswitchRainBow;
  int pswitchLangWeb;
  int pswitchLEDOrder;
  int puseCustomMinuteOrder;
  int pMinuteOrder1;
  int pMinuteOrder2;
  int pMinuteOrder3;
  int pMinuteOrder4;
  int pwchostnamenum;
  int pDCWFlag;
  int puseRTC;
  int pBlinkTime;
  int pPING_IP_ADDR1_O1;
  int pPING_IP_ADDR1_O2;
  int pPING_IP_ADDR1_O3;
  int pPING_IP_ADDR1_O4;
  int pPING_IP_ADDR2_O1;
  int pPING_IP_ADDR2_O2;
  int pPING_IP_ADDR2_O3;
  int pPING_IP_ADDR2_O4;
  int pPING_IP_ADDR3_O1;
  int pPING_IP_ADDR3_O2;
  int pPING_IP_ADDR3_O3;
  int pPING_IP_ADDR3_O4;
  int pPING_TIMEOUTNUM;
  int pPING_DEBUG_MODE;
  int pPING_USEMONITOR;
  int pDEspecial1;
  char pTimeZone[50];
  char pNTPServer[50];
  int pShowDateIntervalSec;
  int pUseHTU21;
  int pShowTemp;
  int pShowTempIntervalSec;
  int pShowHumidity;
  int pShowHumidityIntervalSec;
  int pUseBootText;
  char pBootText1[21];
  char pBootText2[21];
  int pCheckSum;  // This checkSum is used to find out whether we have valid parameters
} parameter;


// ###########################################################################################################################################
// # Hardware settings (GPIO pins), stored behind the parameter record so that the existing settings stay valid.
// # They can be changed on the configuration page (System -> Hardware) and are kept during updates.
// ###########################################################################################################################################
struct hwRec {
  uint32_t magic;
  int16_t ledPin;  // LED strip data
  int16_t sdaPin;  // RTC SDA
  int16_t sclPin;  // RTC SCL
  uint8_t rtcUtc;  // 1 = the RTC holds UTC (V6.1.0 and newer), 0 = local time (older versions)
  uint8_t reserved;
} hw;
#define HW_MAGIC 0x57434831UL  // "WCH1"
#define HW_OFFSET (((sizeof(parameter) + 3) / 4) * 4)

// ###########################################################################################################################################
// # [TEXT STYLES] Settings of the floating texts (date / temperature / humidity), stored behind the hardware record so that all
// # existing settings stay valid. Configured on the web page (tab "Floating texts").
// ###########################################################################################################################################
struct txtRec {
  uint32_t magic;
  uint8_t dateStyle;  // 0 = scroll, 1 = scroll with stops, 2 = fixed pages
  uint8_t tempStyle;  // 0 = scroll, 1 = scroll with stops, 2 = compact (small number + small unit), 3 = compact + colour bar  [V2: was 2 = two-line]
  uint8_t humStyle;   // same as tempStyle (0..3)
  uint8_t tempDec;    // 1 = temperature with one decimal (scroll styles)
  uint8_t dateYear;   // 1 = show the year
  uint8_t reserved[3];
  uint16_t dateSpeed;  // milliseconds per column (smaller = faster)
  uint16_t tempSpeed;
  uint16_t humSpeed;
  uint16_t holdMs;  // time a stopped group / page stays on the display
} txt;
#define TXT_MAGIC 0x57435431UL  // "WCT1"
#define TXT_OFFSET (HW_OFFSET + ((sizeof(hwRec) + 3) / 4) * 4)
#define EEPROM_SIZE (TXT_OFFSET + sizeof(txtRec))

void txtDefaults() {
  txt.magic = TXT_MAGIC;
  txt.dateStyle = txt.tempStyle = txt.humStyle = 0;
  txt.tempDec = 1;
  txt.dateYear = 1;
  txt.reserved[0] = txt.reserved[1] = txt.reserved[2] = 0;
  txt.dateSpeed = txt.tempSpeed = txt.humSpeed = 150;
  txt.holdMs = 2000;
}

void writeTxtConfig() {
  txt.magic = TXT_MAGIC;
  EEPROM.put(TXT_OFFSET, txt);
  EEPROM.commit();
}

bool validPin(int p) {
#if defined(ESP32)
  return p >= 0 && p < GPIO_NUM_MAX && GPIO_IS_VALID_OUTPUT_GPIO(p);
#else
  return p >= 0 && p <= 16 && !(p >= 6 && p <= 11);  // GPIO 6..11 are used by the flash memory
#endif
}

void writeHwConfig() {
  hw.magic = HW_MAGIC;
  EEPROM.put(HW_OFFSET, hw);
  EEPROM.commit();
}

// ###########################################################################################################################################
// # Minute corner LEDs order (1 LED per minute):
// # - Default behavior is kept via switchLEDOrder (clockwise / anti clockwise)
// # - Optional: fully custom order via web interface (saved to EEPROM)
// #
// # minuteLedOrder[] stores the LED indices (0..NUMPIXELS-1) in the order for minute 1..4.
// # You can enter either:
// #   - a permutation of 1..4 (e.g. "3,1,4,2") which will be mapped to the base corner LEDs {110,111,112,113}
// #   - OR the real LED numbers directly (e.g. "112,113,110,111")
// ###########################################################################################################################################
int useCustomMinuteOrder = 0;                  // 0 = use switchLEDOrder mapping, 1 = use minuteLedOrder[]
int minuteLedOrder[4] = {112, 113, 110, 111}; // default (clockwise, "new wiring")
const int minuteCornerBase[4] = {110, 111, 112, 113};

void setDefaultMinuteLedOrderFromSwitch() {
  if (switchLEDOrder) {  // clockwise
    minuteLedOrder[0] = 112;
    minuteLedOrder[1] = 113;
    minuteLedOrder[2] = 110;
    minuteLedOrder[3] = 111;
  } else {  // anti clockwise
    minuteLedOrder[0] = 113;
    minuteLedOrder[1] = 112;
    minuteLedOrder[2] = 111;
    minuteLedOrder[3] = 110;
  }
}

static String urlDecodeMinimal(String s) {
  s.replace("+", " ");
  s.replace("%2C", ",");
  s.replace("%2c", ",");
  s.replace("%3B", ";");
  s.replace("%3b", ";");
  return s;
}

static bool isUnique4(const int a[4]) {
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 4; j++) {
      if (a[i] == a[j]) return false;
    }
  }
  return true;
}

// Parse "3,1,4,2" (perm 1..4) or "112,113,110,111" (LED indices)
static bool parseMinuteOrder(String raw, int outOrder[4]) {
  raw = urlDecodeMinimal(raw);
  raw.trim();
  if (raw.length() == 0) return false;

  raw.replace(";", ",");
  raw.replace(" ", ",");
  while (raw.indexOf(",,") >= 0) raw.replace(",,", ",");

  int vals[4] = {-1, -1, -1, -1};
  int found = 0;

  int start = 0;
  while (found < 4) {
    int comma = raw.indexOf(',', start);
    String token = (comma >= 0) ? raw.substring(start, comma) : raw.substring(start);
    token.trim();
    if (token.length() == 0) break;
    vals[found++] = token.toInt();
    if (comma < 0) break;
    start = comma + 1;
  }
  if (found != 4) return false;

  // Case A: permutation of 1..4 -> map to base {110,111,112,113}
  bool perm = true;
  for (int i = 0; i < 4; i++) {
    if (vals[i] < 1 || vals[i] > 4) { perm = false; break; }
  }
  if (perm) {
    if (!isUnique4(vals)) return false;
    for (int i = 0; i < 4; i++) outOrder[i] = minuteCornerBase[vals[i] - 1];
    return true;
  }

  // Case B: direct LED indices (e.g. 110..113)
  for (int i = 0; i < 4; i++) {
    if (vals[i] < 0 || vals[i] >= NUMPIXELS) return false;
    outOrder[i] = vals[i];
  }
  if (!isUnique4(outOrder)) return false;
  return true;
}

static String minuteOrderToDisplayString() {
  // Prefer showing as 1..4 if possible (matches base corner set)
  int perm[4] = {-1, -1, -1, -1};
  bool allInBase = true;
  for (int i = 0; i < 4; i++) {
    int idx = -1;
    for (int j = 0; j < 4; j++) {
      if (minuteLedOrder[i] == minuteCornerBase[j]) { idx = j; break; }
    }
    if (idx < 0) { allInBase = false; break; }
    perm[i] = idx + 1;
  }
  String s = "";
  for (int i = 0; i < 4; i++) {
    if (i) s += ",";
    s += allInBase ? String(perm[i]) : String(minuteLedOrder[i]);
  }
  return s;
}

// ###########################################################################################################################################
// # Setup function that runs once at startup of the ESP:
// ###########################################################################################################################################
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("######################################################################");
  Serial.print("# WordClock startup of version: ");
  Serial.print(WORD_CLOCK_VERSION);
  Serial.println(" (" CHIP_NAME ")");
  Serial.println("######################################################################");
  readEEPROM();                     // get persistent data from EEPROM (and the pins)
  setupResetButton();               // [FORK] WiFi reset button (pin check + pull-up)
  pixels.setPin(hw.ledPin);         // LED pin from the configuration page
  pixels.begin();                   // Init the NeoPixel library
  dunkel();                         // Switch display black
  if (!useCustomMinuteOrder) setDefaultMinuteLedOrderFromSwitch();    // Set default minute corner LED order based on switchLEDOrder setting
  pixels.setBrightness(intensity);  // Set LED brightness
#if defined(ESP32)
  sntp_set_time_sync_notification_cb(ntpSyncCallback);  // Remember when the time came from the NTP server
#else
  settimeofday_cb([](bool fromSntp) {  // Remember when the time came from the NTP server
    if (fromSntp) ntpSyncPending = true;
  });
#endif
  setenv("TZ", timeZone.c_str(), 1);  // Time zone, so the RTC time is shown correctly before the first NTP sync
  tzset();
  rtcReadTime();                    // Take the time from the RTC, if one is connected
  DisplayTest();                    // Perform the LED test
  showBootTexts();                  // Show the two configurable scrolling boot texts
  pixels.setBrightness(intensity);  // Set LED brightness
  handleTime();                     // handle NTP / RTC time
  if (timeValid) {                  // Time from the RTC: show it right away
    ShowTheTime();
    pixels.show();
  } else {
    SetWLAN();  // Show SET WLAN text
  }
  WIFI_login();  // WiFiManager: connect to the saved WiFi or open the setup hotspot - both without stopping the clock
  Serial.println("######################################################################");
  Serial.println("# WordClock startup finished...");
  Serial.println("######################################################################");
}


// ###########################################################################################################################################
// # Loop function which runs all the time after the startup was done:
// ###########################################################################################################################################
void loop() {
  ESP_WDT_FEED();  // Reset watchdog timer

  // Check WiFi connection - the clock keeps running without WiFi, the time then comes from the RTC:
  WIFI_check();
  // Check, whether something has been entered on Config Page
  checkClient();
  ESP_WDT_FEED();  // Reset watchdog timer
  handleTime();    // handle NTP / RTC time
  serviceDelay(750);

  if (!timeValid) {  // Neither NTP nor RTC delivered a time yet: show that WiFi is needed
    pixels.setBrightness(intensity);
    dunkel();
    SetWLAN();
  } else {
    // RainBow effects - new colors every new minute (one loop pass takes longer than a second, so "iSecond == 0" could be skipped):
    if (iMinute != lastColorMinute) {
      lastColorMinute = iMinute;
      rainbowSeed = random(0x7FFFFFFF);  // RainBow variant 1: new random colors for the words
      if (switchRainBow == 2) {          // RainBow variant 2 effect active - one random color for all words
        redVal = random(255);
        greenVal = random(255);
        blueVal = random(255);
      }
    }

    // Show the display only during the set Min/Max time if option is set
    if (displayoff) {
      switch (iWeekDay) {
        case 0:  // Sunday
          DayNightMode(displayonminSU, displayonmaxSU);
          break;
        case 1:  // Monday
          DayNightMode(displayonminMO, displayonmaxMO);
          break;
        case 2:  // Tuesday
          DayNightMode(displayonminTU, displayonmaxTU);
          break;
        case 3:  // Wednesday
          DayNightMode(displayonminWE, displayonmaxWE);
          break;
        case 4:  // Thursday
          DayNightMode(displayonminTH, displayonmaxTH);
          break;
        case 5:  // Friday
          DayNightMode(displayonminFR, displayonmaxFR);
          break;
        case 6:  // Saturday
          DayNightMode(displayonminSA, displayonmaxSA);
          break;
      }
    } else {
      pixels.setBrightness(intensity);  // DAY brightness
      ShowTheTime();
    }
  }

  ESP_WDT_FEED();  // Reset watchdog timer
  serviceDelay(delayval);
  ESP_WDT_FEED();  // Reset watchdog timer

  // Web update start:
  if (servicesStarted) {
    httpServer.handleClient();
#if !defined(ESP32)
    MDNS.update();
#endif
  }

  // PING function:
  if (PING_USEMONITOR == 1 && WiFi.status() == WL_CONNECTED) PingIP();

  if (LEDsON == true && RESTmanLEDsON == true) pixels.show();  // This sends the updated pixel color to the hardware.
}


// ###########################################################################################################################################
// # Try to read settings from FLASH - initialize if WLAN ID read from flash is invalid:
// ###########################################################################################################################################
void readEEPROM() {
  //Serial.print("Copy ");
  //Serial.print(sizeof(parameter));
  //Serial.println(" bytes from flash memory to EPROM buffer: ");

  // initialize space to read parameter and hardware settings
  EEPROM.begin(EEPROM_SIZE);
  delay(10);

  // Copy bytes from EEPROM
  byte* p = (byte*)(void*)&parameter;

  int check = 0;
  for (int L = 0; L < sizeof(parameter); ++L) {
    byte b = EEPROM.read(L);

    // Don't take the checkSum itself into account
    if (L < sizeof(parameter) - sizeof(parameter.pCheckSum))
      check = check + b;

    *p++ = b;
    /*  Serial.print("Read FLASH Byte ");
      Serial.print(L);
      Serial.print(" = ");
      Serial.println(b);  */
  }

  // Check checksum
  //Serial.print("Compare checksums: ");
  //Serial.print(check);
  //Serial.print("/");
  //Serial.println(parameter.pCheckSum);

  if (check == parameter.pCheckSum) {
    // Serial.println("Checksum does match. Get parameter values from EEPROM");
    redVal = parameter.pRed;
    greenVal = parameter.pGreen;
    blueVal = parameter.pBlue;
    showDate = parameter.pShowDate;
    showDateIntervalSec = parameter.pShowDateIntervalSec;
    useHTU21 = parameter.pUseHTU21;
    showTemp = parameter.pShowTemp;
    showTempIntervalSec = parameter.pShowTempIntervalSec;
    showHumidity = parameter.pShowHumidity;
    showHumidityIntervalSec = parameter.pShowHumidityIntervalSec;
    if (showDateIntervalSec < 30 || showDateIntervalSec > 86400) showDateIntervalSec = 60;
    if (showTempIntervalSec < 30 || showTempIntervalSec > 86400) showTempIntervalSec = 300;
    if (showHumidityIntervalSec < 30 || showHumidityIntervalSec > 86400) showHumidityIntervalSec = 300;
    useBootText = parameter.pUseBootText;
    parameter.pBootText1[20] = 0;
    parameter.pBootText2[20] = 0;
    bootText1 = String(parameter.pBootText1);
    bootText2 = String(parameter.pBootText2);
    displayoff = parameter.pdisplayoff;
    useNightLEDs = parameter.puseNightLEDs;
    displayonmaxMO = parameter.pdisplayonmaxMO;
    displayonminMO = parameter.pdisplayonminMO;
    displayonmaxTU = parameter.pdisplayonmaxTU;
    displayonminTU = parameter.pdisplayonminTU;
    displayonmaxWE = parameter.pdisplayonmaxWE;
    displayonminWE = parameter.pdisplayonminWE;
    displayonmaxTH = parameter.pdisplayonmaxTH;
    displayonminTH = parameter.pdisplayonminTH;
    displayonmaxFR = parameter.pdisplayonmaxFR;
    displayonminFR = parameter.pdisplayonminFR;
    displayonmaxSA = parameter.pdisplayonmaxSA;
    displayonminSA = parameter.pdisplayonminSA;
    displayonmaxSU = parameter.pdisplayonmaxSU;
    displayonminSU = parameter.pdisplayonminSU;
    wchostnamenum = parameter.pwchostnamenum;
    useupdate = parameter.puseupdate;
    useresturl = parameter.puseresturl;
    powersupply = parameter.ppowersupply;
    useledtest = parameter.puseledtest;
    usesetwlan = parameter.pusesetwlan;
    useshowip = parameter.puseshowip;
    switchRainBow = parameter.pswitchRainBow;
    switchLangWeb = parameter.pswitchLangWeb;
    switchLEDOrder = parameter.pswitchLEDOrder;
    
     useCustomMinuteOrder = parameter.puseCustomMinuteOrder;
    // Load custom minute order if enabled and valid; otherwise fall back to default mapping
    int tmpOrder[4] = {parameter.pMinuteOrder1, parameter.pMinuteOrder2, parameter.pMinuteOrder3, parameter.pMinuteOrder4};
    bool tmpOk = true;
    for (int k = 0; k < 4; k++) {
      if (tmpOrder[k] < 0 || tmpOrder[k] >= NUMPIXELS) { tmpOk = false; break; }
    }
    if (useCustomMinuteOrder && tmpOk && isUnique4(tmpOrder)) {
      for (int k = 0; k < 4; k++) minuteLedOrder[k] = tmpOrder[k];
    } else {
      useCustomMinuteOrder = 0;
      setDefaultMinuteLedOrderFromSwitch();
    }
        
    blinkTime = parameter.pBlinkTime;
    DEspecial1 = parameter.pDEspecial1;
    dcwFlag = parameter.pDCWFlag;
    useRTC = parameter.puseRTC;
    intensity = parameter.pIntensity;
    intensityNight = parameter.pIntensityNight;
    PING_IP_ADDR1_O1 = parameter.pPING_IP_ADDR1_O1;
    PING_IP_ADDR1_O2 = parameter.pPING_IP_ADDR1_O2;
    PING_IP_ADDR1_O3 = parameter.pPING_IP_ADDR1_O3;
    PING_IP_ADDR1_O4 = parameter.pPING_IP_ADDR1_O4;
    PING_IP_ADDR2_O1 = parameter.pPING_IP_ADDR2_O1;
    PING_IP_ADDR2_O2 = parameter.pPING_IP_ADDR2_O2;
    PING_IP_ADDR2_O3 = parameter.pPING_IP_ADDR2_O3;
    PING_IP_ADDR2_O4 = parameter.pPING_IP_ADDR2_O4;
    PING_IP_ADDR3_O1 = parameter.pPING_IP_ADDR3_O1;
    PING_IP_ADDR3_O2 = parameter.pPING_IP_ADDR3_O2;
    PING_IP_ADDR3_O3 = parameter.pPING_IP_ADDR3_O3;
    PING_IP_ADDR3_O4 = parameter.pPING_IP_ADDR3_O4;
    PING_TIMEOUTNUM = parameter.pPING_TIMEOUTNUM;
    PING_DEBUG_MODE = parameter.pPING_DEBUG_MODE;
    PING_USEMONITOR = parameter.pPING_USEMONITOR;
    String ntp(parameter.pNTPServer);
    ntpServer = ntp;
    String tz(parameter.pTimeZone);
    timeZone = tz;
  } else {
    Serial.println("Checksum does not match. New program version or new installed ESP detected...");
  }

  // Hardware settings (pins), use the defaults from settings.h if none are stored yet:
  EEPROM.get(HW_OFFSET, hw);
  if (hw.magic != HW_MAGIC || !validPin(hw.ledPin) || !validPin(hw.sdaPin) || !validPin(hw.sclPin)) {
    hw.magic = HW_MAGIC;
    hw.ledPin = PIN;
    hw.sdaPin = SDA_PIN;
    hw.sclPin = SCL_PIN;
    hw.rtcUtc = 0;
    hw.reserved = 0;
  }

  // Floating text settings (styles, speeds), use the defaults if none are stored yet:
  EEPROM.get(TXT_OFFSET, txt);
  if (txt.magic != TXT_MAGIC || txt.dateStyle > 2 || txt.tempStyle > 3 || txt.humStyle > 3 ||  /* [V2] 3 = compact + bar */ txt.dateSpeed < 30 || txt.dateSpeed > 600 || txt.tempSpeed < 30 || txt.tempSpeed > 600 || txt.humSpeed < 30 || txt.humSpeed > 600 || txt.holdMs < 300 || txt.holdMs > 6000) txtDefaults();
}


// ###########################################################################################################################################
// # Write current parameter settings to flash:
// ###########################################################################################################################################
void writeEEPROM() {
  //Serial.println("Write parameter into EEPROM");
  parameter.pRed = redVal;
  parameter.pGreen = greenVal;
  parameter.pBlue = blueVal;
  parameter.pIntensity = intensity;
  parameter.pIntensityNight = intensityNight;
  parameter.pDCWFlag = dcwFlag;
  parameter.puseRTC = useRTC;
  parameter.pBlinkTime = blinkTime;
  parameter.pDEspecial1 = DEspecial1;
  ntpServer.toCharArray(parameter.pNTPServer, sizeof(parameter.pNTPServer));
  timeZone.toCharArray(parameter.pTimeZone, sizeof(parameter.pTimeZone));
  parameter.pShowDate = showDate;
  parameter.pShowDateIntervalSec = showDateIntervalSec;
  parameter.pUseHTU21 = useHTU21;
  parameter.pShowTemp = showTemp;
  parameter.pShowTempIntervalSec = showTempIntervalSec;
  parameter.pShowHumidity = showHumidity;
  parameter.pShowHumidityIntervalSec = showHumidityIntervalSec;
  parameter.pUseBootText = useBootText;
  memset(parameter.pBootText1, 0, sizeof(parameter.pBootText1));
  memset(parameter.pBootText2, 0, sizeof(parameter.pBootText2));
  bootText1.toCharArray(parameter.pBootText1, sizeof(parameter.pBootText1));
  bootText2.toCharArray(parameter.pBootText2, sizeof(parameter.pBootText2));
  parameter.pdisplayoff = displayoff;
  parameter.puseNightLEDs = useNightLEDs;
  parameter.pdisplayonmaxMO = displayonmaxMO;
  parameter.pdisplayonminMO = displayonminMO;
  parameter.pdisplayonmaxTU = displayonmaxTU;
  parameter.pdisplayonminTU = displayonminTU;
  parameter.pdisplayonmaxWE = displayonmaxWE;
  parameter.pdisplayonminWE = displayonminWE;
  parameter.pdisplayonmaxTH = displayonmaxTH;
  parameter.pdisplayonminTH = displayonminTH;
  parameter.pdisplayonmaxFR = displayonmaxFR;
  parameter.pdisplayonminFR = displayonminFR;
  parameter.pdisplayonmaxSA = displayonmaxSA;
  parameter.pdisplayonminSA = displayonminSA;
  parameter.pdisplayonmaxSU = displayonmaxSU;
  parameter.pdisplayonminSU = displayonminSU;
  parameter.pwchostnamenum = wchostnamenum;
  parameter.puseupdate = useupdate;
  parameter.puseresturl = useresturl;
  parameter.ppowersupply = powersupply;
  parameter.puseledtest = useledtest;
  parameter.pusesetwlan = usesetwlan;
  parameter.puseshowip = useshowip;
  parameter.pswitchRainBow = switchRainBow;
  parameter.pswitchLangWeb = switchLangWeb;
  parameter.pswitchLEDOrder = switchLEDOrder;
  parameter.puseCustomMinuteOrder = useCustomMinuteOrder;
  parameter.pMinuteOrder1 = minuteLedOrder[0];
  parameter.pMinuteOrder2 = minuteLedOrder[1];
  parameter.pMinuteOrder3 = minuteLedOrder[2];
  parameter.pMinuteOrder4 = minuteLedOrder[3];
  parameter.pPING_IP_ADDR1_O1 = PING_IP_ADDR1_O1;
  parameter.pPING_IP_ADDR1_O2 = PING_IP_ADDR1_O2;
  parameter.pPING_IP_ADDR1_O3 = PING_IP_ADDR1_O3;
  parameter.pPING_IP_ADDR1_O4 = PING_IP_ADDR1_O4;
  parameter.pPING_IP_ADDR2_O1 = PING_IP_ADDR2_O1;
  parameter.pPING_IP_ADDR2_O2 = PING_IP_ADDR2_O2;
  parameter.pPING_IP_ADDR2_O3 = PING_IP_ADDR2_O3;
  parameter.pPING_IP_ADDR2_O4 = PING_IP_ADDR2_O4;
  parameter.pPING_IP_ADDR3_O1 = PING_IP_ADDR3_O1;
  parameter.pPING_IP_ADDR3_O2 = PING_IP_ADDR3_O2;
  parameter.pPING_IP_ADDR3_O3 = PING_IP_ADDR3_O3;
  parameter.pPING_IP_ADDR3_O4 = PING_IP_ADDR3_O4;
  parameter.pPING_TIMEOUTNUM = PING_TIMEOUTNUM;
  parameter.pPING_DEBUG_MODE = PING_DEBUG_MODE;
  parameter.pPING_USEMONITOR = PING_USEMONITOR;

  // calculate checksum
  byte* p = (byte*)(void*)&parameter;
  parameter.pCheckSum = 0;
  for (int L = 0; L < sizeof(parameter) - sizeof(parameter.pCheckSum); ++L) {
    byte b = *p++;
    parameter.pCheckSum = parameter.pCheckSum + b;
  }
  // Write data to EEPROM
  p = (byte*)(void*)&parameter;
  for (int L = 0; L < sizeof(parameter); ++L) {
    byte b = *p++;
    EEPROM.write(L, b);
    // Serial.print("Write FLASH Byte ");
    // Serial.print(L);
    // Serial.print(" = ");
    // Serial.println(b);
  }
  EEPROM.commit();
}


// ###########################################################################################################################################
// # Check for HTML respond from Client logged on to Web page:
// ###########################################################################################################################################
// Answers waiting requests of the configuration page, the REST functions and the WiFi Manager hotspot.
void checkClient() {
  if (portalActive) wifiManager.process();
  if (servicesStarted) server.handleClient();
  if (useresturl && server1) server1->handleClient();
}

// Delay that keeps the configuration page and the REST functions responsive:
void serviceDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    checkClient();
    checkResetButton();  // [FORK] WiFi reset button, polled every ~5 ms
    delay(5);
  } while (millis() - start < ms);
}


// ###########################################################################################################################################
// # Read current date & time from RTC:
// ###########################################################################################################################################
// Sets the clock of the ESP from the RTC, so the WordClock shows the time without WiFi.
bool rtcReadTime() {
  if (!checkRTC()) return false;
  if (rtc.lostPower()) {  // The RTC was without power (battery empty?), so its time is not usable
    if (lastRequest == 0) Serial.println("rtcReadTime() - RTC lost its time (battery empty?) - waiting for NTP");
    rtcTimeValid = false;
    lastRequest = millis() | 1;
    return false;
  }
  uint32_t t = rtc.now().unixtime();
  if (!hw.rtcUtc) {  // Versions before V6.1.0 stored the local time in the RTC: convert it to UTC
    time_t local = (time_t)t;
    struct tm tmLocal;
    gmtime_r(&local, &tmLocal);
    tmLocal.tm_isdst = -1;
    t = (uint32_t)mktime(&tmLocal);
  }
  lastRequest = millis() | 1;
  if ((time_t)t < MIN_VALID_TIME) {
    rtcTimeValid = false;
    return false;
  }
  rtcTimeValid = true;
  timeval tv = { (time_t)t, 0 };
  settimeofday(&tv, nullptr);
  Serial.println("rtcReadTime() - get time from RTC");
  return true;
}


// ###########################################################################################################################################
// # Show the IP-address on the display:
// ###########################################################################################################################################
void showIP() {
  IPAddress ip = WiFi.localIP();
  Serial.print("Displaying IP address ");
  Serial.println(ip);

  for (int x = 11; x > -75; x--) {
    dunkel();
    int offSet = x;
    // Loop over 4 bytes of IP Address
    for (int idx = 0; idx < 4; idx++) {
      uint8_t octet = ip[idx];
      printAt(octet / 100, offSet, 2);
      octet = octet % 100;
      offSet = offSet + 6;
      printAt(octet / 10, offSet, 2);
      offSet = offSet + 6;
      printAt(octet % 10, offSet, 2);
      offSet = offSet + 6;
      // add delimiter between octets
      if (idx < 3) {
        setLED(ledXY(offSet, 2), ledXY(offSet, 2), -1);  //sets point
        offSet++;
      }
    }
    pixels.show();
    delay(150);  // set speed of timeshift
  }
}


// ###########################################################################################################################################
// #  Switch off all LEDs:
// ###########################################################################################################################################
void dunkel() {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));  // Switch off all LEDs
  }
}


// ###########################################################################################################################################
// # Switch on default Text "ES IST" / "IT IS": (DE or EN depending on the set web configuration language)
// ###########################################################################################################################################
void defaultText() {
  if (switchLangWeb == 1) {  // EN
    setLED(9, 10, 1);        // IT
    setLED(6, 7, 1);         // IS
  } else {                   // DE
    setLED(9, 10, 1);        // ES
    setLED(5, 7, 1);         // IST
  }
}


// ###########################################################################################################################################
// # Convert x/y coordinates into LED number return -1 for invalid coordinate:
// ###########################################################################################################################################
int ledXY(int x, int y) {
  // Test for valid coordinates
  // If outside panel return -1
  if ((x < 0) || (x > 10) || (y < 0) || (y > 9))
    return -1;
  int ledNr = (9 - y) * 11;
  if ((y % 2) == 0)
    ledNr = ledNr + x;
  else
    ledNr = ledNr + 10 - x;
  return ledNr;
}


// ###########################################################################################################################################
// # Sets, where the numbers from 1 to 9 are printed:
// ###########################################################################################################################################
void printAt(int ziffer, int x, int y) {
  switch (ziffer) {
    case 0:                                 //number 0
      setLEDLine(x + 1, x + 3, y + 6, -1);  //-1 is true, so switchOn
      for (int yd = 1; yd < 6; yd++) {
        setLED(ledXY(x, y + yd), ledXY(x, y + yd), -1);
        setLED(ledXY(x + 4, y + yd), ledXY(x + 4, y + yd), -1);
      }
      setLEDLine(x + 1, x + 3, y, -1);
      break;

    case 1:  //number 1
      setLED(ledXY(x + 3, y + 5), ledXY(x + 3, y + 5), -1);
      setLED(ledXY(x + 2, y + 4), ledXY(x + 2, y + 4), -1);
      for (int yd = 0; yd <= 6; yd++) {
        setLED(ledXY(x + 4, y + yd), ledXY(x + 4, y + yd), -1);
      }
      break;

    case 2:  //number 2
      for (int d = 1; d <= 4; d++) {
        setLED(ledXY(x + d, y + d), ledXY(x + d, y + d), -1);
      }
      setLEDLine(x, x + 4, y, -1);
      setLED(ledXY(x, y + 5), ledXY(x, y + 5), -1);
      setLED(ledXY(x + 4, y + 5), ledXY(x + 4, y + 5), -1);
      setLEDLine(x + 1, x + 3, y + 6, -1);
      break;

    case 3:  //number 3
      for (int yd = 1; yd <= 2; yd++) {
        setLED(ledXY(x + 4, y + yd + 3), ledXY(x + 4, y + yd + 3), -1);
        setLED(ledXY(x + 4, y + yd), ledXY(x + 4, y + yd), -1);
      }
      for (int yd = 0; yd < 7; yd = yd + 3) {
        setLEDLine(x + 1, x + 3, y + yd, -1);
      }
      setLED(ledXY(x, y + 1), ledXY(x, y + 1), -1);
      setLED(ledXY(x, y + 5), ledXY(x, y + 5), -1);
      break;

    case 4:  //number 4
      for (int d = 0; d <= 3; d++) {
        setLED(ledXY(x + d, y + d + 3), ledXY(x + d, y + d + 3), -1);
      }
      for (int yd = 0; yd <= 3; yd++) {
        setLED(ledXY(x + 3, y + yd), ledXY(x + 3, y + yd), -1);
      }
      setLEDLine(x, x + 4, y + 2, -1);
      break;

    case 5:  //number 5
      setLEDLine(x, x + 4, y + 6, -1);
      setLED(ledXY(x, y + 5), ledXY(x, y + 5), -1);
      setLED(ledXY(x, y + 4), ledXY(x, y + 4), -1);
      setLEDLine(x, x + 3, y + 3, -1);
      setLED(ledXY(x + 4, y + 2), ledXY(x + 4, y + 2), -1);
      setLED(ledXY(x + 4, y + 1), ledXY(x + 4, y + 1), -1);
      setLEDLine(x, x + 3, y, -1);
      break;

    case 6:  //number 6
      for (int d = 0; d <= 3; d++) {
        setLED(ledXY(x + d, y + d + 3), ledXY(x + d, y + d + 3), -1);
      }
      for (int yd = 0; yd < 4; yd = yd + 3) {
        setLEDLine(x + 1, x + 3, y + yd, -1);
      }
      setLED(ledXY(x, y + 1), ledXY(x, y + 1), -1);
      setLED(ledXY(x, y + 2), ledXY(x, y + 2), -1);
      setLED(ledXY(x + 4, y + 1), ledXY(x + 4, y + 1), -1);
      setLED(ledXY(x + 4, y + 2), ledXY(x + 4, y + 2), -1);
      break;

    case 7:  //number 7
      for (int yd = 0; yd <= 6; yd++) {
        setLED(ledXY(x + 3, y + yd), ledXY(x + 3, y + yd), -1);
      }
      setLEDLine(x + 1, x + 4, y + 3, -1);
      setLEDLine(x, x + 3, y + 6, -1);
      break;

    case 8:  //number 8
      for (int yd = 1; yd <= 2; yd++) {
        setLED(ledXY(x + 4, y + yd + 3), ledXY(x + 4, y + yd + 3), -1);
        setLED(ledXY(x + 4, y + yd), ledXY(x + 4, y + yd), -1);
        setLED(ledXY(x, y + yd + 3), ledXY(x, y + yd + 3), -1);
        setLED(ledXY(x, y + yd), ledXY(x, y + yd), -1);
      }
      for (int yd = 0; yd < 7; yd = yd + 3) {
        setLEDLine(x + 1, x + 3, y + yd, -1);
      }
      break;

    case 9:  //number 9
      for (int d = 0; d <= 3; d++) {
        setLED(ledXY(x + d + 1, y + d), ledXY(x + d + 1, y + d), -1);
      }
      for (int yd = 4; yd <= 5; yd++) {
        setLED(ledXY(x, y + yd), ledXY(x, y + yd), -1);
        setLED(ledXY(x + 4, y + yd), ledXY(x + 4, y + yd), -1);
      }
      setLEDLine(x + 1, x + 3, y + 6, -1);
      setLEDLine(x + 1, x + 4, y + 3, -1);
      break;
  }
}


// ###########################################################################################################################################
// # Turns on the outer four LEDs (one per minute):
// ###########################################################################################################################################
void showMinutes(int minutes) {
  int minMod = (minutes % 5);

  // If custom is disabled, ensure the default mapping matches the selected direction
  if (!useCustomMinuteOrder) setDefaultMinuteLedOrderFromSwitch();

  for (int i = 1; i < 5; i++) {
    int ledNr = minuteLedOrder[i - 1];

    if (minMod < i)
      pixels.setPixelColor(ledNr, pixels.Color(0, 0, 0));
    else
      pixels.setPixelColor(ledNr, pixels.Color(redVal, greenVal, blueVal));
  }
}


// ###########################################################################################################################################
// # 5x7 pixel font (classic column-major GLCD font layout), one entry per supported letter - each letter is 5 columns wide,
// # bit 0 of a column byte is the TOP pixel of the letter and bit 6 is the BOTTOM pixel (matching the y+6=top / y=bottom
// # convention already used by printAt() above). Only the letters actually needed for the boot texts are defined; add more
// # rows here (and in getGlyph() below) if you want to scroll other letters:
// ###########################################################################################################################################
const byte fontA[5] = { 0x7E, 0x11, 0x11, 0x11, 0x7E };
const byte fontB[5] = { 0x7F, 0x49, 0x49, 0x49, 0x36 };
const byte fontC[5] = { 0x3E, 0x41, 0x41, 0x41, 0x22 };
const byte fontD[5] = { 0x7F, 0x41, 0x41, 0x41, 0x3E };
const byte fontE[5] = { 0x7F, 0x49, 0x49, 0x49, 0x41 };
const byte fontF[5] = { 0x7F, 0x09, 0x09, 0x09, 0x01 };
const byte fontG[5] = { 0x3E, 0x41, 0x49, 0x49, 0x7A };
const byte fontH[5] = { 0x7F, 0x08, 0x08, 0x08, 0x7F };
const byte fontI[5] = { 0x00, 0x41, 0x7F, 0x41, 0x00 };
const byte fontJ[5] = { 0x20, 0x40, 0x41, 0x3F, 0x01 };
const byte fontK[5] = { 0x7F, 0x08, 0x14, 0x22, 0x41 };
const byte fontL[5] = { 0x7F, 0x40, 0x40, 0x40, 0x40 };
const byte fontM[5] = { 0x7F, 0x02, 0x0C, 0x02, 0x7F };
const byte fontN[5] = { 0x7F, 0x04, 0x08, 0x10, 0x7F };
const byte fontO[5] = { 0x3E, 0x41, 0x41, 0x41, 0x3E };
const byte fontP[5] = { 0x7F, 0x09, 0x09, 0x09, 0x06 };
const byte fontQ[5] = { 0x3E, 0x41, 0x51, 0x21, 0x5E };
const byte fontR[5] = { 0x7F, 0x09, 0x19, 0x29, 0x46 };
const byte fontS[5] = { 0x46, 0x49, 0x49, 0x49, 0x31 };
const byte fontT[5] = { 0x01, 0x01, 0x7F, 0x01, 0x01 };
const byte fontU[5] = { 0x3F, 0x40, 0x40, 0x40, 0x3F };
const byte fontV[5] = { 0x1F, 0x20, 0x40, 0x20, 0x1F };
const byte fontW[5] = { 0x3F, 0x40, 0x38, 0x40, 0x3F };
const byte fontX[5] = { 0x63, 0x14, 0x08, 0x14, 0x63 };
const byte fontY[5] = { 0x07, 0x08, 0x70, 0x08, 0x07 };
const byte fontZ[5] = { 0x43, 0x45, 0x49, 0x51, 0x61 };
const byte fontPercent[5] = { 0x23, 0x13, 0x08, 0x64, 0x62 };  // '%' symbol, used for the humidity display


// ###########################################################################################################################################
// # Looks up the 5x7 glyph for a letter. Returns nullptr for a space (or any letter that is not defined above), which simply
// # leaves that character's column blank when scrolling:
// ###########################################################################################################################################
const byte* getGlyph(char c) {
  switch (c) {
    case 'A': return fontA;
    case 'B': return fontB;
    case 'C': return fontC;
    case 'D': return fontD;
    case 'E': return fontE;
    case 'F': return fontF;
    case 'G': return fontG;
    case 'H': return fontH;
    case 'I': return fontI;
    case 'J': return fontJ;
    case 'K': return fontK;
    case 'L': return fontL;
    case 'M': return fontM;
    case 'N': return fontN;
    case 'O': return fontO;
    case 'P': return fontP;
    case 'Q': return fontQ;
    case 'R': return fontR;
    case 'S': return fontS;
    case 'T': return fontT;
    case 'U': return fontU;
    case 'V': return fontV;
    case 'W': return fontW;
    case 'X': return fontX;
    case 'Y': return fontY;
    case 'Z': return fontZ;
    default: return nullptr;  // space / unsupported character (accents, digits, punctuation, ...) -> blank column
  }
}


// ###########################################################################################################################################
// # Draws one 5x7 font letter with its bottom-left corner at (x, y):
// ###########################################################################################################################################
void printLetterAt(const byte* colData, int x, int y) {
  if (colData == nullptr) return;  // space -> nothing to draw
  for (int col = 0; col < 5; col++) {
    byte bits = colData[col];
    for (int row = 0; row < 7; row++) {
      if (bits & (1 << row)) {
        setLED(ledXY(x + col, y + 6 - row), ledXY(x + col, y + 6 - row), -1);
      }
    }
  }
}


// ###########################################################################################################################################
// # Scrolls an arbitrary uppercase text (letters A-Z and spaces) across the display once, using the same scrolling technique
// # as showIP() / showCurrentDate(). Unsupported characters are simply shown as a blank column - see getGlyph() above:
// ###########################################################################################################################################
void showScrollText(const char* text) {
  const int charWidth = 6;  // 5 pixel wide letter + 1 pixel gap
  const int numChars = strlen(text);
  const int totalWidth = numChars * charWidth;

  for (int x = 11; x > -(totalWidth + 11); x--) {
    dunkel();
    for (int i = 0; i < numChars; i++) {
      printLetterAt(getGlyph(text[i]), x + i * charWidth, 2);
    }
    pixels.show();
    delay(80);  // set speed of timeshift
  }
  dunkel();
  pixels.show();
}


// ###########################################################################################################################################
// # Shows the two configurable scrolling boot texts (bootText1 and bootText2 - settable on the web configuration page) once at startup:
// ###########################################################################################################################################
void showBootTexts() {
  if (!useBootText) return;

  if (bootText1.length() > 0) {
    String bt1 = bootText1;
    bt1.toUpperCase();  // font only supports uppercase letters
    Serial.println("Show boot text 1: " + bt1);
    showScrollText(bt1.c_str());
  }

  if (bootText2.length() > 0) {
    String bt2 = bootText2;
    bt2.toUpperCase();  // font only supports uppercase letters
    Serial.println("Show boot text 2: " + bt2);
    showScrollText(bt2.c_str());
  }
}


// ###########################################################################################################################################
// # Sets, where the special symbols (C, H, minus sign, decimal point) for the sensor display are printed - same 5x7 grid as printAt():
// ###########################################################################################################################################
void printSymbolAt(char symbol, int x, int y) {
  switch (symbol) {
    case 'C':  // Celsius - like a "0" digit but open on the right side
      setLEDLine(x + 1, x + 3, y + 6, -1);
      for (int yd = 1; yd <= 5; yd++) {
        setLED(ledXY(x, y + yd), ledXY(x, y + yd), -1);
      }
      setLEDLine(x + 1, x + 3, y, -1);
      break;

    case '%':  // Percent (humidity)
      printLetterAt(fontPercent, x, y);
      break;

    case '-':  // Minus sign (negative temperature)
      setLEDLine(x, x + 4, y + 3, -1);
      break;

    case '.':  // Decimal point
      setLED(ledXY(x, y), ledXY(x, y), -1);
      break;
  }
}


// ###########################################################################################################################################
// # [TEXT STYLES] Floating texts (date / temperature / humidity) with selectable style, speed and hold time.
// # Style 0 = scroll (as before), 1 = scroll with stops (every group scrolls in, stops, scrolls out),
// # 2 = fixed pages (date: day / month as pages; temperature & humidity: COMPACT = small 3x5 number + small unit below) [V2],
// # 3 = (temperature & humidity only) COMPACT + COLOUR BAR in the two left columns [V2].
// # The settings are configured on the web page (tab "Floating texts") and stored behind the hardware record (txtRec).
// # The same drawing logic exists in web/index.html (preview) - keep both in sync.
// ###########################################################################################################################################
const int TXT_VIS = 11;  // visible columns of the panel

// Advance (columns) per character: digits and symbols are 5 wide + 1 gap. The decimal point sits in the gap of the character before it.
int txtAdv(char c) {
  if (c == '.') return 1;
  if (c == 'C') return 5;
  return 6;
}

int textWidth(const char* s) {
  int w = 0;
  for (; *s; s++) w += txtAdv(*s);
  return (w > 0) ? w - 1 : 0;
}

// Draws a text of digits, '.', '-', 'C' and '%' with its bottom-left corner at (x, y). Parts outside the panel are ignored.
void drawText(const char* s, int x, int y) {
  for (; *s; s++) {
    char c = *s;
    if (c >= '0' && c <= '9') printAt(c - '0', x, y);
    else if (c == '.') printSymbolAt('.', x - 1, y);
    else printSymbolAt(c, x, y);
    x += txtAdv(c);
  }
}

void txtFrame(const char* s, int x, int y) {
  dunkel();
  drawText(s, x, y);
  pixels.show();
}

void txtBlank() {
  dunkel();
  pixels.show();
}

// Scrolls a text from the right edge completely out to the left:
void txtScrollThrough(const char* s, int speed) {
  int w = textWidth(s);
  for (int x = TXT_VIS - 1; x >= -w; x--) {
    txtFrame(s, x, 2);
    delay(speed);
  }
}

// One group of a text. style 1: scrolls in, stops (hold time), scrolls out. style 2: appears, stays (hold time), disappears.
// A group wider than the panel (f.e. the year) cannot stop and scrolls through at the set speed.
void txtShowSegment(const char* s, int style, int speed) {
  int w = textWidth(s);
  if (w > TXT_VIS) {
    txtScrollThrough(s, speed);
    return;
  }
  int x0 = (TXT_VIS - w) / 2;
  if (style == 1) {
    for (int x = TXT_VIS - 1; x > x0; x--) {
      txtFrame(s, x, 2);
      delay(speed);
    }
  }
  txtFrame(s, x0, 2);
  delay(txt.holdMs);
  if (style == 1) {
    for (int x = x0 - 1; x >= -w; x--) {
      txtFrame(s, x, 2);
      delay(speed);
    }
  } else {
    txtBlank();
    delay(150);
  }
}

// ---------------------------------------------------------------------------------------------------------------------------------------------
// [V2] COMPACT layout (styles 2 and 3) for temperature / humidity.
// Panel = 11 columns (x 0..10) x 10 rows (y 0..9, y = 0 is the bottom row).
//   rows 5..9 : number in a small 3x5 font (3 columns per character + 1 gap column)
//   row  4    : gap
//   rows 1..3 : small 5x3 unit symbol (degree + C, or %)
//   style 3   : columns 0..1 = colour bar (whole height), column 2 = gap, number + unit centred in columns 3..10
// ---------------------------------------------------------------------------------------------------------------------------------------------

// [V2] Range of the temperature bar in degrees C (bar empty-ish / blue at MIN or below, full / red at MAX or above):
#define TXT_BAR_TEMP_MIN 10.0f
#define TXT_BAR_TEMP_MAX 35.0f

// [V2] 3x5 font: 5 rows (top first), 3 bits per row (bit 2 = left column). Index 0..9 = digits, 10 = minus sign.
const uint8_t TXT_FONT3X5[11][5] = {
  { 0b111, 0b101, 0b101, 0b101, 0b111 },  // 0
  { 0b010, 0b110, 0b010, 0b010, 0b111 },  // 1
  { 0b111, 0b001, 0b111, 0b100, 0b111 },  // 2
  { 0b111, 0b001, 0b111, 0b001, 0b111 },  // 3
  { 0b101, 0b101, 0b111, 0b001, 0b001 },  // 4
  { 0b111, 0b100, 0b111, 0b001, 0b111 },  // 5
  { 0b111, 0b100, 0b111, 0b101, 0b111 },  // 6
  { 0b111, 0b001, 0b001, 0b010, 0b010 },  // 7
  { 0b111, 0b101, 0b111, 0b101, 0b111 },  // 8
  { 0b111, 0b101, 0b111, 0b001, 0b111 },  // 9
  { 0b000, 0b000, 0b111, 0b000, 0b000 }   // -
};

// [V2] Width in columns of a text in the 3x5 font (every character is 3 wide, 1 gap column between them):
int txtSmallWidth(const char* s) {
  int n = strlen(s);
  return (n > 0) ? n * 4 - 1 : 0;
}

// [V2] Draws a text of digits / '-' in the 3x5 font. x = left column, yTop = top row of the characters:
void txtDrawSmall(const char* s, int x, int yTop) {
  for (; *s; s++, x += 4) {
    int idx = -1;
    if (*s >= '0' && *s <= '9') idx = *s - '0';
    else if (*s == '-') idx = 10;
    if (idx < 0) continue;
    for (int r = 0; r < 5; r++) {
      for (int c = 0; c < 3; c++) {
        if (TXT_FONT3X5[idx][r] & (4 >> c)) {
          int led = ledXY(x + c, yTop - r);
          setLED(led, led, -1);
        }
      }
    }
  }
}

// Small 5x3 unit symbols (the old version always drew them at x = 3, y = 0). [V2] now with a free position:
// x0 = left column, yBottom = bottom row of the symbol (it is 3 rows high). Top row first in the patterns:
void txtSmallUnit(char unit, int x0, int yBottom) {
  static const char* const pct[3] = { "XX..X", "..X..", "X..XX" };
  static const char* const deg[3] = { "X.XXX", "..X..", "..XXX" };
  const char* const* g = (unit == '%') ? pct : deg;
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 5; c++) {
      if (g[r][c] == 'X') setLED(ledXY(x0 + c, yBottom + 2 - r), ledXY(x0 + c, yBottom + 2 - r), -1);
    }
  }
}

// [V2] Colour of the bar. Temperature: blue (cold) -> green (normal, middle of the range) -> red (hot).
// Humidity: orange (too dry, < 30 %) -> green (comfortable, 40..60 %) -> blue (too humid, > 70 %).
// hueDeg is the colour wheel angle: 0 = red, 30 = orange, 120 = green, 240 = blue.
uint32_t txtBarColor(bool isTemp, float v) {
  float hueDeg;
  if (isTemp) {
    float frac = (v - TXT_BAR_TEMP_MIN) / (TXT_BAR_TEMP_MAX - TXT_BAR_TEMP_MIN);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    hueDeg = 240.0f * (1.0f - frac);
  } else {
    if (v <= 30.0f) hueDeg = 30.0f;
    else if (v < 40.0f) hueDeg = 30.0f + (v - 30.0f) * 9.0f;    // 30 -> 120 over 10 %
    else if (v <= 60.0f) hueDeg = 120.0f;
    else if (v < 70.0f) hueDeg = 120.0f + (v - 60.0f) * 11.0f;  // 120 -> 230 over 10 %
    else hueDeg = 230.0f;
  }
  uint16_t hue = (uint16_t)(hueDeg * 65535.0f / 360.0f);
  return pixels.gamma32(pixels.ColorHSV(hue, 255, 255));  // same colour handling as the rainbow mode
}

// [V2] Draws the bar into the two left columns: height 1..10 rows from the bottom (temperature: MIN..MAX, humidity: 0..100 %).
void txtDrawBar(bool isTemp, float v) {
  float frac = isTemp ? (v - TXT_BAR_TEMP_MIN) / (TXT_BAR_TEMP_MAX - TXT_BAR_TEMP_MIN) : v / 100.0f;
  if (frac < 0.0f) frac = 0.0f;
  if (frac > 1.0f) frac = 1.0f;
  int h = (int)lroundf(frac * 10.0f);
  if (h < 1) h = 1;  // always show at least one row, so the colour is visible
  uint32_t color = txtBarColor(isTemp, v);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < 2; x++) {
      int led = ledXY(x, y);
      if (led >= 0) pixels.setPixelColor(led, color);
    }
  }
}

// [V2] Compact layout, replaces txtShowTwoLine(). Returns false if the number does not fit (then the caller scrolls it).
// raw = the value for the bar (temperature in C, humidity in %).
bool txtShowCompact(const char* num, char unit, bool withBar, float raw) {
  int w = txtSmallWidth(num);
  int areaX = withBar ? 3 : 0;   // first column of the area for number + unit
  int areaW = withBar ? 8 : 11;  // columns of that area
  if (w > areaW && withBar) {    // 3 characters do not fit next to the bar: layout without bar
    withBar = false;
    areaX = 0;
    areaW = 11;
  }
  if (w > areaW) return false;
  dunkel();
  txtDrawSmall(num, areaX + (areaW - w) / 2, 9);        // digits in rows 5..9
  txtSmallUnit(unit, areaX + (areaW - 5) / 2, 1);       // unit in rows 1..3
  if (withBar) txtDrawBar(unit == 'C', raw);
  pixels.show();
  delay(txt.holdMs);
  txtBlank();
  return true;
}

// Temperature / humidity: numScroll = number for the scroll styles (may have a decimal), numInt = integer for the compact styles,
// [V2] raw = value for the colour bar (unit 'C' = temperature in C, otherwise humidity in %)
void txtShowValue(int style, int speed, const char* numScroll, const char* numInt, char unit, float raw) {
  if (style == 2 || style == 3) {  // [V2] 2 = compact, 3 = compact + bar
    if (txtShowCompact(numInt, unit, style == 3, raw)) return;
    style = 1;  // does not fit: fall back to scroll with stops
  }
  if (style == 1) {
    char u[2] = { unit, 0 };
    txtShowSegment(numScroll, 1, speed);
    txtShowSegment(u, 1, speed);
    return;
  }
  char buf[16];
  snprintf(buf, sizeof(buf), "%s%c", numScroll, unit);
  txtScrollThrough(buf, speed);
}

// ###########################################################################################################################################
// # Show current date on clock:
// ###########################################################################################################################################
void showCurrentDate() {
  int year = iYear;
  if (year < 1000) year += 2000;
  int style = txt.dateStyle;
  int speed = txt.dateSpeed;
  char dd[4], mm[4], yy[8];
  snprintf(dd, sizeof(dd), "%02d", iDay % 100);
  snprintf(mm, sizeof(mm), "%02d", iMonth % 100);
  snprintf(yy, sizeof(yy), "%04d", year % 10000);
  if (style == 0) {
    char buf[24];
    if (txt.dateYear) snprintf(buf, sizeof(buf), "%s.%s.%s", dd, mm, yy);
    else snprintf(buf, sizeof(buf), "%s.%s.", dd, mm);
    txtScrollThrough(buf, speed);
  } else {
    txtShowSegment(dd, style, speed);
    txtShowSegment(mm, style, speed);
    if (txt.dateYear) txtShowSegment(yy, style, speed);
  }
  txtBlank();
}


// ###########################################################################################################################################
// # Show current temperature on clock - reads the GY-21 / HTU21D sensor once before showing:
// ###########################################################################################################################################
void showCurrentTemp() {
  if (!checkHTU()) return;  // Sensor not available - skip silently

  float t = htu.readTemperature();
  if (isnan(t)) return;  // Faulty reading - skip this cycle, will retry next interval

  Serial.print("Showing current temperature: ");
  Serial.print(t);
  Serial.println(" C");

  int tx10 = (int)lroundf(t * 10.0f);
  int tInt = (int)lroundf(t);
  char numInt[8], numDec[12];
  snprintf(numInt, sizeof(numInt), "%d", tInt);
  snprintf(numDec, sizeof(numDec), "%s%d.%d", tx10 < 0 ? "-" : "", abs(tx10) / 10, abs(tx10) % 10);
  txtShowValue(txt.tempStyle, txt.tempSpeed, txt.tempDec ? numDec : numInt, numInt, 'C', t);  // [V2] + raw value for the bar
  txtBlank();
}


// ###########################################################################################################################################
// # Show current humidity on clock - reads the GY-21 / HTU21D sensor once before showing:
// ###########################################################################################################################################
void showCurrentHumidity() {
  if (!checkHTU()) return;  // Sensor not available - skip silently

  float h = htu.readHumidity();
  if (isnan(h)) return;  // Faulty reading - skip this cycle, will retry next interval

  Serial.print("Showing current humidity: ");
  Serial.print(h);
  Serial.println(" %");

  int humInt = (int)(h + 0.5f);
  if (humInt > 100) humInt = 100;
  if (humInt < 0) humInt = 0;
  char num[8];
  snprintf(num, sizeof(num), "%d", humInt);
  txtShowValue(txt.humStyle, txt.humSpeed, num, num, '%', (float)humInt);  // [V2] + raw value for the bar
  txtBlank();
}


// ###########################################################################################################################################
// # Initialize & check the GY-21 / HTU21D temperature and humidity sensor (I2C, shares the bus with the RTC).
// # Keeps retrying htu.begin() (at most every 5 seconds) until it succeeds once - so fixing the wiring takes effect without a restart:
// ###########################################################################################################################################
int checkHTU() {
  static unsigned long lastTry = 0;
  if (!htuStarted && (lastTry == 0 || millis() - lastTry > 5000)) {
    lastTry = millis();
    if (htu.begin()) {
      htuStarted = -1;
      useHTU21 = 1;
    } else {
      useHTU21 = 0;
      Serial.println("Couldn't find GY-21 / HTU21D sensor! Will retry automatically.");
    }
    Wire.begin(hw.sdaPin, hw.sclPin);  // htu.begin() starts the bus with the default pins - restore the configured ones (RTC)
  }
  return htuStarted;
}


// ###########################################################################################################################################
// # Display current time: (DE or EN depending on the set web configuration language)
// ###########################################################################################################################################
void showCurrentTime() {
  dunkel();           // switch off all LEDs
  rainbowWordNr = 0;  // RainBow variant 1: count the words to give each its own random color
  defaultText();      // Switch on "ES IST" / "IT IS"

  // TEST TIMES:
  // iHour = 9;
  // iMinute = 15;

  // divide minute by 5 to get value for display control
  int minDiv = iMinute / 5;

  switch (switchLangWeb) {
    case 0:  // DE
    default:  // Spanish: same LED layout as German
      {
        // Fuenf: (Minuten)
        setLED(0, 3, ((minDiv == 1) || (minDiv == 5) || (minDiv == 7) || (minDiv == 11)));
        // Viertel:
        if (DEspecial1 == 0) setLED(22, 28, ((minDiv == 3) || (minDiv == 9)));
        if (DEspecial1 == 1) setLED(22, 28, ((minDiv == 3)));
        // DREIVIERTEL:
        if (DEspecial1 == 1) setLED(22, 32, ((minDiv == 9)));
        // Zehn: (Minuten)
        setLED(11, 14, ((minDiv == 2) || (minDiv == 10)));
        // Zwanzig:
        setLED(15, 21, ((minDiv == 4) || (minDiv == 8)));
        // Nach:
        if (DEspecial1 == 0) setLED(40, 43, ((minDiv == 1) || (minDiv == 2) || (minDiv == 3) || (minDiv == 4) || (minDiv == 7)));
        if (DEspecial1 == 1) {
          setLED(40, 43, ((minDiv == 1) || (minDiv == 2) || (minDiv == 4) || (minDiv == 7)));
          if (minDiv == 3) iHour = iHour + 1;
        }
        // Vor:
        if (DEspecial1 == 0) setLED(33, 35, ((minDiv == 5) || (minDiv == 8) || (minDiv == 9) || (minDiv == 10) || (minDiv == 11)));
        if (DEspecial1 == 1) setLED(33, 35, ((minDiv == 5) || (minDiv == 8) || (minDiv == 10) || (minDiv == 11)));
        // Halb:
        setLED(51, 54, ((minDiv == 5) || (minDiv == 6) || (minDiv == 7)));
        // Eck-LEDs: 1 pro Minute
        showMinutes(iMinute);

        //set hour from 1 to 12 (at noon, or midnight)
        int xHour = (iHour % 12);
        if (xHour == 0)
          xHour = 12;
        // at minute 25 hour needs to be counted up:
        // fuenf vor halb 2 = 13:25
        if (iMinute >= 25) {
          if (xHour == 12)
            xHour = 1;
          else
            xHour++;
        }

        // Uhr:
        setLED(107, 109, (iMinute < 5));
        // Ein / Eins: "EINS" except at x:00 ("ES IST EIN UHR"), set as one word so it has one color in the rainbow mode
        setLEDHour(55, (iMinute > 4) ? 58 : 57, (xHour == 1));
        // Zwei:
        setLEDHour(62, 65, (xHour == 2));
        // Drei:
        setLEDHour(73, 76, (xHour == 3));
        // Vier:
        setLEDHour(66, 69, (xHour == 4));
        // Fuenf:
        setLEDHour(44, 47, (xHour == 5));
        // Sechs:
        setLEDHour(77, 81, (xHour == 6));
        // Sieben:
        setLEDHour(93, 98, (xHour == 7));
        // Acht:
        setLEDHour(84, 87, (xHour == 8));
        // Neun:
        setLEDHour(102, 105, (xHour == 9));
        // Zehn: (Stunden)
        setLEDHour(99, 102, (xHour == 10));
        // Elf:
        setLEDHour(47, 49, (xHour == 11));
        // Zwoelf:
        setLEDHour(88, 92, (xHour == 12));
        break;
      }
    case 1:  // EN
      {
        // FIVE: (Minutes)            // x:05 + x:25 + x:35 + x:55
        setLED(23, 26, ((minDiv == 1) || (minDiv == 5) || (minDiv == 7) || (minDiv == 11)));
        // QUARTER:                   // x:15 + X:45
        setLED(13, 19, ((minDiv == 3) || (minDiv == 9)));
        // TEN: (Minutes)             // x:10 + x:50
        setLED(38, 40, ((minDiv == 2) || (minDiv == 10)));
        // TWENTY:                    // x:20 + x:25 + x:35 + x:40
        setLED(27, 32, ((minDiv == 4) || (minDiv == 5) || (minDiv == 7) || (minDiv == 8)));
        // PAST:                      // x:05 + x:10 + x:15 + x:20 + x:25 + x:30
        setLED(51, 54, ((minDiv == 1) || (minDiv == 2) || (minDiv == 3) || (minDiv == 4) || (minDiv == 5) || (minDiv == 6)));
        // TO:                        // x:35 + x:40 + x:45 + x:50 + x:55
        setLED(42, 43, ((minDiv == 7) || (minDiv == 8) || (minDiv == 9) || (minDiv == 10) || (minDiv == 11)));
        // HALF:                      // x:30
        setLED(33, 36, ((minDiv == 6)));
        // A:                         // x:15 + X:45
        setLED(3, 3, ((minDiv == 3) || (minDiv == 9)));

        // CORNER-LEDs: 1 per minute
        showMinutes(iMinute);

        //set hour from 1 to 12 (at noon, or midnight)
        int xHour = (iHour % 12);
        if (xHour == 0)
          xHour = 12;
        // at minute 35 hour needs to be counted up:
        if (iMinute >= 35) {
          if (xHour == 12)
            xHour = 1;
          else
            xHour++;
        }

        // O'CLOCK:
        setLED(104, 109, (iMinute < 5));
        // ONE:
        setLEDHour(55, 57, (xHour == 1));
        // TWO:
        setLEDHour(66, 68, (xHour == 2));
        // THREE:
        setLEDHour(61, 65, (xHour == 3));
        // FOUR:
        setLEDHour(73, 76, (xHour == 4));
        // FIVE:
        setLEDHour(69, 72, (xHour == 5));
        // SIX:
        setLEDHour(58, 60, (xHour == 6));
        // SEVEN:
        setLEDHour(94, 98, (xHour == 7));
        // EIGHT:
        setLEDHour(77, 81, (xHour == 8));
        // NINE:
        setLEDHour(44, 47, (xHour == 9));
        // TEN: (Hour)
        setLEDHour(99, 101, (xHour == 10));
        // ELEVEN:
        setLEDHour(82, 87, (xHour == 11));
        // TWELVE:
        setLEDHour(88, 93, (xHour == 12));
        break;
      }
  }
  rainbowWordNr = -1;
}


// ###########################################################################################################################################
// # Function to write / set the clock:
// ###########################################################################################################################################
// Writes the current time of the ESP to the RTC. It is stored as UTC, so the daylight saving time works without WiFi too.
void rtcWriteTime() {
  if (!checkRTC()) return;
  time_t now = time(nullptr);
  if (now < MIN_VALID_TIME) return;
  rtc.adjust(DateTime((uint32_t)now));
  rtcTimeValid = true;
  if (!hw.rtcUtc) {  // From now on the RTC holds UTC
    hw.rtcUtc = 1;
    writeHwConfig();
  }
}


// ###########################################################################################################################################
// # Handle the time from the NTP server and write it to the RTC:
// ###########################################################################################################################################
// - The clock of the ESP is the only time source for the display (local time from the time zone setting).
// - At startup it is set from the RTC (rtcReadTime), so the WordClock shows the time without WiFi.
// - After each NTP sync the RTC is set to the NTP time (rtcWriteTime).
// - While NTP is not available, the clock of the ESP is aligned to the more accurate RTC once per hour.
#if defined(ESP32)
void ntpSyncCallback(struct timeval* tv) {  // Called by the ESP32 after each NTP sync
  ntpSyncPending = true;
}
#endif

void handleTime() {
  if (ntpSyncPending) {  // New time from the NTP server: write it to the RTC
    ntpSyncPending = false;
    lastNtpSync = millis() | 1;
    rtcWriteTime();
  }
  bool ntpFresh = lastNtpSync != 0 && millis() - lastNtpSync < 2UL * 3600UL * 1000UL;
  if (!ntpFresh && rtcTimeValid && millis() - lastRequest > 3600UL * 1000UL) rtcReadTime();

  time_t now;
  time(&now);  // read the current time
  timeValid = now >= MIN_VALID_TIME;
  if (!timeValid) return;
  struct tm ti;
  localtime_r(&now, &ti);
  iYear = ti.tm_year + 1900;
  iMonth = ti.tm_mon + 1;
  iDay = ti.tm_mday;
  iHour = ti.tm_hour;
  iMinute = ti.tm_min;
  iSecond = ti.tm_sec;
  iWeekDay = ti.tm_wday;  // Before V6.1.0 the weekday was only set with an RTC, so the night mode used Sunday without one
}


// ###########################################################################################################################################
// # Every Hour blink orange DCW - if switched on:
// ###########################################################################################################################################
void showDCW() {
  if ((dcwFlag) && (iMinute == 0) && (iSecond < 10)) {
    if ((iSecond % 2) == 0) {
      pixels.setPixelColor(59, pixels.Color(255, 128, 0));
      pixels.setPixelColor(60, pixels.Color(255, 128, 0));
      pixels.setPixelColor(61, pixels.Color(255, 128, 0));
    } else {
      pixels.setPixelColor(59, pixels.Color(0, 0, 0));
      pixels.setPixelColor(60, pixels.Color(0, 0, 0));
      pixels.setPixelColor(61, pixels.Color(0, 0, 0));
    }
  }
}


// ###########################################################################################################################################
// # Display the time:
// ###########################################################################################################################################
void ShowTheTime() {
  // Date / temperature / humidity are each shown on their own timer (millis() based, ">=" comparison), so a screen is never
  // skipped even though every animation blocks the loop for several seconds.
  static bool autoShowInit = false;
  static unsigned long nextDateShowMillis = 0;
  static unsigned long nextTempShowMillis = 0;
  static unsigned long nextHumidityShowMillis = 0;
  if (!autoShowInit) {
    autoShowInit = true;
    nextDateShowMillis = millis() + (unsigned long)max(showDateIntervalSec, 5) * 1000UL;
    nextTempShowMillis = millis() + (unsigned long)max(showTempIntervalSec, 5) * 1000UL;
    nextHumidityShowMillis = millis() + (unsigned long)max(showHumidityIntervalSec, 5) * 1000UL;
  }

  if (showDate && (long)(millis() - nextDateShowMillis) >= 0) {
    showCurrentDate();
    nextDateShowMillis = millis() + (unsigned long)max(showDateIntervalSec, 5) * 1000UL;
  }
  if (showTemp && (long)(millis() - nextTempShowMillis) >= 0) {
    showCurrentTemp();
    nextTempShowMillis = millis() + (unsigned long)max(showTempIntervalSec, 5) * 1000UL;
  }
  if (showHumidity && (long)(millis() - nextHumidityShowMillis) >= 0) {
    showCurrentHumidity();
    nextHumidityShowMillis = millis() + (unsigned long)max(showHumidityIntervalSec, 5) * 1000UL;
  }

  // Manual "Show now" buttons on the web configuration page:
  if (triggerShowDateNow) {
    triggerShowDateNow = false;
    showCurrentDate();
    nextDateShowMillis = millis() + (unsigned long)max(showDateIntervalSec, 5) * 1000UL;
  }
  if (triggerShowTempNow) {
    triggerShowTempNow = false;
    showCurrentTemp();
    nextTempShowMillis = millis() + (unsigned long)max(showTempIntervalSec, 5) * 1000UL;
  }
  if (triggerShowHumidityNow) {
    triggerShowHumidityNow = false;
    showCurrentHumidity();
    nextHumidityShowMillis = millis() + (unsigned long)max(showHumidityIntervalSec, 5) * 1000UL;
  }

  showCurrentTime();
  showDCW();
}


// ###########################################################################################################################################
// # Handle day and night time mode:
// ###########################################################################################################################################

void DayNightMode(int displayonMin, int displayonMax) {
  if (iHour > displayonMin && iHour < displayonMax) {
    pixels.setBrightness(intensity);  // Day brightness
    NightModeActive = false;
    ShowTheTime();
  } else {
    if (useNightLEDs == -1) {
      pixels.setBrightness(intensityNight);  // Night brightness
      NightModeActive = true;
      ShowTheTime();
    } else {
      dunkel();
    }
  }
}


// ###########################################################################################################################################
// # NTP time function:
// ###########################################################################################################################################
void configNTPTime() {
#if defined(ESP32)
  configTzTime(timeZone.c_str(), ntpServer.c_str());
#else
  configTime(timeZone.c_str(), ntpServer.c_str());
#endif
}


// ###########################################################################################################################################
// # Convert hex digit to int value:
// ###########################################################################################################################################
unsigned char h2int(char c) {
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}


// ###########################################################################################################################################
// # Get RTC - if any:
// ###########################################################################################################################################
int checkRTC() {
  // Initialize & check RTC (once)
  static bool rtcChecked = false;
  if (!rtcChecked) {
    rtcChecked = true;
    Wire.begin(hw.sdaPin, hw.sclPin);  // start RTC Communication via Wire.h library, pins from the configuration page
    if (rtc.begin()) {
      rtcStarted = -1;
      useRTC = 1;
      Serial.println("RTC found");
    } else {
      useRTC = 0;
      Serial.println("Couldn't find RTC! --> The time comes from the NTP server only.");
    }
  }
  return rtcStarted;
}


// ###########################################################################################################################################
// # Decode %xx values in String - comming from URL / HTTP:
// ###########################################################################################################################################
String urldecode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == '+') {
      encodedString += ' ';
    } else if (c == '%') {
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      encodedString += c;
    } else {
      encodedString += c;
    }
    yield();
  }
  return encodedString;
}


// ###########################################################################################################################################
// # REST command function: ROOT
// ###########################################################################################################################################
void handleRoot() {
  server1->send(200, "text/plain", "WordClock REST web server active");
  Serial.print("WordClock REST web server active on port: ");
  Serial.println(server1port);
}


// ###########################################################################################################################################
// # REST command function: LED set to ON
// ###########################################################################################################################################
void ledsON() {
  server1->send(200, "text/plain", "WordClock LEDs set to ON");
  Serial.println("WordClock LEDs set to ON");
  pixels.setBrightness(intensity);
  RESTmanLEDsON = true;
  LEDsON = true;
  pixels.show();
}


// ###########################################################################################################################################
// # REST command function: LED set to OFF
// ###########################################################################################################################################
void ledsOFF() {
  server1->send(200, "text/plain", "WordClock LEDs set to OFF");
  Serial.println("WordClock LEDs set to OFF");
  pixels.setBrightness(0);
  pixels.show();
  RESTmanLEDsON = false;
  LEDsON = false;
  pixels.show();
}


// ###########################################################################################################################################
// # REST command function: LED status
// ###########################################################################################################################################
void LedStatus() {
  server1->send(200, "text/plain", String(LEDsON));
  Serial.print("LED status: ");
  Serial.println(LEDsON);
}


// ###########################################################################################################################################
// # Startup LED test function:
// ###########################################################################################################################################
// LED test --> no blank display if WiFi was not set yet:
void DisplayTest() {
  if (useledtest) {
    Serial.println("Display Test...");
    pixels.setBrightness(64);

    for (int i = 0; i < NUMPIXELS; i++) {
      setLED(i, i, 0);
      pixels.show();
    }

    for (int i = 0; i < NUMPIXELS; i++) {
      setLED(i, i, 1);
      pixels.show();
      delay(50);
      pixels.setPixelColor(i, 0, 0, 0);
    }

    pixels.setPixelColor(NUMPIXELS, 0, 0, 0);
    pixels.show();
  }
}


// ###########################################################################################################################################
// # Startup LED test function:
// ###########################################################################################################################################
void SetWLAN() {  // Shown at startup and as long as the clock has no time (no WiFi and no RTC)
  if (usesetwlan) {
    switch (switchLangWeb) {
      case 0:
      default:  // Spanish: same LED layout as German
        {  // DE:
          setLED(6, 6, 1);      // S
          setLED(12, 12, 1);    // E
          setLED(24, 24, 1);    // T
          setLED(63, 63, 1);    // W
          setLED(83, 83, 1);    // L
          setLED(84, 84, 1);    // A
          setLED(93, 93, 1);    // N
          setLED(110, 110, 1);  // Corner 1
          setLED(111, 111, 1);  // Corner 2
          setLED(112, 112, 1);  // Corner 3
          setLED(113, 113, 1);  // Corner 4
          break;
        }
      case 1:
        {  // EN:
          setLED(31, 31, 1);    // W
          setLED(25, 25, 1);    // I
          setLED(76, 76, 1);    // F
          setLED(71, 71, 1);    // I
          setLED(110, 110, 1);  // Corner 1
          setLED(111, 111, 1);  // Corner 2
          setLED(112, 112, 1);  // Corner 3
          setLED(113, 113, 1);  // Corner 4
          break;
        }
    }
    pixels.show();
  }
}

// ###########################################################################################################################################
// # Restart the clock:
// ###########################################################################################################################################
void ClockRestart() {  // Called by the REST function /clockrestart and by the configuration page
  dunkel();
  Serial.println("Show RESET before board reset...");
  switch (switchLangWeb) {
    case 0:  // DE:
    default:  // Spanish: same LED layout as German
      {
        setLED(30, 31, 1);  // RE
        setLED(58, 58, 1);  // S
        setLED(64, 64, 1);  // E
        setLED(87, 87, 1);  // T
        break;
      }
    case 1:  // EN:
      {
        setLED(16, 16, 1);  // R
        setLED(30, 30, 1);  // E
        setLED(37, 37, 1);  // S
        setLED(39, 39, 1);  // E
        setLED(42, 42, 1);  // T
        break;
      }
  }
  pixels.show();
  Serial.println("##########################################");
  Serial.println("# WORDCLOCK WILL RESTART IN 3 SECONDS... #");
  Serial.println("##########################################");
  delay(3000);
  ESP.restart();
}


// ###########################################################################################################################################
// # Reset the WiFi configuration:
// ###########################################################################################################################################
void ClockWifiReset() {  // Called by the REST function /clockwifireset and by the configuration page
  dunkel();
  switch (switchLangWeb) {
    case 0:
    default:  // Spanish: same LED layout as German
      {  // DE:
        Serial.println("Show SET WLAN...");
        setLED(6, 6, 1);      // S
        setLED(12, 12, 1);    // E
        setLED(24, 24, 1);    // T
        setLED(63, 63, 1);    // W
        setLED(83, 83, 1);    // L
        setLED(84, 84, 1);    // A
        setLED(93, 93, 1);    // N
        setLED(110, 110, 1);  // Corner 1
        setLED(111, 111, 1);  // Corner 2
        setLED(112, 112, 1);  // Corner 3
        setLED(113, 113, 1);  // Corner 4
        break;
      }
    case 1:
      {  // EN:
        Serial.println("Show WIFI...");
        setLED(31, 31, 1);    // W
        setLED(25, 25, 1);    // I
        setLED(76, 76, 1);    // F
        setLED(71, 71, 1);    // I
        setLED(110, 110, 1);  // Corner 1
        setLED(111, 111, 1);  // Corner 2
        setLED(112, 112, 1);  // Corner 3
        setLED(113, 113, 1);  // Corner 4
        break;
      }
  }
  pixels.show();
  WiFi.disconnect(true);
  delay(1500);
  wifiManager.resetSettings();
  delay(1500);
  Serial.println("####################################################################################################");
  Serial.println("# WIFI SETTING WERE SET TO DEFAULT... WORDCLOCK WILL NOW RESTART... PLEASE CONFIGURE WIFI AGAIN... #");
  Serial.println("####################################################################################################");
  delay(3000);
  ESP.restart();
}


// ###########################################################################################################################################
// # [FORK] WiFi reset button (V6.1.0.2): hold BUTTON_PIN to GND for BUTTON_HOLD_MS -> ClockWifiReset() (deletes the WiFi data, restarts,
// # opens the hotspot). Wiring: push button between BUTTON_PIN and GND, no resistor (internal pull-up). Settings: see settings.h.
// ###########################################################################################################################################
bool buttonEnabled = false;          // false if BUTTON_PIN = -1 or the pin is used for something else
bool buttonDown = false;             // button is currently held
unsigned long buttonDownSince = 0;   // millis() when the button was pressed
int buttonCorners = 0;               // how many corner LEDs are lit as a warning
unsigned long buttonLastShow = 0;    // millis() of the last warning display

void setupResetButton() {
  // The pin must exist and must not be one of the pins configured on the configuration page (LED strip, RTC):
  buttonEnabled = (BUTTON_PIN >= 0) && validPin(BUTTON_PIN) && BUTTON_PIN != hw.ledPin && BUTTON_PIN != hw.sdaPin && BUTTON_PIN != hw.sclPin;
  if (buttonEnabled) {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.println("WiFi reset button: pin " + String(BUTTON_PIN) + ", hold " + String(BUTTON_HOLD_MS / 1000) + " s to reset the WiFi");
  } else {
    Serial.println("WiFi reset button: switched off (BUTTON_PIN not set or used for the LEDs / RTC)");
  }
}

void checkResetButton() {  // Called every ~5 ms from serviceDelay()
  if (!buttonEnabled) return;
  if (digitalRead(BUTTON_PIN) == HIGH) {  // not pressed (pull-up). A released button cancels everything:
    buttonDown = false;
    buttonCorners = 0;
    return;
  }
  unsigned long now = millis();
  if (!buttonDown) {  // button just pressed
    buttonDown = true;
    buttonDownSince = now;
    buttonCorners = 0;
    return;
  }
  unsigned long held = now - buttonDownSince;
  if (held >= BUTTON_HOLD_MS) {  // held long enough: reset the WiFi (does not return, the clock restarts)
    buttonDown = false;
    Serial.println("WiFi reset button held for " + String(BUTTON_HOLD_MS / 1000) + " s - resetting the WiFi...");
    ClockWifiReset();
    return;
  }
  // Warning while holding: one more corner LED every BUTTON_HOLD_MS / 5 (the main loop redraws the display, so refresh it every 200 ms)
  int corners = (int)(held / (BUTTON_HOLD_MS / 5));
  if (corners > 4) corners = 4;
  if (corners > 0 && (corners != buttonCorners || now - buttonLastShow > 200)) {
    buttonCorners = corners;
    buttonLastShow = now;
    for (int i = 0; i < corners; i++) setLED(110 + i, 110 + i, 1);  // corner LEDs 110..113
    pixels.show();
  }
}


// ###########################################################################################################################################
// # RainBow variant 1 - color of a word:
// ###########################################################################################################################################
// The words of the time get random colors, new every minute. The colors come from rainbowSeed and the number of the word,
// so they stay the same during the minute and the configuration page can show the same colors (same calculation there).
// Of 8 random colors the one that differs most from the words before is used, so f.e. ES and IST never look alike.
uint32_t rainbowRandom(uint32_t n) {
  uint32_t h = rainbowSeed ^ (n * 0x9E3779B9UL);
  h ^= h >> 16;
  h *= 0x85EBCA6BUL;
  h ^= h >> 13;
  h *= 0xC2B2AE35UL;
  h ^= h >> 16;
  return h;
}

uint16_t rainbowHue(int ledNr) {
  if (rainbowWordNr < 0 || rainbowWordNr >= 16) {  // Not a word of the time (date, IP-address, ...): color by position
    return (uint16_t)(((long)((ledNr * 23L) % NUMPIXELS) * 65536L) / NUMPIXELS);
  }
  uint16_t bestHue = 0;
  long bestDistance = -1;
  for (int attempt = 0; attempt < 8; attempt++) {
    uint16_t hue = rainbowRandom(rainbowWordNr * 8 + attempt) & 0xFFFF;
    long distance = 65536;  // Distance on the color circle to the nearest color used before
    for (int k = 0; k < rainbowWordNr; k++) {
      long d = labs((long)hue - (long)rainbowWordHues[k]);
      if (d > 32768) d = 65536 - d;
      if (d < distance) distance = d;
    }
    if (distance > bestDistance) {
      bestDistance = distance;
      bestHue = hue;
    }
  }
  rainbowWordHues[rainbowWordNr++] = bestHue;
  return bestHue;
}


// ###########################################################################################################################################
// # Actual function, which controls 1/0 of the LED:
// ###########################################################################################################################################
void setLED(int ledNrFrom, int ledNrTo, int switchOn) {
  if (switchOn) {
    if (switchRainBow == 1) {  // RainBow effect active: every word in its own color
      if (ledNrFrom > ledNrTo) {
        setLED(ledNrTo, ledNrFrom, switchOn);
      } else {
        uint32_t color = pixels.gamma32(pixels.ColorHSV(rainbowHue(ledNrFrom)));
        for (int i = ledNrFrom; i <= ledNrTo; i++) {
          if ((i >= 0) && (i < NUMPIXELS)) pixels.setPixelColor(i, color);
        }
      }
    } else {
      if (ledNrFrom > ledNrTo) {
        setLED(ledNrTo, ledNrFrom, switchOn);  //sets LED numbers in correct order (because of the date programming below)
      } else {
        for (int i = ledNrFrom; i <= ledNrTo; i++) {
          if ((i >= 0) && (i < NUMPIXELS))
            pixels.setPixelColor(i, pixels.Color(redVal, greenVal, blueVal));
        }
      }
    }
  }
}


// ###########################################################################################################################################
// # Actual function, which controls 1/0 of the LED:
// ###########################################################################################################################################
void setLEDHour(int ledNrFrom, int ledNrTo, int switchOn) {
  // Every Hour blink orange
  if ((blinkTime) && (switchOn) && (iMinute == 0) && (iSecond < 10)) {
    if ((iSecond % 2) == 1) {
      for (int i = ledNrFrom; i <= ledNrTo; i++) {
        if ((i >= 0) && (i < NUMPIXELS))
          pixels.setPixelColor(i, pixels.Color(255, 128, 0));
      }
    } else
      setLED(ledNrFrom, ledNrTo, switchOn);
  } else
    setLED(ledNrFrom, ledNrTo, switchOn);
}


// ###########################################################################################################################################
// # Switch a horizontal sequence of LEDs ON or OFF, depending on boolean value switchOn:
// ###########################################################################################################################################
void setLEDLine(int xFrom, int xTo, int y, int switchOn) {
  if (xFrom > xTo)
    setLEDLine(xTo, xFrom, y, switchOn);
  else {
    for (int x = xFrom; x <= xTo; x++) {
      setLED(ledXY(x, y), ledXY(x, y), switchOn);
    }
  }
}


// ###########################################################################################################################################
// # PING function... Turn off the LED by presence status of IP-addresses (of your smart phone) monitored by a PING request 2 times perminute:
// ###########################################################################################################################################
void PingIP() {
  if (iSecond == 45 || iSecond == 15) {
    IPAddress IP1(PING_IP_ADDR1_O1, PING_IP_ADDR1_O2, PING_IP_ADDR1_O3, PING_IP_ADDR1_O4);
    IPAddress IP2(PING_IP_ADDR2_O1, PING_IP_ADDR2_O2, PING_IP_ADDR2_O3, PING_IP_ADDR2_O4);
    IPAddress IP3(PING_IP_ADDR3_O1, PING_IP_ADDR3_O2, PING_IP_ADDR3_O3, PING_IP_ADDR3_O4);

    // IP 1:
    // #####
    if (PING_IP_ADDR1_O1 != 0) {
      if (PING_DEBUG_MODE == 1) {
        Serial.print("IP gets pinged now: " + String(iHour) + ":" + String(iMinute) + ":" + String(iSecond) + " - IP1 ");
        Serial.print(String(PING_IP_ADDR1_O1) + "." + String(PING_IP_ADDR1_O2) + "." + String(PING_IP_ADDR1_O3) + "." + String(PING_IP_ADDR1_O4));
        Serial.print(" --> ");
      }
      if (Ping.ping(IP1)) {
        if (PING_DEBUG_MODE == 1) Serial.print("online - remaining attempts = ");
        PING_ATTEMPTSIP1 = PING_TIMEOUTNUM;  // Reset to configured value
        if (PING_DEBUG_MODE == 1) Serial.println(PING_ATTEMPTSIP1);
        PingStatusIP1 = true;
      } else {
        if (PING_ATTEMPTSIP1 >= 1) PING_ATTEMPTSIP1 = PING_ATTEMPTSIP1 - 1;
        if (PING_DEBUG_MODE == 1) Serial.print("OFFLINE - remaining attempts = ");
        if (PING_DEBUG_MODE == 1) Serial.println(PING_ATTEMPTSIP1);
      }
      if (PING_ATTEMPTSIP1 == 0) {
        PingStatusIP1 = false;
      }
    } else {
      PingStatusIP1 = false;
    }

    // IP 2:
    // #####
    if (PING_IP_ADDR2_O1 != 0) {
      if (PING_DEBUG_MODE == 1) {
        Serial.print("IP gets pinged now: " + String(iHour) + ":" + String(iMinute) + ":" + String(iSecond) + " - IP2 ");
        Serial.print(String(PING_IP_ADDR2_O1) + "." + String(PING_IP_ADDR2_O2) + "." + String(PING_IP_ADDR2_O3) + "." + String(PING_IP_ADDR2_O4));
        Serial.print(" --> ");
      }
      if (Ping.ping(IP2)) {
        if (PING_DEBUG_MODE == 1) Serial.print("online - remaining attempts = ");
        PING_ATTEMPTSIP2 = PING_TIMEOUTNUM;  // Reset to configured value
        if (PING_DEBUG_MODE == 1) Serial.println(PING_ATTEMPTSIP2);
        PingStatusIP2 = true;
      } else {
        if (PING_ATTEMPTSIP2 >= 1) PING_ATTEMPTSIP2 = PING_ATTEMPTSIP2 - 1;
        if (PING_DEBUG_MODE == 1) Serial.print("OFFLINE - remaining attempts = ");
        if (PING_DEBUG_MODE == 1) Serial.println(PING_ATTEMPTSIP2);
      }
      if (PING_ATTEMPTSIP2 == 0) {
        PingStatusIP2 = false;
      }
    } else {
      PingStatusIP2 = false;
    }

    // IP 3:
    // #####
    if (PING_IP_ADDR3_O1 != 0) {
      if (PING_DEBUG_MODE == 1) {
        Serial.print("IP gets pinged now: " + String(iHour) + ":" + String(iMinute) + ":" + String(iSecond) + " - IP3 ");
        Serial.print(String(PING_IP_ADDR3_O1) + "." + String(PING_IP_ADDR3_O2) + "." + String(PING_IP_ADDR3_O3) + "." + String(PING_IP_ADDR3_O4));
        Serial.print(" --> ");
      }
      if (Ping.ping(IP3)) {
        if (PING_DEBUG_MODE == 1) Serial.print("online - remaining attempts = ");
        PING_ATTEMPTSIP3 = PING_TIMEOUTNUM;  // Reset to configured value
        if (PING_DEBUG_MODE == 1) Serial.println(PING_ATTEMPTSIP3);
        PingStatusIP3 = true;
      } else {
        if (PING_ATTEMPTSIP3 >= 1) PING_ATTEMPTSIP3 = PING_ATTEMPTSIP3 - 1;
        if (PING_DEBUG_MODE == 1) Serial.print("OFFLINE - remaining attempts = ");
        if (PING_DEBUG_MODE == 1) Serial.println(PING_ATTEMPTSIP3);
      }
      if (PING_ATTEMPTSIP3 == 0) {
        PingStatusIP3 = false;
      }
    } else {
      PingStatusIP3 = false;
    }

    // PING status check:
    if (PingStatusIP1 == true || PingStatusIP2 == true || PingStatusIP3 == true) {
      if (NightModeActive == false) pixels.setBrightness(intensity);
      if (NightModeActive == true) pixels.setBrightness(intensityNight);
      if (RESTmanLEDsON == true) LEDsON = true;
    }
    if (PingStatusIP1 == false && PingStatusIP2 == false && PingStatusIP3 == false) {
      if (PING_DEBUG_MODE == 1) Serial.println("All clients offline --> LEDs = OFF until one of the IP-addresses can be reached again...");
      pixels.setBrightness(0);
      pixels.show();
      if (RESTmanLEDsON == true) LEDsON = false;
    }
  }
}


// ###########################################################################################################################################
// # Wifi Manager setup and reconnect function that runs once at startup and during the loop function of the ESP:
// ###########################################################################################################################################
// The WordClock does not wait for the WiFi:
// - At startup it connects to the saved WiFi. If that fails (or nothing is saved yet), the WiFi Manager hotspot
//   DEFAULT_AP_NAME opens for AP_TIMEOUT seconds. The clock keeps showing the time (from the RTC) meanwhile.
// - Afterwards the ESP keeps trying the saved WiFi in the background. To enter new WiFi data, restart the clock.
void WIFI_login() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  Serial.print("Try to connect to WiFi: ");
  Serial.println(WiFi.SSID());
  bool WiFires;
  bool DebugWifiLEDs = false;
  configNTPTime();                                      // Set timezone
  wifiManager.setHostname(wchostname + wchostnamenum);  // Hostname in your router
  wifiManager.setConfigPortalBlocking(false);           // The clock keeps running while the hotspot is open
  wifiManager.setConfigPortalTimeout(AP_TIMEOUT);       // Max wait for 4 minutes
  wifiManager.setConnectTimeout(20);
  WiFires = wifiManager.autoConnect(DEFAULT_AP_NAME);
  if (!WiFires) {
    portalActive = true;
    Serial.print("Failed to connect to WiFi: ");
    Serial.println(WiFi.SSID());
    Serial.printf("WiFi Manager hotspot \"%s\" is open for %d seconds\n", DEFAULT_AP_NAME, AP_TIMEOUT);
    if (DebugWifiLEDs) WIFI_DebugWifiLEDs(pixels.Color(255, 0, 0));
  } else {
    Serial.print("Connected to WiFi: ");
    Serial.println(WiFi.SSID());
    if (DebugWifiLEDs) WIFI_DebugWifiLEDs(pixels.Color(0, 255, 0));
  }
}


// ###########################################################################################################################################
// # Check the WiFi connection during the loop function: WiFi Manager hotspot, start of the configuration page after connecting
// ###########################################################################################################################################
void WIFI_check() {
  if (portalActive) {
    wifiManager.process();
    if (!wifiManager.getConfigPortalActive()) {  // Hotspot closed: WiFi data saved or timeout
      portalActive = false;
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Manager hotspot closed - trying the saved WiFi in the background");
        WiFi.mode(WIFI_STA);
        WiFi.begin();
      }
    } else if (WiFi.status() == WL_CONNECTED) {  // The saved WiFi came back while the hotspot was open
      wifiManager.stopConfigPortal();
      portalActive = false;
    }
  }
  bool connected = WiFi.status() == WL_CONNECTED && !portalActive;
  if (connected && !wasConnected) {
    Serial.println("Connected to WiFi: " + WiFi.SSID() + " - IP-address " + WiFi.localIP().toString());
    if (!servicesStarted) startNetworkServices();
  }
  if (!connected && wasConnected) Serial.println(rtcTimeValid ? "WiFi connection lost - the clock keeps running with the RTC time" : "WiFi connection lost - the clock keeps running with its internal time");
  wasConnected = connected;
}


// ###########################################################################################################################################
// # Start the configuration page, the update and the REST functions after the first WiFi connection:
// ###########################################################################################################################################
void startNetworkServices() {
  MDNS.begin((wchostname + wchostnamenum).c_str());  // Reachable as http://WordClock<n>.local
  MDNS.addService("http", "tcp", 80);
  if (useupdate == -1) useupdate = 1;            // Fix for settings from older versions
  if (useupdate == 1) startLocalUpdateServer();  // Local web update function setup
  setupWebServer();                              // Configuration page
  if (useresturl) startRestServer();             // REST function web server
  servicesStarted = true;
  Serial.println("######################################################################");
  Serial.print("# Configuration page now available: ");
  Serial.println("http://" + WiFi.localIP().toString());
  Serial.println("######################################################################");
  if (useshowip && !ipShown) {
    ipShown = true;
    showIP();  // Show IP-address on display
  }
}


// ###########################################################################################################################################
// # LED debug for Wifi Manager setup and reconnect function that runs once at startup and during the loop function of the ESP:
// ###########################################################################################################################################
void WIFI_DebugWifiLEDs(uint32_t color) {
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, (0, 0, 0));
  }
  pixels.show();
  delay(1000);
  int myArray[] = { 110, 111, 112, 113 };
  for (int element : myArray) {
    pixels.setPixelColor(element, color);
  }
  pixels.show();
  delay(3000);
}


// ###########################################################################################################################################
// # Direct update for the ESP:
// ###########################################################################################################################################
void esphttpupdate() {  // Basic ESP update function (REST: /esphttpupdate) - installs the newest GitHub release
  if (useupdate != 2) {
    server1->send(200, "text/plain", "The automatic update function is currently disabled...");
    Serial.println("The automatic update function is currently disabled...");
  } else {
    String errorMessage;
    if (!readhttpfile(errorMessage)) {  // Check for the newest version on GitHub
      server1->send(502, "text/plain", "GitHub is not reachable...");
    } else if (UpdateAvailable == 1) {
      server1->send(200, "text/plain", "Update Start... Check status on the display...");
      Serial.println("Update START");
      delay(300);
      installRelease(AvailableVersion);
    } else {
      server1->send(200, "text/plain", "No update available...");
      Serial.println("No update available...");
    }
  }
}

void update_started() {  // Callback update start function
  Serial.println("CALLBACK:  HTTP update process started");
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, (0, 0, 0));
  }
  pixels.show();
  switchRainBow = 0;
  pixels.setBrightness(50);
  redVal = 0;
  greenVal = 255;
  blueVal = 255;
  // Arrow borders:
  setLED(3, 7, 1);
  setLED(14, 14, 1);
  setLED(18, 18, 1);
  setLED(29, 29, 1);
  setLED(25, 25, 1);
  setLED(36, 36, 1);
  setLED(40, 40, 1);
  setLED(51, 51, 1);
  setLED(47, 47, 1);
  setLED(56, 58, 1);
  setLED(62, 64, 1);
  setLED(68, 68, 1);
  setLED(74, 74, 1);
  setLED(80, 80, 1);
  setLED(84, 84, 1);
  setLED(94, 94, 1);
  setLED(92, 92, 1);
  setLED(104, 104, 1);
  pixels.show();
  delay(1500);
}

void update_finished() {  // Callback update success finish function
  Serial.println("CALLBACK:  HTTP update process finished");
  switchRainBow = 0;
  pixels.setBrightness(50);
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, (0, 0, 0));
  }
  pixels.show();
  update_msg_LEDs(0, 255, 0);
  delay(3000);
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, (0, 0, 0));
  }
  pixels.show();
  Serial.println("Show RESET before board reset...");
  switch (switchLangWeb) {
    case 0:  // DE:
    default:  // Spanish: same LED layout as German
      {
        setLED(30, 31, 1);  // RE
        setLED(58, 58, 1);  // S
        setLED(64, 64, 1);  // E
        setLED(87, 87, 1);  // T
        break;
      }
    case 1:  // EN:
      {
        setLED(16, 16, 1);  // R
        setLED(30, 30, 1);  // E
        setLED(37, 37, 1);  // S
        setLED(39, 39, 1);  // E
        setLED(42, 42, 1);  // T
        break;
      }
  }
  pixels.show();
  Serial.println("##########################################");
  Serial.println("# WORDCLOCK WILL RESTART IN 3 SECONDS... #");
  Serial.println("##########################################");
  delay(3000);
}

void update_error(int err) {  // Callback update error finish function
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
  switchRainBow = 0;
  pixels.setBrightness(50);
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, (0, 0, 0));
  }
  pixels.show();
  update_msg_LEDs(255, 0, 0);
  delay(3000);
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, (0, 0, 0));
  }
  pixels.show();
  Serial.println("Show RESET before board reset...");
  switch (switchLangWeb) {
    case 0:  // DE:
    default:  // Spanish: same LED layout as German
      {
        setLED(30, 31, 1);  // RE
        setLED(58, 58, 1);  // S
        setLED(64, 64, 1);  // E
        setLED(87, 87, 1);  // T
        break;
      }
    case 1:  // EN:
      {
        setLED(16, 16, 1);  // R
        setLED(30, 30, 1);  // E
        setLED(37, 37, 1);  // S
        setLED(39, 39, 1);  // E
        setLED(42, 42, 1);  // T
        break;
      }
  }
  pixels.show();
  Serial.println("##########################################");
  Serial.println("# WORDCLOCK WILL RESTART IN 3 SECONDS... #");
  Serial.println("##########################################");
  delay(3000);
}

void update_msg_LEDs(int redCol, int greenCol, int blueCol) {
  redVal = redCol;
  greenVal = greenCol;
  blueVal = blueCol;
  setLED(1, 2, 1);  // 1 (Arrow)
  setLED(4, 6, 1);
  setLED(8, 8, 1);
  setLED(10, 10, 1);
  setLED(21, 21, 1);  // 2
  setLED(19, 19, 1);
  setLED(17, 17, 1);
  setLED(15, 15, 1);
  setLED(13, 13, 1);
  setLED(11, 11, 1);
  setLED(22, 22, 1);  // 3
  setLED(24, 24, 1);
  setLED(26, 28, 1);
  setLED(30, 30, 1);
  setLED(32, 32, 1);
  setLED(33, 35, 1);  // 4
  setLED(37, 37, 1);
  setLED(41, 42, 1);
  setLED(72, 74, 1);  // O (7-10)
  setLED(81, 81, 1);
  setLED(94, 94, 1);
  setLED(79, 79, 1);
  setLED(96, 96, 1);
  setLED(101, 103, 1);
  setLED(70, 70, 1);  // K (7-10)
  setLED(83, 83, 1);
  setLED(92, 92, 1);
  setLED(105, 105, 1);
  setLED(68, 68, 1);
  setLED(84, 84, 1);
  setLED(91, 91, 1);
  setLED(107, 107, 1);
  pixels.show();
}

void update_progress(int cur, int total) {  // Callback update download progress function
  switchRainBow = 0;
  pixels.setBrightness(50);
  // Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
  int percent = (cur * 100) / total;
  redVal = 0;
  greenVal = 255;
  blueVal = 0;
  if (percent >= 10) {
    setLED(3, 7, 1);
  }
  if (percent >= 20) {
    setLED(14, 18, 1);
  }
  if (percent >= 30) {
    setLED(25, 29, 1);
  }
  if (percent >= 40) {
    setLED(36, 40, 1);
  }
  if (percent >= 50) {
    setLED(47, 51, 1);
  }
  if (percent >= 60) {
    setLED(56, 64, 1);
  }
  if (percent >= 70) {
    setLED(68, 74, 1);
  }
  if (percent >= 80) {
    setLED(80, 84, 1);
  }
  if (percent >= 90) {
    setLED(92, 94, 1);
  }
  if (percent >= 99) {
    setLED(104, 104, 1);
  }
  if (percent == 100) {
    delay(1500);
  }
  pixels.show();
  Serial.print("Automatic update - download file progress: ");
  Serial.print(percent);
  Serial.println("%");
}


// ###########################################################################################################################################
// # REST function web server (port server1port):
// ###########################################################################################################################################
void startRestServer() {
  if (server1) return;
  server1.reset(new ESP8266WebServer(WiFi.localIP(), server1port));
  server1->on("/", handleRoot);
  server1->on("/ledson", ledsON);
  server1->on("/ledsoff", ledsOFF);
  server1->on("/clockrestart", []() {
    server1->send(200, "text/plain", "WORDCLOCK WILL RESTART IN 3 SECONDS...");
    delay(1000);
    ClockRestart();
  });
  server1->on("/clockwifireset", []() {
    server1->send(200, "text/plain", "WIFI SETTING WERE SET TO DEFAULT... WORDCLOCK WILL NOW RESTART... PLEASE CONFIGURE WIFI AGAIN...");
    delay(1000);
    ClockWifiReset();
  });
  server1->on("/ledstatus", LedStatus);
  server1->on("/esphttpupdate", esphttpupdate);
  server1->begin();
}


// ###########################################################################################################################################
// # Local update page (http://<clock>:2022/update):
// ###########################################################################################################################################
void startLocalUpdateServer() {
  static bool updaterReady = false;
  if (!updaterReady) {
    httpUpdater.setup(&httpServer);
    MDNS.addService("http", "tcp", 2022);
    updaterReady = true;
  }
  httpServer.begin();
  UpdatePath = "http://" + String(wchostname + wchostnamenum) + ".local:2022/update";
  UpdatePathIP = "http://" + WiFi.localIP().toString() + ":2022/update";
}

void applyUpdateMode(int mode) {
  if (mode == useupdate) return;
  useupdate = mode;
  if (useupdate == 1) startLocalUpdateServer();
  else httpServer.stop();
}


// ###########################################################################################################################################
// # Web interface: static page from webpage.h plus a small JSON API on port 80
// #
// #   GET  /                 web interface (gzipped HTML, see web/index.html)
// #   GET  /api/state        all settings ("cfg") and device status ("status")
// #   POST /api/config       save settings (same JSON shape as "cfg")
// #   POST /api/restart      restart the clock
// #   POST /api/wifireset    delete the WiFi credentials and restart
// #   POST /api/cornertest   light the corner LEDs 110..113 one after another
// #   POST /api/time         set the time from the browser: {"epoch": <unix time>}
// #   POST /api/updatecheck  look up the newest GitHub release
// #   POST /api/update       install the release found by /api/updatecheck
// ###########################################################################################################################################
void setupWebServer() {
  server.on("/", HTTP_GET, handleIndex);
  server.on("/index.html", HTTP_GET, handleIndex);
  server.on("/api/state", HTTP_GET, handleApiState);
  server.on("/api/config", HTTP_POST, handleApiConfig);
  server.on("/api/restart", HTTP_POST, handleApiRestart);
  server.on("/api/shownow", HTTP_POST, handleApiShowNow);
  server.on("/api/wifireset", HTTP_POST, handleApiWifiReset);
  server.on("/api/cornertest", HTTP_POST, handleApiCornerTest);
  server.on("/api/time", HTTP_POST, handleApiTime);
  server.on("/api/updatecheck", HTTP_POST, handleApiUpdateCheck);
  server.on("/api/update", HTTP_POST, handleApiUpdate);
  server.on("/setWC.php", HTTP_GET, handleHiddenDCW);  // Hidden function, see handleHiddenDCW()
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
  server.begin();
}

// Hidden function (not shown in the web interface): http://<clock>/setWC.php?DCW=ON or ?DCW=OFF
// switches the orange "DCW" (Dorf Campus Wanlo) that flashes at every full hour. Same URL as in the old versions,
// but now it changes only this flag (the old page also switched off all other options that were missing in the URL).
void handleHiddenDCW() {
  String value = server.arg("DCW");
  value.toUpperCase();
  if (value == "ON") dcwFlag = -1;
  else if (value == "OFF") dcwFlag = 0;
  else return server.send(404, "text/plain", "Not found");
  writeEEPROM();
  server.send(200, "text/plain", dcwFlag ? "DCW ON" : "DCW OFF");
}

void handleIndex() {
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, PSTR("text/html; charset=utf-8"), (PGM_P)WEBPAGE_GZ, WEBPAGE_GZ_LEN);
}

void sendJson(int code, JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", out);
}

void sendOk() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", "{\"ok\":true}");
}

void sendError(int code, const char* message) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["error"] = message;
  sendJson(code, doc);
}

String colorToHex(int r, int g, int b) {
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", r & 255, g & 255, b & 255);
  return String(buf);
}

bool hexToColor(const char* s, int& r, int& g, int& b) {
  if (!s || s[0] != '#' || strlen(s) != 7) return false;
  for (int i = 1; i < 7; i++)
    if (!isxdigit((unsigned char)s[i])) return false;
  long v = strtol(s + 1, nullptr, 16);
  r = (v >> 16) & 255;
  g = (v >> 8) & 255;
  b = v & 255;
  return true;
}

// Night mode times per weekday, Monday first (same order as in the web interface):
int* const nightOffFrom[7] = { &displayonmaxMO, &displayonmaxTU, &displayonmaxWE, &displayonmaxTH, &displayonmaxFR, &displayonmaxSA, &displayonmaxSU };
int* const nightOnAfter[7] = { &displayonminMO, &displayonminTU, &displayonminWE, &displayonminTH, &displayonminFR, &displayonminSA, &displayonminSU };

int nightModeValue() {  // 0 = unchanged, 1 = display off, 2 = dimmed
  if (!displayoff) return 0;
  return useNightLEDs ? 2 : 1;
}

int minModeValue() {  // 0 = clockwise, 1 = counter-clockwise, 2 = custom order
  if (useCustomMinuteOrder) return 2;
  return switchLEDOrder ? 0 : 1;
}

void handleApiState() {
  if (useupdate < 0 || useupdate > 2) useupdate = 1;  // Fix for settings from older versions

  JsonDocument doc;
  JsonObject c = doc["cfg"].to<JsonObject>();
  // In the random color mode redVal/greenVal/blueVal change every minute, so report the saved color.
  if (switchRainBow == 2) c["color"] = colorToHex(parameter.pRed, parameter.pGreen, parameter.pBlue);
  else c["color"] = colorToHex(redVal, greenVal, blueVal);
  c["rainbow"] = switchRainBow;
  c["bright"] = intensity;
  c["brightNight"] = intensityNight;
  c["psu"] = powersupply != 0;
  c["night"] = nightModeValue();
  JsonArray nd = c["nightDays"].to<JsonArray>();
  for (int d = 0; d < 7; d++) {
    JsonArray day = nd.add<JsonArray>();
    day.add(*nightOffFrom[d]);
    day.add(*nightOnAfter[d]);
  }
  c["lang"] = switchLangWeb;
  c["deSpecial"] = DEspecial1;
  c["blink"] = blinkTime != 0;
  c["date"] = showDate != 0;
  c["dateInt"] = showDateIntervalSec;
  c["temp"] = showTemp != 0;
  c["tempInt"] = showTempIntervalSec;
  c["hum"] = showHumidity != 0;
  c["humInt"] = showHumidityIntervalSec;
  JsonObject tx = c["txt"].to<JsonObject>();
  tx["dateStyle"] = txt.dateStyle;
  tx["tempStyle"] = txt.tempStyle;
  tx["humStyle"] = txt.humStyle;
  tx["dateSpeed"] = txt.dateSpeed;
  tx["tempSpeed"] = txt.tempSpeed;
  tx["humSpeed"] = txt.humSpeed;
  tx["hold"] = txt.holdMs;
  tx["tempDec"] = txt.tempDec != 0;
  tx["dateYear"] = txt.dateYear != 0;
  c["bootText"] = useBootText != 0;
  c["boot1"] = bootText1;
  c["boot2"] = bootText2;
  c["minMode"] = minModeValue();
  c["minOrder"] = minuteOrderToDisplayString();
  c["ledTest"] = useledtest != 0;
  c["setWlan"] = usesetwlan != 0;
  c["showIp"] = useshowip != 0;
  c["ntp"] = ntpServer;
  c["tz"] = timeZone;
  c["hostNum"] = wchostnamenum;
  JsonObject p = c["ping"].to<JsonObject>();
  p["on"] = PING_USEMONITOR != 0;
  JsonArray ips = p["ips"].to<JsonArray>();
  ips.add(IPAddress(PING_IP_ADDR1_O1, PING_IP_ADDR1_O2, PING_IP_ADDR1_O3, PING_IP_ADDR1_O4).toString());
  ips.add(IPAddress(PING_IP_ADDR2_O1, PING_IP_ADDR2_O2, PING_IP_ADDR2_O3, PING_IP_ADDR2_O4).toString());
  ips.add(IPAddress(PING_IP_ADDR3_O1, PING_IP_ADDR3_O2, PING_IP_ADDR3_O3, PING_IP_ADDR3_O4).toString());
  p["tries"] = PING_TIMEOUTNUM;
  p["debug"] = PING_DEBUG_MODE != 0;
  c["rest"] = useresturl != 0;
  c["update"] = useupdate;
  JsonObject pins = c["pins"].to<JsonObject>();
  pins["led"] = hw.ledPin;
  pins["sda"] = hw.sdaPin;
  pins["scl"] = hw.sclPin;

  JsonObject s = doc["status"].to<JsonObject>();
  s["version"] = WORD_CLOCK_VERSION;
  s["chip"] = CHIP_NAME;
  s["ip"] = WiFi.localIP().toString();
  s["mac"] = WiFi.macAddress();
  s["ssid"] = WiFi.SSID();
  s["rssi"] = WiFi.RSSI();
  unsigned long ntpAge = lastNtpSync ? millis() - lastNtpSync : 0;
  s["ntpOk"] = lastNtpSync != 0 && ntpAge < 3UL * 3600UL * 1000UL;
  s["ntpAgoMin"] = lastNtpSync ? (long)(ntpAge / 60000UL) : -1;
  s["rtc"] = rtcStarted != 0;
  s["htu"] = checkHTU() != 0;
  s["rtcOk"] = rtcTimeValid;
  s["timeOk"] = timeValid;
  s["uptime"] = millis() / 1000UL;
  s["heap"] = ESP.getFreeHeap();
  s["ledsOn"] = LEDsON && RESTmanLEDsON;
  s["curColor"] = colorToHex(redVal, greenVal, blueVal);
  s["rainbowSeed"] = rainbowSeed;  // RainBow variant 1: the page calculates the same word colors  // Color shown right now (differs from the saved one in the random color mode)
  s["restPort"] = server1port;
  JsonObject pd = s["pinDefaults"].to<JsonObject>();
  pd["led"] = PIN;
  pd["sda"] = SDA_PIN;
  pd["scl"] = SCL_PIN;
  sendJson(200, doc);
}

void handleApiConfig() {
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) return sendError(400, "invalid JSON");
  JsonObjectConst c = doc.as<JsonObjectConst>();
  if (c.isNull()) return sendError(400, "invalid JSON");

  // ---- Read and check everything first, nothing is changed until all values are valid ----
  int r = redVal, g = greenVal, b = blueVal;
  if (switchRainBow == 2) {
    r = parameter.pRed;
    g = parameter.pGreen;
    b = parameter.pBlue;
  }
  if (c["color"].is<const char*>() && !hexToColor(c["color"].as<const char*>(), r, g, b)) return sendError(400, "color");
  int rainbow = constrain((int)(c["rainbow"] | switchRainBow), 0, 2);
  bool psu = c["psu"] | (powersupply != 0);
  int maxIntensity = psu ? 255 : 128;
  int bright = constrain((int)(c["bright"] | intensity), 1, maxIntensity);
  int brightNight = constrain((int)(c["brightNight"] | intensityNight), 1, maxIntensity);
  int night = constrain((int)(c["night"] | nightModeValue()), 0, 2);

  int offFrom[7], onAfter[7];
  for (int d = 0; d < 7; d++) {
    offFrom[d] = *nightOffFrom[d];
    onAfter[d] = *nightOnAfter[d];
  }
  JsonArrayConst nd = c["nightDays"].as<JsonArrayConst>();
  if (!nd.isNull()) {
    if (nd.size() != 7) return sendError(400, "nightDays");
    for (int d = 0; d < 7; d++) {
      offFrom[d] = nd[d][0] | -1;
      onAfter[d] = nd[d][1] | -1;
      if (offFrom[d] < 1 || offFrom[d] > 24 || onAfter[d] < 0 || onAfter[d] > 23) return sendError(400, "nightDays");
    }
  }

  int lang = constrain((int)(c["lang"] | switchLangWeb), 0, 2);  // 0 = DE, 1 = EN, 2 = ES (clock face uses the German layout)
  int deSpecial = constrain((int)(c["deSpecial"] | DEspecial1), 0, 1);
  bool blink = c["blink"] | (blinkTime != 0);
  bool date = c["date"] | (showDate != 0);
  int dateInt = constrain((int)(c["dateInt"] | showDateIntervalSec), 30, 86400);
  bool temp = c["temp"] | (showTemp != 0);
  int tempInt = constrain((int)(c["tempInt"] | showTempIntervalSec), 30, 86400);
  bool hum = c["hum"] | (showHumidity != 0);
  int humInt = constrain((int)(c["humInt"] | showHumidityIntervalSec), 30, 86400);
  JsonObjectConst tx = c["txt"].as<JsonObjectConst>();
  int dateStyle = constrain((int)(tx["dateStyle"] | txt.dateStyle), 0, 2);
  int tempStyle = constrain((int)(tx["tempStyle"] | txt.tempStyle), 0, 3);  // [V2] 3 = compact + bar
  int humStyle = constrain((int)(tx["humStyle"] | txt.humStyle), 0, 3);  // [V2] 3 = compact + bar
  int dateSpeed = constrain((int)(tx["dateSpeed"] | txt.dateSpeed), 30, 600);
  int tempSpeed = constrain((int)(tx["tempSpeed"] | txt.tempSpeed), 30, 600);
  int humSpeed = constrain((int)(tx["humSpeed"] | txt.humSpeed), 30, 600);
  int holdMs = constrain((int)(tx["hold"] | txt.holdMs), 300, 6000);
  bool tempDec = tx["tempDec"] | (txt.tempDec != 0);
  bool dateYear = tx["dateYear"] | (txt.dateYear != 0);
  bool bootOn = c["bootText"] | (useBootText != 0);
  String boot1 = bootText1, boot2 = bootText2;
  if (c["boot1"].is<const char*>()) boot1 = cleanBootText(c["boot1"].as<const char*>());
  if (c["boot2"].is<const char*>()) boot2 = cleanBootText(c["boot2"].as<const char*>());
  int minMode = constrain((int)(c["minMode"] | minModeValue()), 0, 2);
  int order[4] = { minuteLedOrder[0], minuteLedOrder[1], minuteLedOrder[2], minuteLedOrder[3] };
  if (minMode == 2 && !parseMinuteOrder(String(c["minOrder"] | ""), order)) return sendError(400, "minOrder");
  bool ledTest = c["ledTest"] | (useledtest != 0);
  bool setWlan = c["setWlan"] | (usesetwlan != 0);
  bool showIp = c["showIp"] | (useshowip != 0);

  String ntp = ntpServer, tz = timeZone;
  if (c["ntp"].is<const char*>()) ntp = c["ntp"].as<const char*>();
  if (c["tz"].is<const char*>()) tz = c["tz"].as<const char*>();
  ntp.trim();
  tz.trim();
  if (ntp.length() == 0 || ntp.length() >= sizeof(parameter.pNTPServer)) return sendError(400, "ntp");
  if (tz.length() == 0 || tz.length() >= sizeof(parameter.pTimeZone)) return sendError(400, "tz");
  int hostNum = constrain((int)(c["hostNum"] | wchostnamenum), 0, 9);

  JsonObjectConst p = c["ping"].as<JsonObjectConst>();
  bool pingOn = p["on"] | (PING_USEMONITOR != 0);
  bool pingDebug = p["debug"] | (PING_DEBUG_MODE != 0);
  int tries = constrain((int)(p["tries"] | PING_TIMEOUTNUM), 1, 120);
  IPAddress ips[3] = {
    IPAddress(PING_IP_ADDR1_O1, PING_IP_ADDR1_O2, PING_IP_ADDR1_O3, PING_IP_ADDR1_O4),
    IPAddress(PING_IP_ADDR2_O1, PING_IP_ADDR2_O2, PING_IP_ADDR2_O3, PING_IP_ADDR2_O4),
    IPAddress(PING_IP_ADDR3_O1, PING_IP_ADDR3_O2, PING_IP_ADDR3_O3, PING_IP_ADDR3_O4)
  };
  JsonArrayConst ipList = p["ips"].as<JsonArrayConst>();
  for (int i = 0; i < 3 && i < (int)ipList.size(); i++) {
    IPAddress ip;
    if (ip.fromString(ipList[i] | "")) ips[i] = ip;
    else if (pingOn) return sendError(400, "ping.ips");
  }

  bool rest = c["rest"] | (useresturl != 0);
  int update = constrain((int)(c["update"] | useupdate), 0, 2);

  int ledPin = hw.ledPin, sdaPin = hw.sdaPin, sclPin = hw.sclPin;
  JsonObjectConst pinCfg = c["pins"].as<JsonObjectConst>();
  if (!pinCfg.isNull()) {
    ledPin = pinCfg["led"] | ledPin;
    sdaPin = pinCfg["sda"] | sdaPin;
    sclPin = pinCfg["scl"] | sclPin;
    if (!validPin(ledPin) || !validPin(sdaPin) || !validPin(sclPin) || ledPin == sdaPin || ledPin == sclPin || sdaPin == sclPin) return sendError(400, "pins");
  }

  // ---- Apply ----
  redVal = r;
  greenVal = g;
  blueVal = b;
  switchRainBow = rainbow;
  powersupply = psu ? 1 : 0;
  intensity = bright;
  intensityNight = brightNight;
  displayoff = night ? -1 : 0;
  useNightLEDs = (night == 2) ? -1 : 0;
  if (night != 2) NightModeActive = false;
  for (int d = 0; d < 7; d++) {
    *nightOffFrom[d] = offFrom[d];
    *nightOnAfter[d] = onAfter[d];
  }
  switchLangWeb = lang;
  DEspecial1 = deSpecial;
  blinkTime = blink ? -1 : 0;
  showDate = date ? -1 : 0;
  showDateIntervalSec = dateInt;
  showTemp = temp ? -1 : 0;
  showTempIntervalSec = tempInt;
  showHumidity = hum ? -1 : 0;
  showHumidityIntervalSec = humInt;
  bool txtChanged = dateStyle != txt.dateStyle || tempStyle != txt.tempStyle || humStyle != txt.humStyle || dateSpeed != txt.dateSpeed || tempSpeed != txt.tempSpeed || humSpeed != txt.humSpeed || holdMs != txt.holdMs || tempDec != (txt.tempDec != 0) || dateYear != (txt.dateYear != 0);
  txt.dateStyle = dateStyle;
  txt.tempStyle = tempStyle;
  txt.humStyle = humStyle;
  txt.dateSpeed = dateSpeed;
  txt.tempSpeed = tempSpeed;
  txt.humSpeed = humSpeed;
  txt.holdMs = holdMs;
  txt.tempDec = tempDec ? 1 : 0;
  txt.dateYear = dateYear ? 1 : 0;
  useBootText = bootOn ? -1 : 0;
  bootText1 = boot1;
  bootText2 = boot2;
  if (minMode == 2) {
    useCustomMinuteOrder = 1;
    for (int k = 0; k < 4; k++) minuteLedOrder[k] = order[k];
  } else {
    useCustomMinuteOrder = 0;
    switchLEDOrder = (minMode == 0) ? -1 : 0;
    setDefaultMinuteLedOrderFromSwitch();
  }
  useledtest = ledTest ? -1 : 0;
  usesetwlan = setWlan ? -1 : 0;
  useshowip = showIp ? -1 : 0;
  ntpServer = ntp;
  timeZone = tz;
  wchostnamenum = hostNum;
  PING_USEMONITOR = pingOn ? 1 : 0;
  PING_DEBUG_MODE = pingDebug ? 1 : 0;
  PING_TIMEOUTNUM = tries;
  PING_IP_ADDR1_O1 = ips[0][0];
  PING_IP_ADDR1_O2 = ips[0][1];
  PING_IP_ADDR1_O3 = ips[0][2];
  PING_IP_ADDR1_O4 = ips[0][3];
  PING_IP_ADDR2_O1 = ips[1][0];
  PING_IP_ADDR2_O2 = ips[1][1];
  PING_IP_ADDR2_O3 = ips[1][2];
  PING_IP_ADDR2_O4 = ips[1][3];
  PING_IP_ADDR3_O1 = ips[2][0];
  PING_IP_ADDR3_O2 = ips[2][1];
  PING_IP_ADDR3_O3 = ips[2][2];
  PING_IP_ADDR3_O4 = ips[2][3];
  if (!pingOn && RESTmanLEDsON) LEDsON = true;  // LEDs switched off by the ping monitor come back on
  useresturl = rest ? -1 : 0;
  if (useresturl) startRestServer();  // Without this, enabling REST at runtime crashed the clock (server1 was never created)
  applyUpdateMode(update);

  writeEEPROM();
  if (txtChanged) writeTxtConfig();
  configNTPTime();  // also sets the time zone
  pixels.setBrightness(intensity);

  bool pinsChanged = ledPin != hw.ledPin || sdaPin != hw.sdaPin || sclPin != hw.sclPin;
  if (pinsChanged) {  // Used after the next restart
    hw.ledPin = ledPin;
    hw.sdaPin = sdaPin;
    hw.sclPin = sclPin;
    writeHwConfig();
  }
  JsonDocument res;
  res["ok"] = true;
  res["restart"] = pinsChanged;
  sendJson(200, res);
}

String cleanBootText(const char* in) {  // Boot texts: letters A-Z and spaces only, max. 20 characters
  String out = "";
  for (const char* p = in; *p && out.length() < 20; p++) {
    char ch = toupper((unsigned char)*p);
    if ((ch >= 'A' && ch <= 'Z') || ch == ' ') out += ch;
  }
  return out;
}

void handleApiShowNow() {  // "Show now" buttons: {"what":"date"|"temp"|"hum"}
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) return sendError(400, "invalid JSON");
  String what = doc["what"] | "";
  if (what == "date") {
    triggerShowDateNow = true;
  } else if (what == "temp" || what == "hum") {
    if (!checkHTU()) return sendError(503, "sensor");
    if (what == "temp") triggerShowTempNow = true;
    else triggerShowHumidityNow = true;
  } else {
    return sendError(400, "what");
  }
  JsonDocument res;
  res["ok"] = true;
  sendJson(200, res);
}

void handleApiRestart() {
  sendOk();
  delay(300);
  ClockRestart();
}

void handleApiWifiReset() {
  sendOk();
  delay(300);
  ClockWifiReset();
}

void handleApiCornerTest() {
  sendOk();
  for (int k = 0; k < 4; k++) {
    dunkel();
    pixels.setPixelColor(110 + k, pixels.Color(255, 255, 255));
    pixels.show();
    delay(1000);
  }
  dunkel();
  pixels.show();
}

void handleApiTime() {
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) return sendError(400, "invalid JSON");
  uint32_t epoch = doc["epoch"] | 0UL;
  if ((time_t)epoch < MIN_VALID_TIME) return sendError(400, "epoch");
  timeval tv = { (time_t)epoch, 0 };
  settimeofday(&tv, nullptr);
  rtcWriteTime();
  handleTime();
  Serial.printf("Time set from the web interface: %02d:%02d:%02d\n", iHour, iMinute, iSecond);
  sendOk();
}

void handleApiUpdateCheck() {
  String errorMessage;
  if (!readhttpfile(errorMessage)) return sendError(502, errorMessage.c_str());
  JsonDocument doc;
  doc["ok"] = true;
  doc["latest"] = AvailableVersion;
  doc["available"] = UpdateAvailable;
  sendJson(200, doc);
}

void handleApiUpdate() {
  if (!UpdateAvailable) return sendError(409, "no update available");
  sendOk();
  delay(300);
  installRelease(AvailableVersion);
}


// ###########################################################################################################################################
// # Automatic update from the GitHub releases of this project:
// #
// # The newest release is found through the redirect of https://github.com/<repo>/releases/latest to .../releases/tag/<tag>.
// # The tag is the version (e.g. "V6.1.0"). Each release has to contain the file UPDATE_BIN_NAME (built by the GitHub Action).
// ###########################################################################################################################################
void parseVersion(const String& v, int out[4]) {  // "V6.1.0.1" -> {6, 1, 0, 1}  [FORK] was 3 parts ("V6.1.0" -> {6, 1, 0})
  out[0] = out[1] = out[2] = out[3] = 0;
  int idx = 0;
  long num = 0;
  bool inNum = false;
  for (unsigned int i = 0; i < v.length() && idx < 4; i++) {
    char ch = v[i];
    if (isdigit((unsigned char)ch)) {
      num = num * 10 + (ch - '0');
      inNum = true;
    } else if (ch == '.' && inNum) {
      out[idx++] = num;
      num = 0;
      inNum = false;
    } else if (inNum) {  // [FORK] anything else (f.e. "-beta") ends the number, the rest is ignored
      break;
    }
  }
  if (inNum && idx < 4) out[idx] = num;
}

int compareVersions(const String& a, const String& b) {  // > 0 if a is newer than b
  int va[4], vb[4];  // [FORK] 4 parts instead of 3, so the fork can number its own releases (V6.1.0.1 is newer than V6.1.0)
  parseVersion(a, va);
  parseVersion(b, vb);
  for (int i = 0; i < 4; i++)
    if (va[i] != vb[i]) return va[i] > vb[i] ? 1 : -1;
  return 0;
}

void installRelease(const String& tag) {  // Download and install the update file of the release
  Serial.println("Update: installing release " + tag + " (" UPDATE_BIN_NAME ")");
  WiFiClientSecure client;
  client.setInsecure();
  ESPhttpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // GitHub redirects downloads to its file server
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  String url = "https://github.com/" UPDATE_REPO "/releases/download/" + tag + "/" UPDATE_BIN_NAME;
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      ESP.restart();  // update_error() changed the colors and showed RESET, so start clean
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}


// ###########################################################################################################################################
// # Check for available updates:
// ###########################################################################################################################################
// Reads the newest version from GitHub into AvailableVersion. UpdateAvailable is set if it is newer than the running version.
bool readhttpfile(String& errorMessage) {
#if defined(ESP32)
  bool enoughMemory = ESP.getMaxAllocHeap() > 40000;
#else
  bool enoughMemory = ESP.getMaxFreeBlockSize() > 24000;  // The HTTPS connection needs about 22 KB in one block
#endif
  if (!enoughMemory) {
    errorMessage = "memory";
    Serial.println("AUTOMATIC UPDATE: not enough free memory for the HTTPS connection to GitHub");
    return false;
  }
  errorMessage = "github";
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure();  // No certificate check (the update server used before was plain HTTP)
  HTTPClient http;
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  http.setTimeout(10000);
  if (!http.begin(wifiClient, "https://github.com/" UPDATE_REPO "/releases/latest")) return false;
  const char* headerKeys[] = { "Location" };
  http.collectHeaders(headerKeys, 1);
  int httpCode = http.GET();
  String response = http.header("Location");  // .../releases/tag/<version>
  http.end();
  int pos = response.lastIndexOf("/tag/");
  if ((httpCode != 301 && httpCode != 302) || pos < 0) {
    Serial.println("AUTOMATIC UPDATE: Error response (" + String(httpCode) + ")");
    return false;
  }
  String version = response.substring(pos + 5);
  version.trim();
  if (version.length() == 0) return false;
  AvailableVersion = version;
  UpdateAvailable = compareVersions(AvailableVersion, WORD_CLOCK_VERSION) > 0;
  Serial.println("AUTOMATIC UPDATE: newest version on GitHub: " + AvailableVersion);
  errorMessage = "";
  return true;
}


// ###########################################################################################################################################
// # EOF - You have successfully reached the end of the code - well done ;-)
// ###########################################################################################################################################