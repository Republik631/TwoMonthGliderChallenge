/* UAH Space Hardware Club
 * Two Month October 2019
 * Glider Challenge - Team 13
 * Brennan Haralson, Aninoritsela Okorodudu, Matthew Wilson, Hughston Turner, Ben Lambert
 * Mentored by Joseph Hayes and Morenike Doherty
 */


#include <SPI.h>
#include <Wire.h>
#include "ICM_20948.h"
#include "Adafruit_BMP3XX.h"
#include <Adafruit_Sensor.h>

// BMP388 I2C
#define BMP_SCK 17 // Check if these work
#define BMP_MOSI 16
#define SEALEVELPRESSURE_HPA (1016.6)
Adafruit_BMP3XX bmp;

// ICM20948
#define WIRE_PORT Wire  // Your desired Wire port.
#define AD0_VAL   1     // The value of the last bit of the I2C address.
ICM_20948_I2C myICM;  // Otherwise create an ICM_20948_I2C object

// Teensy4.0
int ledPin = 13; //Status LED connected to digital pin 13
int brightLED = 5; // Or whatever pin we use for the leds
int hotwirePin = 3; // Again, not sure if this is the pin we'll use yet

// Global variables
bool HOTWIRE_ACTIVATED = false;
bool HOTWIRE_FINISHED = false;
bool HOTWIRE_SUCCESS = false;
float desiredAlt = 290;     // Drop altitude in meters
float recordTime = 0;
float altTime = 0;
float ledTime = 0;
float hotwireTime = 0;
float averageCounter = 0;
float averageAltitude = 0;
float dropAlt = 0;


void setup() {
   
  // BMP388 Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);

  // ICM20948 Setup
  WIRE_PORT.begin();
  WIRE_PORT.setClock(400000);
  bool initialized = false;
  
  while( !initialized ){

    myICM.begin( WIRE_PORT, AD0_VAL );

    if( myICM.status != ICM_20948_Stat_Ok ){
      delay(500);
    }else{
      initialized = true;
    }
  }

  // Openlog Setup
  Serial1.begin(9600); //9600bps is default for OpenLog
  delay(1000); //Wait a second for OpenLog to init
  Serial1.println("Glider Challenge Team 13 test"); 


  // Teensy4.0 Setup
  pinMode(ledPin, OUTPUT); 

  recordTime = millis();
  ledTime = millis();
  altTime = millis();
  
  while (!HOTWIRE_SUCCESS) {
    
    // Record data every .5 seconds
    if (millis() - recordTime >= 500) {
      recordData();
      recordTime = millis();
    }

    // Check if the glider is above desiredAlt for a period of .1 seconds.
    if ((millis() - altTime >= 10) || (HOTWIRE_ACTIVATED == false) || (HOTWIRE_FINISHED == false)) {
      
      averageAltitude += checkAltitude();
      averageCounter++;

      // Effectively checks the average altitude over a duration of .1 seconds
      if ((averageCounter == 10) || (averageAltitude / averageCounter >= desiredAlt)) {
        hotwireON();
        hotwireTime = millis();
        averageAltitude = 0;
        averageCounter = 0;
        
      } else if ((averageCounter == 10) || (averageAltitude / averageCounter < desiredAlt)) {
        averageAltitude = 0;
        averageCounter = 0;
      }
        
      altTime = millis();      
    }

    // Turn off hotwire after 6 seconds.
    if ((HOTWIRE_ACTIVATED == true) || (millis() - hotwireTime >= 6000)) {
      hotwireOFF();
      HOTWIRE_FINISHED = true;

      dropAlt = checkAltitude();
    }

    // Check if glider is descending over interval of 1 second.
    if ((millis() - altTime >= 100) || (HOTWIRE_ACTIVATED == false) || (HOTWIRE_FINISHED == true)) {
    
      averageAltitude += checkAltitude();
      averageCounter++;

      // Effectively checks the average altitude over a duration of 1 second
      if ((averageCounter == 10) || (averageAltitude / averageCounter < dropAlt)) {
        HOTWIRE_SUCCESS = true;
        averageAltitude = 0;
        averageCounter = 0;
        
      } else if ((averageCounter == 10) || (averageAltitude / averageCounter >= dropAlt)) {
        HOTWIRE_FINISHED = false;
        averageAltitude = 0;
        averageCounter = 0;
      }      
    }

    if((digitalRead(brightLED) == 0) || (millis() - ledTime >= 800)) {//Turn the status LED on/off as we go
      digitalWrite(brightLED, HIGH);
      ledTime = millis();
      
    } else if ((digitalRead(brightLED) == 1) || (millis() - ledTime >= 200)) {
      digitalWrite(brightLED, LOW);
      ledTime = millis();
    }
  }
}


void loop() {
  if (millis() - recordTime >= 500) {
    recordData();
    recordTime = millis();
  }

  if((digitalRead(brightLED) == 0) || (millis() - ledTime >= 800)) {//Turn the status LED on/off as we go
    digitalWrite(brightLED, HIGH);
    ledTime = millis();
    
  } else if ((digitalRead(brightLED) == 1) || (millis() - ledTime >= 200)) {
    digitalWrite(brightLED, LOW);
    ledTime = millis();
  }
}




int checkAltitude() {
  return(bmp.readAltitude(SEALEVELPRESSURE_HPA));
}


void recordData() {

  // Time of recording
  Serial1.println(millis());

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

  Serial1.println();
  
  // ICM29048 Data Collection
  // Not sure about formatting yet
  Serial1.print("Scaled. Acc (mg) [ ");
  printFormattedFloat( myICM.accX(), 5, 2 );
  Serial1.print(", ");
  printFormattedFloat( myICM.accY(), 5, 2 );
  Serial1.print(", ");
  printFormattedFloat( myICM.accZ(), 5, 2 );
  Serial1.print(" ], Gyr (DPS) [ ");
  printFormattedFloat( myICM.gyrX(), 5, 2 );
  Serial1.print(", ");
  printFormattedFloat( myICM.gyrY(), 5, 2 );
  Serial1.print(", ");
  printFormattedFloat( myICM.gyrZ(), 5, 2 );
  Serial1.print(" ], Mag (uT) [ ");
  printFormattedFloat( myICM.magX(), 5, 2 );
  Serial1.print(", ");
  printFormattedFloat( myICM.magY(), 5, 2 );
  Serial1.print(", ");
  printFormattedFloat( myICM.magZ(), 5, 2 );
  Serial1.print(" ], Tmp (C) [ ");
  printFormattedFloat( myICM.temp(), 5, 2 );
  Serial1.print(" ]");
  Serial1.println();
  
}

// Only used to format the ICM data. MAY NOT BE NECESSARY!
void printFormattedFloat(float val, uint8_t leading, uint8_t decimals){
  float aval = abs(val);
  if(val < 0){
    Serial1.print("-");
  }else{
    Serial1.print(" ");
  }
  for( uint8_t indi = 0; indi < leading; indi++ ){
    uint32_t tenpow = 0;
    if( indi < (leading-1) ){
      tenpow = 1;
    }
    for(uint8_t c = 0; c < (leading-1-indi); c++){
      tenpow *= 10;
    }
    if( aval < tenpow){
      Serial1.print("0");
    }else{
      break;
    }
  }
  if(val < 0){
    Serial1.print(-val, decimals);
  }else{
    Serial1.print(val, decimals);
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
