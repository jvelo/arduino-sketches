#include "LedControl.h"
#include <dht.h>
#include <SFE_BMP180.h>
#include <Wire.h>

dht DHT;
SFE_BMP180 bmp180;

#define DHT22_PIN 3

#define ALTITUDE 252.0 // Croix rousse

// Arduino Pin 7 to DIN, 6 to Clk, 5 to LOAD, no.of devices is 1
LedControl lc=LedControl(7,6,5,1);

int counter = 1;

void setup() {
 // Initialize  device
  lc.shutdown(0,false);
  lc.setIntensity(0,5); // 0 to 15
  lc.clearDisplay(0);
  
  Serial.begin(9600);
  Serial.println("--- Booting ---");
  
  if (!bmp180.begin()) {
    Serial.println("BMP180 init fail\n\n");
  }
}

void loop() {

  char bmpStatus;
  double T,P,p0,a;
  
  int chk = DHT.read22(DHT22_PIN);
  switch (chk)
  {
    case DHTLIB_OK: 
      printChar('t', 7);
      printValue(DHT.temperature, 6);
      printChar('h', 3);
      printValue(DHT.humidity, 2);
      break;
    case DHTLIB_ERROR_CHECKSUM: 
      Serial.print("DHT Checksum error,\t"); 
      break;
    case DHTLIB_ERROR_TIMEOUT: 
      Serial.print("DHT Time out error,\t"); 
      break;
    default: 
      Serial.print("DHT Unknown error,\t"); 
      break;
  }
  
  bmpStatus = bmp180.startTemperature();
  if (bmpStatus != 0) {
    delay(bmpStatus);
    
    bmpStatus = bmp180.getTemperature(T);
    if (bmpStatus != 0) {
      // Print out the measurement:
      Serial.print("temperature: ");
      Serial.print(T,2);
      Serial.println(" deg C");
    }
    else {
      Serial.println("BMP Error retrieving temperature measurement\n");
    }
  }
  else {
    Serial.println("BMP Error starting temperature measurement\n");
  }
  
  bmpStatus = bmp180.startPressure(3);
  if (bmpStatus != 0) {
     delay(bmpStatus);
     bmpStatus = bmp180.getPressure(P,T);
     if (bmpStatus != 0) {
          // Print out the measurement:
          Serial.print("absolute pressure: ");
          Serial.print(P,2);
          Serial.println(" mb");
     }
     else {
       Serial.println("BMP Error retrieving pressure measurement\n");
     }
  }
  else {
    Serial.println("BMP error starting pressure measurement");
  }

  delay(1000);
}

void printValue(double value, int l) {
  int v = (int) (value * 10);
  Serial.println(v % 10);
    int i = l - 2;
    while (i <= l) {
        lc.setDigit(0,i,v % 10, i == l - 1);
        v= v / 10;
        i++;
    }
}

void printChar(char c, int i) {
    switch (c) {
        case 't':
             printT(i);
             break;
        default:          
            lc.setChar(0,i,c, false);  
            break;
    }
}

void printNumber(int v) {
    if(v > 99999999 || v < 0) {
       return;
    }
    int i = 0;
    while (i < 8) {
      lc.setDigit(0,i,(byte)v % 10,false);
      v= v / 10;
      i++;
    }
}

void printT(int i) {
     lc.setLed(0,i,7,true); // T bar
     lc.setLed(0,i,5,true); // lower left
     lc.setLed(0,i,4,true); // bottom
     lc.setLed(0,i,6,true); // top left
}
