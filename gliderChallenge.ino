/* UAH Space Hardware Club
 * Two Month October 2019
 * Glider Challenge - Team 13
 * Brennan Haralson, Aninoritsela Okorodudu, Matthew Wilson, Hughston Turner, Ben Lambert
 * Mentored by Joseph Hayes and Morenike Doherty
 */
#include <Wire.h>
#include "ICM_20948.h"
#include "Adafruit_BMP3XX.h"
#include <Adafruit_Sensor.h>

// BMP388 I2C
#define SEALEVELPRESSURE_HPA (1012.0) // Must be updated the day flight.
Adafruit_BMP3XX bmp;   // Create a Adafruit_BMP3XX object.

// ICM20948
#define WIRE_PORT Wire  // Your desired Wire port.
#define AD0_VAL   1     // The value of the last bit of the I2C address.
ICM_20948_I2C myICM;  // Create an ICM_20948_I2C object.

// Teensy4.0
int brightLED = 2; // Or whatever pin we use for the leds
int hotwirePin = 3; // Again, not sure if this is the pin we'll use yet

// Global variables
bool HOTWIRE_ACTIVATED = false;        // State of power to hotwire
bool HOTWIRE_FINISHED = false;         // States whether the hotwire has been run
bool HOTWIRE_SUCCESS = false;          // State of succes of the hotwire burn
float localAlt = 200;                  // Local elevation
float desiredAlt = localAlt + 290;     // Drop altitude in meters
float recordTime = 0;                  // Time since last recording
float altTime = 0;                     // Time since last altitude reading
float ledTime = 0;                     // Time since last LED state change
float hotwireTime = 0;                 // Time since hotwire was activated
float averageCounter = 0;              // Accumulator to track number of altitude readings taken
float averageAltitude = 0;             // Accumulator used for finding average altitude
float dropAlt = 0;                     // Altitude at whih hotwire is deactivated


void setup() {

  // Openlog Setup
  Serial1.begin(9600); //9600bps is default for OpenLog
  delay(1000); //Wait a second for OpenLog to init
  Serial1.println("GLIDER CHALLENGE TEAM 13");
  Serial1.println("Data is collected every 500 milliseconds.");
  Serial1.println();

  // BMP388 Initialization
  Serial1.print("BMP388 initialization: ");
  if (!bmp.begin(0x77)) {
    Serial1.println("FAILED!");
    while (1);
  }
  Serial1.println("SUCCESS!");
  
  // BMP388 Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);

  // ICM20948 Initialization
  WIRE_PORT.begin(0x68);
  WIRE_PORT.setClock(400000);

  Serial1.print("ICM20948 initialization: ");
  myICM.begin( WIRE_PORT, AD0_VAL );
  if( myICM.status != ICM_20948_Stat_Ok ){
    Serial1.println("FAILED!");
    while(1);
  }
  Serial1.println("SUCCESS!");   
  Serial1.println();


  // Teensy4.0 Setup
  pinMode(brightLED, OUTPUT); 

  // Begin timers
  recordTime = millis();
  ledTime = millis();
  altTime = millis();

  // This loop manages all events up to and including a successful hotwire activation.
  while (!HOTWIRE_SUCCESS) {
    
    // Record data every .5 seconds
    if (millis() - recordTime >= 500) {
      recordData();
      recordTime = millis();
    }

    // Check if the glider is above desiredAlt for a period of .1 seconds.
    if ((millis() - altTime >= 50) && (HOTWIRE_ACTIVATED == false) && (HOTWIRE_FINISHED == false)) {

      // Add the current altitude to the accumulator and increment the counter.
      averageAltitude += checkAltitude();
      averageCounter++;

      // Effectively checks the average altitude over a duration of .5 seconds.
      if ((averageCounter == 10) && (averageAltitude / averageCounter >= desiredAlt)) {
        // If the altitude is above desiredAlt, activate the hotwire and begin the hotwire timer.
        hotwireON();
        hotwireTime = millis();
        // Reset acumulators (they are used in the next function).
        averageAltitude = 0;
        averageCounter = 0;

      // Reset the average altitude counter if the desired altitude is not reached over an interval of .5 seconds
      } else if ((averageCounter == 10) && (averageAltitude / averageCounter < desiredAlt)) {
        averageAltitude = 0;
        averageCounter = 0;
      }

      // Reset the altTime counter
      altTime = millis();      
    }

    // Chekc the state of the hotwire and turn it off after 6 seconds.
    if ((HOTWIRE_ACTIVATED == true) && (millis() - hotwireTime >= 6000)) {
      hotwireOFF();
      HOTWIRE_FINISHED = true;
      
      // Record the altitude at which the hotwire is deactivated
      dropAlt = checkAltitude();
    }

    // Check if glider is descending over interval of 1 second.
    if ((millis() - altTime >= 100) && (HOTWIRE_ACTIVATED == false) && (HOTWIRE_FINISHED == true)) {

      // Add the current altitude to the accumulator and increment the counter.
      averageAltitude += checkAltitude();
      averageCounter++;

      // Effectively checks the average altitude over a duration of 1 second
      if ((averageCounter == 10) && (averageAltitude / averageCounter < dropAlt)) {
        // If the average altitude over an interval of 1 second is less than the drop altitude, 
        // then the hotwire burn was successful.
        HOTWIRE_SUCCESS = true;
        // Reset acumulators
        averageAltitude = 0;
        averageCounter = 0;
        
      } else if ((averageCounter == 10) && (averageAltitude / averageCounter >= dropAlt)) {
        // If the average altitude over an interval of 1 second is greater than the drop altitude, 
        // then the hotwire burn failed. Reset the state variables (so another burn can be done).
        HOTWIRE_FINISHED = false;
        // Reset acumulators
        averageAltitude = 0;
        averageCounter = 0;
      }      
    }

    // Blinks the LED on intervals specified in the blinkLED function.
    blinkLED();
    
  }
}

// After a successful hotwire burn, the loop function takes over. 
void loop() {

  // Record data every .5 seconds
  if (millis() - recordTime >= 500) {
    recordData();
    recordTime = millis();
  }

  // Blinks the LED on intervals specified in the blinkLED function.
  blinkLED();
}



// Check the current altitude.
float checkAltitude() {
  return(bmp.readAltitude(SEALEVELPRESSURE_HPA));
}

// Write data out to the SD card.
void recordData() {

  // BMP388 Data Collection
  Serial1.print("Temperature = ");
  Serial1.print(bmp.temperature);
  Serial1.println(" *C");

  Serial1.print("Pressure = ");
  Serial1.print(bmp.pressure / 100.0);
  Serial1.println(" hPa");

  Serial1.print("Altitude = ");
  Serial1.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
  Serial1.println(" m");

  myICM.getAGMT();
  // ICM29048 Data Collection
  // Not sure about formatting yet
  Serial1.print("Scaled. Acc (mg) [ ");
  Serial1.print(myICM.accX());
  Serial1.print(", ");
  Serial1.print(myICM.accY());
  Serial1.print(", ");
  Serial1.print(myICM.accZ());
  Serial1.print(" ], Gyr (DPS) [ ");
  Serial1.print(myICM.gyrX());
  Serial1.print(", ");
  Serial1.print(myICM.gyrY());
  Serial1.print(", ");
  Serial1.print(myICM.gyrZ());
  Serial1.print(" ], Mag (uT) [ ");
  Serial1.print(myICM.magX());
  Serial1.print(", ");
  Serial1.print(myICM.magY());
  Serial1.print(", ");
  Serial1.print(myICM.magZ());
  Serial1.print(" ], Tmp (C) [ ");
  Serial1.print(myICM.temp());
  Serial1.println(" ]");

  // Time of recording
  Serial1.print("Elapsed time since last recording: ");
  Serial1.println(millis()-recordTime);
  Serial1.println();
  
}

// Blink LED on for 400 milliseconds, off for 1200 milliseconds.
void blinkLED() {
  if((digitalRead(brightLED) == LOW) && (millis() - ledTime >= 1200)) {
    digitalWrite(brightLED, HIGH);
    ledTime = millis();
    
  } else if ((digitalRead(brightLED) == HIGH) && (millis() - ledTime >= 400)) {
    digitalWrite(brightLED, LOW);
    ledTime = millis();
  }
}


// Pass voltage across the MOSFET control pin to turn on the hotwire.
void hotwireON() {
  digitalWrite(hotwirePin, HIGH);
  HOTWIRE_ACTIVATED = true;
}

// Turn off the MOSFET control pin for the hotwire.
void hotwireOFF() {
  digitalWrite(hotwirePin, LOW);
  HOTWIRE_ACTIVATED = false;
}
