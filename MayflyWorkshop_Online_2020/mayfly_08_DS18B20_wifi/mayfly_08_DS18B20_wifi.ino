/*****************************************************************************
DRWI_LTE.ino
Written By:  Sara Damiano (sdamiano@stroudcenter.org)
Development Environment: PlatformIO
Hardware Platform: EnviroDIY Mayfly Arduino Datalogger
Software License: BSD-3.
  Copyright (c) 2017, Stroud Water Research Center (SWRC)
  and the EnviroDIY Development Team

This sketch is an example of logging data to an SD card and sending the data to
both the EnviroDIY data portal as should be used by groups involved with
The William Penn Foundation's Delaware River Watershed Initiative

DISCLAIMER:
THIS CODE IS PROVIDED "AS IS" - NO WARRANTY IS GIVEN.
*****************************************************************************/

// ==========================================================================
//    Defines for the Arduino IDE
//    In PlatformIO, set these build flags in your platformio.ini
// ==========================================================================
#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 512
#endif
#ifndef TINY_GSM_YIELD_MS
#define TINY_GSM_YIELD_MS 2
#endif

// ==========================================================================
//    Include the base required libraries
// ==========================================================================
#include <Arduino.h>          // The base Arduino library
#include <EnableInterrupt.h>  // for external and pin change interrupts
#include <LoggerBase.h>       // The modular sensors library

// ==========================================================================
//    Data Logger Settings
// ==========================================================================
// The name of this file
const char* sketchName = "mayfly_08_DS18B20_wifi.ino";
// Logger ID, also becomes the prefix for the name of the data file on SD card
const char* LoggerID = "xxxxxx";  // give your logger a unique ID
// How frequently (in minutes) to log data
const uint8_t loggingInterval = 1;
// Your logger's timezone.
const int8_t timeZone = -5;  // Eastern Standard Time
// NOTE:  Daylight savings time will not be applied!  Please use standard time!

// ==========================================================================
//    Primary Arduino-Based Board and Processor
// ==========================================================================
#include <sensors/ProcessorStats.h>

const long serialBaud = 57600;  // Baud rate for the primary serial port for debugging
const int8_t greenLED  = 8;  // MCU pin for the green LED (-1 if not applicable)
const int8_t redLED    = 9;  // MCU pin for the red LED (-1 if not applicable)
const int8_t buttonPin = 21;  // MCU pin for a button to use to enter debugging
                              // mode  (-1 if not applicable)
const int8_t wakePin = A7;    // MCU interrupt/alarm pin to wake from sleep
// Set the wake pin to -1 if you do not want the main processor to sleep.
// In a SAMD system where you are using the built-in rtc, set wakePin to 1
const int8_t sdCardPwrPin = -1;  // MCU SD card power pin (-1 if not applicable)
const int8_t sdCardSSPin = 12;  // MCU SD card chip select/slave select pin (must be given!)
const int8_t sensorPowerPin = 22;  // MCU pin controlling main sensor power (-1 if not applicable)

// Create the main processor chip "sensor" - for general metadata
const char*    mcuBoardVersion = "v0.5b";
ProcessorStats mcuBoard(mcuBoardVersion);

// ==========================================================================
//  Wifi/Cellular Modem Options
// ==========================================================================
/** Start [esp8266] */
// For almost anything based on the Espressif ESP8266 using the AT command
// firmware
#include <modems/EspressifESP8266.h>
// Create a reference to the serial port for the modem
HardwareSerial& modemSerial = Serial1;  // Use hardware serial if possible
const long      modemBaud   = 115200;   // Communication speed of the modem
// NOTE:  This baud rate too fast for an 8MHz board, like the Mayfly!  The
// module should be programmed to a slower baud rate or set to auto-baud using
// the AT+UART_CUR or AT+UART_DEF command.

// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
const int8_t modemVccPin     = -2;  // MCU pin controlling modem power
const int8_t modemStatusPin  = -1;  // MCU pin used to read modem status
const int8_t modemResetPin   = -1;  // MCU pin connected to modem reset pin
const int8_t modemSleepRqPin = -1;  // MCU pin for modem sleep/wake request
const int8_t modemLEDPin = redLED;  // MCU pin connected an LED to show modem status

// Pins for light sleep on the ESP8266. For power savings, I recommend NOT using
// these if it's possible to use deep sleep.
const int8_t espSleepRqPin = -1;  // GPIO# ON THE ESP8266 to assign for light
                                  // sleep request
const int8_t espStatusPin = -1;   // GPIO# ON THE ESP8266 to assign for light
                                  // sleep status

//****
// Network connection information
const char* wifiId  = "xxxxxxx";     // The WiFi access point
const char* wifiPwd = "xxxxxxx";     // The password for connecting to WiFi

// Create the loggerModem object
EspressifESP8266 modemESP(&modemSerial, modemVccPin, modemStatusPin,
                          modemResetPin, modemSleepRqPin, wifiId, wifiPwd,
                          espSleepRqPin, espStatusPin);
// Create an extra reference to the modem by a generic name
EspressifESP8266 modem = modemESP;
/** End [esp8266] */

// ==========================================================================
//    Maxim DS3231 RTC (Real Time Clock)
// ==========================================================================
#include <sensors/MaximDS3231.h>

// Create a DS3231 sensor object
MaximDS3231 ds3231(1);

// ==========================================================================
//  Maxim DS18 One Wire Temperature Sensor
// ==========================================================================
/** Start [ds18] */
#include <sensors/MaximDS18.h>

// OneWire Address [array of 8 hex characters]
// If only using a single sensor on the OneWire bus, you may omit the address
// DeviceAddress OneWireAddress1 = {0x28, 0xFF, 0xBD, 0xBA, 0x81, 0x16, 0x03,
// 0x0C};
const int8_t OneWirePower = sensorPowerPin;  // Power pin (-1 if unconnected)
const int8_t OneWireBus   = 4;  // OneWire Bus Pin (-1 if unconnected)

// Create a Maxim DS18 sensor objects (use this form for a known address)
// MaximDS18 ds18(OneWireAddress1, OneWirePower, OneWireBus);

// Create a Maxim DS18 sensor object (use this form for a single sensor on bus
// with an unknown address)
MaximDS18 ds18(OneWirePower, OneWireBus);
/** End [ds18] */

//// ==========================================================================
//    Creating the Variable Array[s] and Filling with Variable Objects
// ==========================================================================

Variable* variableList[] = {
    new ProcessorStats_Battery(&mcuBoard),
    new MaximDS3231_Temp(&ds3231),
    new MaximDS18_Temp(&ds18),
};
// *** CAUTION --- CAUTION --- CAUTION --- CAUTION --- CAUTION ***
// Check the order of your variables in the variable list!!!
// Be VERY certain that they match the order of your UUID's!
// Rearrange the variables in the variable list if necessary to match!
// *** CAUTION --- CAUTION --- CAUTION --- CAUTION --- CAUTION ***
//-----------------------------------------------------------------------------------

const char* UUIDs[] =  // UUID array for device sensors
    {
        "12345678-abcd-1234-ef00-1234567890ab",  // Battery voltage
                                                 // (EnviroDIY_Mayfly_Batt)
        "12345678-abcd-1234-ef00-1234567890ab",  // Temperature
                                                 // (EnviroDIY_Mayfly_Temp)
        "12345678-abcd-1234-ef00-1234567890ab"   // Temperature
                                                 // (Maxim_DS18B20_Temp)
};
const char* registrationToken =
    "12345678-abcd-1234-ef00-1234567890ab";  // Device registration token
const char* samplingFeature =
    "12345678-abcd-1234-ef00-1234567890ab";  // Sampling feature UUID
    
//------------------------------------------------------------------------

// Count up the number of pointers in the array
int variableCount = sizeof(variableList) / sizeof(variableList[0]);

// Create the VariableArray object
VariableArray varArray(variableCount, variableList, UUIDs);

// ==========================================================================
//     The Logger Object[s]
// ==========================================================================

// Create a new logger instance
Logger dataLogger(LoggerID, loggingInterval, &varArray);

// ==========================================================================
//    A Publisher to WikiWatershed
// ==========================================================================
// Device registration and sampling feature information can be obtained after
// registration at http://data.WikiWatershed.org

// Create a data publisher for the EnviroDIY/WikiWatershed POST endpoint
#include <publishers/EnviroDIYPublisher.h>
EnviroDIYPublisher EnviroDIYPOST(dataLogger, &modem.gsmClient,
                                 registrationToken, samplingFeature);

// ==========================================================================
//    Working Functions
// ==========================================================================

// Flashes the LED's on the primary board
void greenredflash(uint8_t numFlash = 4, uint8_t rate = 75) {
    for (uint8_t i = 0; i < numFlash; i++) {
        digitalWrite(greenLED, HIGH);
        digitalWrite(redLED, LOW);
        delay(rate);
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, HIGH);
        delay(rate);
    }
    digitalWrite(redLED, LOW);
}

// Read's the battery voltage
// NOTE: This will actually return the battery level from the previous update!
float getBatteryVoltage() {
    if (mcuBoard.sensorValues[0] == -9999) mcuBoard.update();
    return mcuBoard.sensorValues[0];
}


// ==========================================================================
// Main setup function
// ==========================================================================
void setup() {
    // Start the primary serial connection
    Serial.begin(serialBaud);

    // Print a start-up note to the first serial port
    Serial.print(F("Now running "));
    Serial.print(sketchName);
    Serial.print(F(" on Logger "));
    Serial.println(LoggerID);
    Serial.println();

    Serial.print(F("Using ModularSensors Library version "));
    Serial.println(MODULAR_SENSORS_VERSION);

    // Start the serial connection with the modem
    modemSerial.begin(modemBaud);

    // Set up pins for the LED's
    pinMode(greenLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    pinMode(redLED, OUTPUT);
    digitalWrite(redLED, LOW);
    // Blink the LEDs to show the board is on and starting up
    greenredflash();

    // Set the timezones for the logger/data and the RTC
    // Logging in the given time zone
    Logger::setLoggerTimeZone(timeZone);
    // It is STRONGLY RECOMMENDED that you set the RTC to be in UTC (UTC+0)
    Logger::setRTCTimeZone(0);

    // Attach the modem and information pins to the logger
    dataLogger.attachModem(modem);
    modem.setModemLED(modemLEDPin);
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, buttonPin,
                             greenLED);

    // Begin the logger
    dataLogger.begin();

    if (getBatteryVoltage() > 3.65) {
        modem.modemPowerUp();
        // modem.wake();

#if F_CPU == 8000000L
        if (modemBaud > 57600) {
            modemSerial.begin(115200);
            modem.gsmModem.sendAT(GF("+UART_DEF=9600,8,1,0,0"));
            modem.gsmModem.waitResponse();
            modemSerial.end();
            modemSerial.begin(9600);
        }
#endif

        modem.setup();
    }

    // Note:  Please change these battery voltages to match your battery
    // Set up the sensors, except at lowest battery level
    if (getBatteryVoltage() > 3.4) {
        Serial.println(F("Setting up sensors..."));
        varArray.setupSensors();
    }


    // Create the log file, adding the default header to it
    // Do this last so we have the best chance of getting the time correct and
    // all sensor names correct
    // Writing to the SD card can be power intensive, so if we're skipping
    // the sensor setup we'll skip this too.
    if (getBatteryVoltage() > 3.4) {
        Serial.println(F("Setting up file on SD card"));
        dataLogger.turnOnSDcard(
            true);  // true = wait for card to settle after power up
        dataLogger.createLogFile(true);  // true = write a new header
        dataLogger.turnOffSDcard(
            true);  // true = wait for internal housekeeping after write
    }

    // Power down the modem - but only if there will be more than 15 seconds
    // before the first logging interval - it can take the LTE modem that long
    // to shut down
    if (Logger::getNowEpoch() % (loggingInterval * 60) > 15 ||
        Logger::getNowEpoch() % (loggingInterval * 60) < 6) {
        Serial.println(F("Putting modem to sleep"));
        modem.disconnectInternet();
        modem.modemSleepPowerDown();
    } else {
        Serial.println(F("Leaving modem on until after first measurement"));
    }

    // Call the processor sleep
    Serial.println(F("Putting processor to sleep"));
    Serial.println(F("------------------------------------------\n"));
    dataLogger.systemSleep();
}

// ==========================================================================
// Main loop function
// ==========================================================================

// Use this short loop for simple data logging and sending
void loop() {
    // Note:  Please change these battery voltages to match your battery
    // At very low battery, just go back to sleep
    if (getBatteryVoltage() < 3.4) {
        dataLogger.systemSleep();
    }
    // At moderate voltage, log data but don't send it over the modem
    else if (getBatteryVoltage() < 3.45) {
        dataLogger.logData();
    }
    // If the battery is good, send the data to the world
    else {
        dataLogger.logDataAndPublish();
    }
}
