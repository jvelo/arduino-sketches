#include "LedControl.h"
#include <dht.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <CmdMessenger.h>
#include <Base64.h>
#include <Streaming.h>
#include <ArduinoJson.h>

dht DHT;
SFE_BMP180 bmp180;

#define DHT22_PIN 3
#define ALTITUDE 252.0 // Croix rousse

// Arduino Pin 7 to DIN, 6 to Clk, 5 to LOAD, no.of devices is 1
LedControl lc = LedControl(7, 6, 5, 1);

#define DEBUG false

// Mustnt conflict / collide with our message payload data. Fine if we use base64 library ^^ above
char field_separator = ',';
char command_separator = ';';

// Attach a new CmdMessenger object to the default Serial port
CmdMessenger cmdMessenger = CmdMessenger(Serial, field_separator, command_separator);

// -------- Commands ---------

enum {
  kCOMM_ERROR    = 000, // Lets Arduino report serial port comm error back to the PC (only works for some comm errors)
  kACK           = 001, // Arduino acknowledges cmd was received
  kARDUINO_READY = 002, // After opening the comm port, send this cmd 02 from PC to check arduino is ready
  kCOMMAND_ERROR = 003, // Arduino reports badly formatted cmd, or cmd not recognised

  kINFO_MESSAGE  = 004,
  kERROR_MESSAGE = 005,

  kSENDORS_DATA  = 006,

  kSEND_CMDS_END = 010,
};

// Commands we send from the PC and want to recieve on the Arduino.
// They start at the address kSEND_CMDS_END defined ^^ above as 004
messengerCallbackFunction messengerCallbacks[] =
{
  NULL
};

// ---------------------------

void arduinoReady()
{
  // In response to ping. We just send a throw-away Acknowledgement to say "im alive"
  cmdMessenger.sendCmd(kACK, "Arduino ready");
}

void unknownCmd()
{
  // Default response for unknown commands and corrupt messages
  cmdMessenger.sendCmd(kCOMMAND_ERROR, "Unknown command");
}

// ---------------------------

void attach_callbacks(messengerCallbackFunction* callbacks)
{
  int i = 0;
  int offset = kSEND_CMDS_END;
  while (callbacks[i])
  {
    cmdMessenger.attach(offset + i, callbacks[i]);
    i++;
  }
}

void setup() {
  // Initialize  device
  lc.shutdown(0, false);
  lc.setIntensity(0, 5); // 0 to 15
  lc.clearDisplay(0);

  Serial.begin(57600);

  cmdMessenger.print_LF_CR();   // Make output more readable whilst debugging in Arduino Serial Monitor

  // Attach default / generic callback methods
  cmdMessenger.attach(kARDUINO_READY, arduinoReady);
  cmdMessenger.attach(unknownCmd);

  // Attach my application's user-defined callback methods
  attach_callbacks(messengerCallbacks);

  arduinoReady();

  if (!bmp180.begin()) {
    cmdMessenger.sendCmd(kERROR_MESSAGE, "BMP180 init fail");
  }

  // blink
  pinMode(13, OUTPUT);
}


// Timeout handling
long timeoutInterval = 2000; // 2 seconds
long previousMillis = 0;
int counter = 0;

void timeout()
{
  // blink
  if (counter % 2) {
    digitalWrite(13, HIGH);
  }
  else {
    digitalWrite(13, LOW);
  }
  counter ++;
}

void sendDebugValue(String name, String unit, double value) {
  if (DEBUG) {
    String message = name + String(": ") + String(value) + String(" ") + unit;
    char charBuf[message.length()+1];
    message.toCharArray(charBuf, message.length()+1) ;
    cmdMessenger.sendCmd(kINFO_MESSAGE, charBuf);
  }
}

void loop() {

  // Process incoming serial data, if any
  cmdMessenger.feedinSerialData();

  // handle timeout function, if any
  if (millis() - previousMillis > timeoutInterval)
  {
    timeout();
    previousMillis = millis();
  }

  char bmpStatus;
  double T, P, p0, a;

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  int chk = DHT.read22(DHT22_PIN);
  switch (chk)
  {
    case DHTLIB_OK:
      printChar('t', 7);
      printValue(DHT.temperature, 6);      
      printChar('h', 3);
      printValue(DHT.humidity, 2);
      
      root["h"] = DHT.humidity;
      
      sendDebugValue("Temp DHT", "C", DHT.temperature);
      sendDebugValue("Humidity BMP", "%", DHT.humidity);
      
      break;
    case DHTLIB_ERROR_CHECKSUM:
      cmdMessenger.sendCmd(kERROR_MESSAGE, "DHT Checksum error");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      cmdMessenger.sendCmd(kERROR_MESSAGE, "DHT Time out error");
      break;
    default:
      cmdMessenger.sendCmd(kERROR_MESSAGE, "DHT Unknown error");
      break;
  }

  bmpStatus = bmp180.startTemperature();
  if (bmpStatus != 0) {
    delay(bmpStatus);

    bmpStatus = bmp180.getTemperature(T);
    if (bmpStatus != 0) {
      // Print out the measurement:
      sendDebugValue("Temp BMP", "C", T);
      root["t"] = T;
      bmpStatus = bmp180.startPressure(3);
      if (bmpStatus != 0) {
        delay(bmpStatus);
        bmpStatus = bmp180.getPressure(P, T);
        if (bmpStatus != 0) {
          root["P"] = P;
          sendDebugValue("Pressure", "mb", P);
        }
        else {
          cmdMessenger.sendCmd(kERROR_MESSAGE, "BMP Error retrieving pressure measurement");
        }
      }
      else {
        cmdMessenger.sendCmd(kERROR_MESSAGE, "BMP Error starting pressure measurement");
      }
    }
    else {
      cmdMessenger.sendCmd(kERROR_MESSAGE, "BMP Error starting temperature measurement");
    }
  }
  else {
    cmdMessenger.sendCmd(kERROR_MESSAGE, "BMP Error retrieving temperature measurement");
  }

  char buffer[256];
  root.printTo(buffer, sizeof(buffer));
  cmdMessenger.sendCmd(kSENDORS_DATA, buffer);

  delay(1000);
}

void printValue(double value, int l) {
  int v = (int) (value * 10);
  int i = l - 2;
  while (i <= l) {
    lc.setDigit(0, i, v % 10, i == l - 1);
    v = v / 10;
    i++;
  }
}

void printChar(char c, int i) {
  switch (c) {
    case 't':
      printT(i);
      break;
    default:
      lc.setChar(0, i, c, false);
      break;
  }
}

void printNumber(int v) {
  if (v > 99999999 || v < 0) {
    return;
  }
  int i = 0;
  while (i < 8) {
    lc.setDigit(0, i, (byte)v % 10, false);
    v = v / 10;
    i++;
  }
}

void printT(int i) {
  lc.setLed(0, i, 7, true); // T bar
  lc.setLed(0, i, 5, true); // lower left
  lc.setLed(0, i, 4, true); // bottom
  lc.setLed(0, i, 6, true); // top left
}
