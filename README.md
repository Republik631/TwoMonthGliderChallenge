# TwoMonthGliderChallenge
Team 13's repository of glider stuff.

The two zipped folders are necessary libraries. Download them, extract them, and put them in Documents > Arduino > Libraries.
Restart the Arduino software and they should be installed. You can verify this by checking at the bottom of the list of examples (File > Examples). The libraries are linked below and included in the repo, but may not be the newest versions:
 
[BMP388 Library](Adafruit_BMP3XX_Library.zip)

[ICM-20948 Library](SparkFun_9DoF_IMU_Breakout_-_ICM_20948_-_Arduino_Library.zip)
  
[gliderChallenge_flight_v1.ino](gliderChallenge_flight_v1.ino) is the main program. It should be put in a Documents > Arduino > gliderChallenge_flight_v1, and can then be opened.

The pins used are as follows:
- GND - ground (all components)
- Vin - 5V input (from 5v regulator)
- 3V  - 3.3V output (openlog, bmp388, icm20948)
- 0   - RX1 (openlog)
- 1   - TX1 (openlog)
- 2   - PWM (LED MOSFET)
- 4   - PWM (hotwire MOSFET)
- 18  - SDA (bmp388, icm20948)
- 19  - SCL (bmp388, icm20948)
