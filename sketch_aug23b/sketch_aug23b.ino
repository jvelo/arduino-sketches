#include "LedControl.h"
#include <dht.h>

dht DHT;

#define DHT22_PIN 3

// Arduino Pin 7 to DIN, 6 to Clk, 5 to LOAD, no.of devices is 1
LedControl lc=LedControl(7,6,5,1);

int counter = 1;

void setup() {
 // Initialize  device
  lc.shutdown(0,false);
  lc.setIntensity(0,5); // 0 to 15
  lc.clearDisplay(0);
  
  Serial.begin(9600);
}

void loop() {

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
      Serial.print("Checksum error,\t"); 
      break;
    case DHTLIB_ERROR_TIMEOUT: 
      Serial.print("Time out error,\t"); 
      break;
    default: 
      Serial.print("Unknown error,\t"); 
      break;
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
