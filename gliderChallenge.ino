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
#define SEALEVELPRESSURE_HPA (1014.2)
Adafruit_BMP3XX bmp;

// ICM20948
#define WIRE_PORT Wire  // Your desired Wire port.
#define AD0_VAL   1     // The value of the last bit of the I2C address.
ICM_20948_I2C myICM;  // Otherwise create an ICM_20948_I2C object

// Teensy4.0
int brightLED = 13; // Or whatever pin we use for the leds
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
  
  Serial.begin(115200);
  while (!Serial);
  Serial.println("BMP388 test");

  if (!bmp.begin(0x77)) {
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    while (1);
  }
  // BMP388 Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);

while(!Serial){};
  // ICM20948 Setup
  WIRE_PORT.begin(0x68);
  WIRE_PORT.setClock(400000);
  bool initialized = false;
  
  while( !initialized ){

    myICM.begin( WIRE_PORT, AD0_VAL );

    Serial.print( F("Initialization of the sensor returned: ") );
    Serial.println( myICM.statusString() );
    if( myICM.status != ICM_20948_Stat_Ok ){
      Serial.println( "Trying again..." );
      delay(500);
    }else{
      initialized = true;
    }

  }

  // Openlog Setup
  Serial.begin(9600); //9600bps is default for OpenLog
  delay(1000); //Wait a second for OpenLog to init
  Serial.println("Glider Challenge Team 13 test"); 


  // Teensy4.0 Setup
  pinMode(brightLED, OUTPUT); 

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
    if ((millis() - altTime >= 50) && (HOTWIRE_ACTIVATED == false) && (HOTWIRE_FINISHED == false)) {
      
      averageAltitude += checkAltitude();
      averageCounter++;

      // Effectively checks the average altitude over a duration of .5 seconds
      if ((averageCounter == 10) && (averageAltitude / averageCounter >= desiredAlt)) {
        hotwireON();
        hotwireTime = millis();
        averageAltitude = 0;
        averageCounter = 0;
        
      } else if ((averageCounter == 10) && (averageAltitude / averageCounter < desiredAlt)) {
        averageAltitude = 0;
        averageCounter = 0;
      }
        
      altTime = millis();      
    }

    // Turn off hotwire after 6 seconds.
    if ((HOTWIRE_ACTIVATED == true) && (millis() - hotwireTime >= 6000)) {
      hotwireOFF();
      HOTWIRE_FINISHED = true;

      dropAlt = checkAltitude();
    }

    // Check if glider is descending over interval of 1 second.
    if ((millis() - altTime >= 100) && (HOTWIRE_ACTIVATED == false) && (HOTWIRE_FINISHED == true)) {
    
      averageAltitude += checkAltitude();
      averageCounter++;

      // Effectively checks the average altitude over a duration of 1 second
      if ((averageCounter == 10) && (averageAltitude / averageCounter < dropAlt)) {
        HOTWIRE_SUCCESS = true;
        averageAltitude = 0;
        averageCounter = 0;
        
      } else if ((averageCounter == 10) && (averageAltitude / averageCounter >= dropAlt)) {
        HOTWIRE_FINISHED = false;
        averageAltitude = 0;
        averageCounter = 0;
      }      
    }

    blinkLED();
    
  }
}


void loop() {
  if (millis() - recordTime >= 500) {
    recordData();
    recordTime = millis();
  }

  blinkLED();
}




float checkAltitude() {
  return(bmp.readAltitude(SEALEVELPRESSURE_HPA));
}


void recordData() {
  // Time of recording
  Serial.print("Elapsed Time: ");
  Serial.println(millis()-recordTime);

  // BMP388 Data Collection
  Serial.print("Temperature = ");
  Serial.print(bmp.temperature);
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bmp.pressure / 100.0);
  Serial.println(" hPa");

  Serial.print("Altitude = ");
  Serial.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.println();


  ICM_20948_AGMT_t agmt = myICM.getAGMT();
  // ICM29048 Data Collection
  // Not sure about formatting yet
  Serial.print("Scaled. Acc (mg) [ ");
  printFormattedFloat( myICM.accX(), 5, 2 );
  Serial.print(", ");
  printFormattedFloat( myICM.accY(), 5, 2 );
  Serial.print(", ");
  printFormattedFloat( myICM.accZ(), 5, 2 );
  Serial.print(" ], Gyr (DPS) [ ");
  printFormattedFloat( myICM.gyrX(), 5, 2 );
  Serial.print(", ");
  printFormattedFloat( myICM.gyrY(), 5, 2 );
  Serial.print(", ");
  printFormattedFloat( myICM.gyrZ(), 5, 2 );
  Serial.print(" ], Mag (uT) [ ");
  printFormattedFloat( myICM.magX(), 5, 2 );
  Serial.print(", ");
  printFormattedFloat( myICM.magY(), 5, 2 );
  Serial.print(", ");
  printFormattedFloat( myICM.magZ(), 5, 2 );
  Serial.print(" ], Tmp (C) [ ");
  printFormattedFloat( myICM.temp(), 5, 2 );
  Serial.print(" ]");
  Serial.println();
  
}

// Only used to format the ICM data. MAY NOT BE NECESSARY!
void printFormattedFloat(float val, uint8_t leading, uint8_t decimals){
  float aval = abs(val);
  if(val < 0){
    Serial.print("-");
  }else{
    Serial.print(" ");
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
      Serial.print("0");
    }else{
      break;
    }
  }
  if(val < 0){
    Serial.print(-val, decimals);
  }else{
    Serial.print(val, decimals);
  }
}

// Blink LED
void blinkLED() {
  if((digitalRead(brightLED) == LOW) && (millis() - ledTime >= 1200)) {//Turn the status LED on/off as we go
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
