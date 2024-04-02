Arduino Uno NTP clock and display
---------------------------------

2 April 2024

The code:
* Loads required libraries 
* Connects to the local ethernet network

In a loop:
* Retrieves NTP time from a server
* Displays the time on the TM1637 LED module in HH:MM format
* Displays the device IP address and ping time to a programmable website of choice

Project Hardware:
-----------------
* Arduino Uno 
* W5100 Ethernet Shield for Arduino uno
* 2004a LCD - 5V 20X4 2004 20x4 Character LCD Display
* TM1637 LED Display Module for Arduino 7 Segment x 4 digit
* (dupont jumper wires)
* (mounting hardware) 
* (ethernet cable) 
* (USB type B cable - power and PC connection) 
* The LCD screen uses I2C via the SDA/SCL ports on the arduino.

The TM1637 LED module uses pins 6 & 7 (programmable) on the Arduino 



