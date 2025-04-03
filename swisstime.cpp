// Arduino Uno - Clock for multiple Timezones/Cities
// Daniel Mitchell - 2025 Apr 03
// Arduino Uno
// LCD 2004A display
// TM1637 4x7segmet display
// Ethernet Hat
//
// This project displays *Daylight-Savings Time, Date, Day of the week, TimeZone for 5 major cities
// the display cycles through each city information with a delay (3 seconds)
//

#include <SPI.h>
#include <Ethernet.h>           // Ethernet by Various
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // by Frank de Brabander
#include <TM1637Display.h>      // by Avishay Orpaz
#include <TimeLib.h>            // Arduino Time Lib by Michael Margolis
#include <EthernetUdp.h>

// Network settings
byte mac[] = { 0xDE, 0xCD, 0xDE, 0xFF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 181);
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov
EthernetUDP Udp;

// TM1637 display
#define CLK_PIN   6
#define DIO_PIN   7
TM1637Display display(CLK_PIN, DIO_PIN);

// LCD setup
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 20
#define LCD_ROWS 4
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  Udp.begin(8888);
  lcd.init();
  lcd.backlight();
  display.setBrightness(0x0f);
}

void loop() {
  time_t utcTime = getNTPTime();

  displayTimeZone("BERN", utcTime, 1);           // CET/CEST
  displayTimeZone("BUCHAREST", utcTime, 2);      // EET/EEST
  displayTimeZone("CHICAGO", utcTime, -6);       // CST/CDT
  displayTimeZone("BOSTON", utcTime, -5);        // EST/EDT
  displayTimeZone("LONDON", utcTime, 0);         // GMT/BST
}

time_t getNTPTime() {
  byte packetBuffer[48];
  memset(packetBuffer, 0, 48);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  Udp.beginPacket(timeServer, 123);
  Udp.write(packetBuffer, 48);
  Udp.endPacket();

  delay(1000);

  if (Udp.parsePacket()) {
    Udp.read(packetBuffer, 48);
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
    return epoch;
  }
  return 0;
}

// Compute local time for a given UTC time and base offset
// Handles DST for major regions
bool isDST(int year, int month, int day, int hour, int weekday, const char* zone) {
  if (strcmp(zone, "Europe") == 0) {
    if (month < 3 || month > 10) return false;
    if (month > 3 && month < 10) return true;
    int lastSunday = 31 - ((5 + weekday + (year * 5 / 4 + 1)) % 7);
    if (month == 3) return (day > lastSunday || (day == lastSunday && hour >= 2));
    if (month == 10) return !(day > lastSunday || (day == lastSunday && hour >= 3));
  } else if (strcmp(zone, "America") == 0) {
    if (month < 3 || month > 11) return false;
    if (month > 3 && month < 11) return true;
    int sundayOffset = (weekday + 6) % 7;
    if (month == 3) return (day > (8 - sundayOffset) || (day == (8 - sundayOffset) && hour >= 2));
    if (month == 11) return !(day > (1 + (7 - sundayOffset) % 7) || (day == (1 + (7 - sundayOffset) % 7) && hour >= 2));
  }
  return false;
}

time_t adjustTimeForZone(time_t utc, int baseOffset, const char* zone) {
  tmElements_t tm;
  breakTime(utc + baseOffset * SECS_PER_HOUR, tm);
  if (isDST(tm.Year + 1970, tm.Month, tm.Day, tm.Hour, weekday(utc + baseOffset * SECS_PER_HOUR), zone)) {
    return utc + (baseOffset + 1) * SECS_PER_HOUR;
  } else {
    return utc + baseOffset * SECS_PER_HOUR;
  }
}

void displayTimeZone(const char* cityName, time_t utc, int baseOffset) {
  const char* region = (strcmp(cityName, "Chicago") == 0 || strcmp(cityName, "Boston") == 0) ? "America" : "Europe";
  time_t local = adjustTimeForZone(utc, baseOffset, region);

  tmElements_t tm;
  breakTime(local, tm);
  bool dstActive = isDST(tm.Year + 1970, tm.Month, tm.Day, tm.Hour, weekday(local), region);

  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  const char* monthStr = months[tm.Month - 1];

  char dateStr[20];
  snprintf(dateStr, sizeof(dateStr), "%02d %s %04d", tm.Day, monthStr, tm.Year + 1970);

  updateDisplay(local);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(cityName);
  lcd.setCursor(0, 1);
  lcd.print(dateStr);
  lcd.setCursor(0, 2);
  lcd.print(dstActive ? "DST: Active " : "DST: Inactive");

  const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  const char* dayName = days[weekday(local) - 1];
  const char* tzAbbr;
  if (strcmp(cityName, "BERN") == 0)
    tzAbbr = dstActive ? "CEST" : "CET";
  else if (strcmp(cityName, "BUCHAREST") == 0)
    tzAbbr = dstActive ? "EEST" : "EET";
  else if (strcmp(cityName, "CHICAGO") == 0)
    tzAbbr = dstActive ? "CDT" : "CST";
  else if (strcmp(cityName, "BOSTON") == 0)
    tzAbbr = dstActive ? "EDT" : "EST";
  else if (strcmp(cityName, "LONDON") == 0)
    tzAbbr = dstActive ? "BST" : "GMT";
  else
    tzAbbr = "UNK";

  lcd.setCursor(0, 3);
  lcd.print(dayName);
  lcd.print(" ");
  lcd.print(tzAbbr);
  delay(3000);
}

void updateDisplay(time_t time) {
  if (time != 0) {
    tmElements_t tm;
    breakTime(time, tm);
    int hours = tm.Hour;
    int minutes = tm.Minute;
    display.showNumberDecEx(hours * 100 + minutes, 0b01000000, true);
  }
}

void displayNetworkInfo() {
  // Deprecated in new design
}

int measurePingTime() {
  EthernetClient client;
  if (client.connect("www.swisscom.com", 80)) {
    client.println("GET / HTTP/1.1");
    client.println("Host: www.google.com");
    client.println("Connection: close");
    client.println();
    long startMillis = millis();
    while (client.connected()) {
      if (client.available()) {
        client.readStringUntil('\n');
        break;
      }
    }
    long endMillis = millis();
    client.stop();
    return endMillis - startMillis;
  }
  return -1;
}

