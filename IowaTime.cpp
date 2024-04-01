#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TM1637Display.h>
#include <TimeLib.h>
#include <EthernetUdp.h>

// Replace these values with your network settings
byte mac[] = { 0xDE, 0xCE, 0xDF, 0xFF, 0xFE, 0xED };  // MAC address
IPAddress ip(192, 168, 1, 180);                        // Arduino's IP address
IPAddress timeServer(129, 6, 15, 28);                   // time.nist.gov NTP server
EthernetUDP Udp;

// Define the pins for TM1637
#define CLK_PIN   6  // CLK pin
#define DIO_PIN   7  // DIO pin
TM1637Display display(CLK_PIN, DIO_PIN);

// Define LCD parameters
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 20
#define LCD_ROWS 4
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

void setup() {
  Serial.begin(9600);
  
  // Initialize Ethernet and LCD
  Ethernet.begin(mac, ip);
  Udp.begin(8888);
  lcd.init();
  lcd.backlight();

  // Initialize TM1637 display
  display.setBrightness(0x0f);  // set brightness to maximum
}

void loop() {
  // Update and display time
  time_t currentTime = getNTPTime();
  updateDisplay(currentTime);
  
  // Display network information on LCD
  displayNetworkInfo();
  
  delay(1000); // update display every second
}

time_t getNTPTime() {
  byte packetBuffer[48]; // buffer to hold incoming and outgoing packets
  memset(packetBuffer, 0, 48); // Initialize buffer

  // Set the first byte of the MAC address to 0xFF to indicate broadcast
  memset(packetBuffer, 0, 48);
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // All 48 bytes have been written. Now we can send the packet
  Udp.beginPacket(timeServer, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, 48);
  Udp.endPacket();

  // Wait for response
  delay(1000);  // Wait to receive the response

  if (Udp.parsePacket()) {
    Udp.read(packetBuffer, 48); // read the packet into the buffer

    // The timestamp starts at byte 40 of the received packet and is four bytes long.
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;

    return epoch - 5*3600; // UTC  adjustment
  }
  return 0; // return 0 if unable to get the time
}

void updateDisplay(time_t time) {
  if (time != 0) {
    tmElements_t tm;
    breakTime(time, tm);
    
    // Format the time to HH:MM
    int hours = tm.Hour;
    int minutes = tm.Minute;

    // Display the time on the TM1637 display
    display.showNumberDecEx(hours * 100 + minutes, 0b01000000, true); // Colon on
  }
}

void displayNetworkInfo() {
  // Display IP address on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP Address:");
  lcd.setCursor(0, 1);
  lcd.print(Ethernet.localIP());
  lcd.setCursor(0, 2);
  lcd.print("Iowa Time:");

  // Measure ping time to google.com
  int pingTime = measurePingTime();
  lcd.setCursor(0, 3);
  if (pingTime >= 0) {
    lcd.print(pingTime);
    lcd.print(" ms");
  } else {
    lcd.print("Failed");
  }
}

int measurePingTime() {
  EthernetClient client;
  if (client.connect("www.kcrg.com", 80)) {
    client.println("GET / HTTP/1.1");
    client.println("Host: www.kcrg.com");
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
