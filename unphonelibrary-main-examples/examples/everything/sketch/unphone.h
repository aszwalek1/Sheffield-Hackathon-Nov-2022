// unphone.h
// core definitions and includes

#ifndef UNPHONE_H
#define UNPHONE_H
#define IO_EXPANDER_INJECTED

// include this first so we can pick up pins_arduino.h for the current board
#include <Arduino.h>

// define the default hardware spin to 9 if it is not set by the build context
// (though it _should_ be present in boards.txt and/or <device>.json)
#ifndef UNPHONE_SPIN
#  define UNPHONE_SPIN 9
#endif

#include "joinme.h"               // OTA etc.
#include "ttn-lora.h"             // LoRaWAN and TTN API

#include <stdint.h>               // integer types
#include <Wire.h>                 // I²C comms on the Arduino
#include <SPI.h>                  // the SPI bus
#include <Adafruit_Sensor.h>      // base class etc. for sensor abstraction
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_GFX.h>         // core graphics library
#include <Adafruit_HX8357.h>      // tft display

#include <XPT2046_Touchscreen.h>  // touch screen
#if UNPHONE_SPIN == 7
#include <Adafruit_LSM9DS1.h>     // the accelerometer sensor
#elif UNPHONE_SPIN >= 9
#include <Adafruit_LSM6DS3TRC.h>  // the accelerometer sensor
#endif

// the main API
class unPhone {
public:
  unPhone();                                   // construction
  static unPhone *up;                          // pointer to (singleton) this
  static unPhone& me();                        // static context access

  void begin();                                // initialise hardware
  uint8_t getVersionNumber();                  // hardware revision
  const char *getMAC();                        // the ESP MAC address

#if UNPHONE_SPIN == 7
  static const uint8_t LCD_RESET        =  1 | 0x40;
  static const uint8_t BACKLIGHT        =  2 | 0x40;
  static const uint8_t LCD_CS           =  3 | 0x40;
  static const uint8_t LCD_DC           = 21;
  static const uint8_t LORA_CS          =  4 | 0x40;
  static const uint8_t LORA_RESET       =  5 | 0x40;
  static const uint8_t TOUCH_CS         =  6 | 0x40;
  static const uint8_t LED_RED          =  8 | 0x40;
  static const uint8_t POWER_SWITCH     = 10 | 0x40;
  static const uint8_t SD_CS            = 11 | 0x40;
  static const uint8_t BUTTON1          = 33;        // left button
  static const uint8_t BUTTON2          = 35;        // middle button
  static const uint8_t BUTTON3          = 34;        // right button
  static const uint8_t IR_LEDS          = 13;        // the IR LED pins
  static const uint8_t EXPANDER_POWER   =  2;        // enable exp when high
#elif UNPHONE_SPIN >= 9
  static const uint8_t LCD_RESET        = 46;
  static const uint8_t BACKLIGHT        =  2 | 0x40;
  static const uint8_t LCD_CS           = 48;
  static const uint8_t LCD_DC           = 47;
  static const uint8_t LORA_CS          = 44;
  static const uint8_t LORA_RESET       = 42;
  static const uint8_t TOUCH_CS         = 38;
  static const uint8_t LED_RED          = 13;
  static const uint8_t POWER_SWITCH     = 18;
  static const uint8_t SD_CS            = 43;
  static const uint8_t BUTTON1          = 45;        // left button
  static const uint8_t BUTTON2          =  0;        // middle button
  static const uint8_t BUTTON3          = 21;        // right button
  static const uint8_t IR_LEDS          = 12;        // the IR LED pins
  static const uint8_t EXPANDER_POWER   =  0 | 0x40; // enable exp brd if high
#endif
  static const uint8_t VIBE             =  7 | 0x40;
  static const uint8_t LED_GREEN        =  9 | 0x40;
  static const uint8_t LED_BLUE         = 13 | 0x40;
  static const uint8_t USB_VSENSE       = 14 | 0x40;

  bool button1();                              // register...
  bool button2();                              // ...button...
  bool button3();                              // ...presses

  void vibe(bool);             // vibe motor on or off
  void ir(bool);               // IR LEDs on or off

  bool powerSwitchIsOn();      // is the power switch turned on?
  bool usbPowerConnected();    // is USB power connected?
  void checkPowerSwitch();     // if power switch is off shut down
  void turnPeripheralsOff();   // shut down periphs (used by checkPwrSw)
  void wakeOnPowerSwitch();    // wakeup interrupt on power switch
  void printWakeupReason();    // what woke us up?

  void *uiCont;                // the UI controller
  void redraw();               // redraw the UI
  void provisioned();          // call when provisioning is complete
  void uiLoop();               // allow the UI to run

  void recoverI2C();           // deal with i2c hangs
  bool factoryTestMode();      // read factory test mode
  void factoryTestMode(bool);  // toggle factory test mode
  void factoryTestSetup();     // factory test mode setup
  void factoryTestLoop();      // factory test mode loop

  // the touch screen, display and accelerometer ////////////////////////////
  Adafruit_HX8357 *tftp;
  XPT2046_Touchscreen *tsp;
#if UNPHONE_SPIN == 7
  Adafruit_LSM9DS1 *accelp;
#elif UNPHONE_SPIN >= 9
  Adafruit_LSM6DS3TRC *accelp;
#endif
  void getAccelEvent(sensors_event_t *); // spin-agnostic accelerometer
  void backlight(bool);        // turn the backlight on or off
  void expanderPower(bool);    // turn expander board power on or off

  // SD card filesystem
  SdFat *sdp;

  // calibration data for converting raw touch data to screen coordinates
  static const uint16_t TS_MINX =  300;
  static const uint16_t TS_MAXX = 3800;
  static const uint16_t TS_MINY =  500;
  static const uint16_t TS_MAXY = 3750;

  // the RGB LED
  void rgb(uint8_t red, uint8_t green, uint8_t blue);

  // LoRa radio
  void loraSetup();              // init the LoRa board
  void loraLoop();               // service lora transactions
  void loraSend(const char *, ...); // send (TTN) LoRaWAN message
  static const uint8_t LORA_PAYLOAD_LEN = 101; // max payload bytes (+ '\0')
#if UNPHONE_SPIN == 7
  static const uint8_t LMIC_DIO0 = 39;
  static const uint8_t LMIC_DIO1 = 26;
#elif UNPHONE_SPIN >= 9
  static const uint8_t LMIC_DIO0 = 10;
  static const uint8_t LMIC_DIO1 = 11;
#endif

  // power management chip API
  static const byte BM_I2CADD   = 0x6b; // the chip lives here on I²C
  static const byte BM_WATCHDOG = 0x05; // charge termination/timer cntrl reg
  static const byte BM_OPCON    = 0x07; // misc operation control register
  static const byte BM_STATUS   = 0x08; // system status register
  static const byte BM_VERSION  = 0x0a; // vender / part / revision status reg
  float batteryVoltage(); // get the battery voltage
  void setShipping(bool value); // tells BM chip to shut down
  void setRegister(byte address, byte reg, byte value); //
  byte getRegister(byte address, byte reg);             // I²C...
  void write8(byte address, byte reg, byte value);      // ...helpers
  byte read8(byte address, byte reg);                   //

  // a small, rotating, persistent store  (using Preferences API)
  void beginStore();     // set up small persistent store area
  void store(const char *); // save a value
  void printStore();     // play back the list of saved strings
  void clearStore();     // clear the store (note doesn't empty nvs!)
  static const uint8_t STORE_SIZE = 10; // max strings stored; must be <=255/2
}; // class unPhone

/*
The unPhone has a TCA9555 IO expansion chip that is controlled over I2C and to
which the SPI chip select (CS), reset and etc. lines of many of the modules
are connected. To use these modules the IO expander has to be told to trigger
those lines. This means that the available libraries for the modules also need
to be adapted to talk to the IOExpander, e.g. when doing digitalWrite or
pinMode. We do this by injecting code to call our own versions of these
functions (defined below) and setting the second highest bit of the pin number
high to signal those pins that are controlled via the TCA9555. Confused? Not
nearly as much as I have been...

Usage notes:

call `IOExpander::begin()` in `setup()` method after `Wire.begin()`
then to interface with the IO Expander pins you can do:
`IOExpander::digitalWrite(IOExpander::SD_CS, LOW)`

to read the current board revision use:
`uint8_t version = IOExpander::getVersionNumber()`

to get the power switch position use:
`uint8_t switch_state = IOExpander::digitalRead(IOExpander::POWER_SWITCH);`
*/
class IOExpander {
  public:
    static const uint8_t i2c_address = 0x26;

    // we cache the current state of the ports after
    // an initial read of the values during initialisation
    static uint16_t directions;
    static uint16_t output_states;

    static void begin();
    static void pinMode(uint8_t pin, uint8_t mode);       // if you change...
    static void digitalWrite(uint8_t pin, uint8_t value); // ...these, also...
    static uint8_t digitalRead(uint8_t pin); // ...change bin/lib-injector.cpp

  private:
    static uint16_t readRegisterWord(uint8_t reg);
    static void writeRegisterWord(uint8_t reg, uint16_t value);
};

// macros for debug (and error) calls to printf
#ifdef UNPHONE_PRODUCTION_BUILD
# define D(args...) (void)0;
#else
# define D(args...) printf(args);
#endif
#define  E(args...) printf("ERROR: " args);
static const char *TAG = "MAIN";        // ESP logger debug tag

// delay/yield/timing macros
#define WAIT_A_SEC   vTaskDelay(    1000/portTICK_PERIOD_MS); // 1 second
#define WAIT_SECS(n) vTaskDelay((n*1000)/portTICK_PERIOD_MS); // n seconds
#define WAIT_MS(n)   vTaskDelay(       n/portTICK_PERIOD_MS); // n millis

#endif
