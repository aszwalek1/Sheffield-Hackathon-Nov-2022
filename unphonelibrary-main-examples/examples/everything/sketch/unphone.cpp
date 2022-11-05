// unphone.cpp
// core library

#include "unphone.h"
#include <Preferences.h>
#include "UIController.h"         // UI control
#include <esp_task_wdt.h>

// construction; unPhone is instantiated as a singleton object
unPhone::unPhone() {
  if(up) E("creating multiple instances: unPhone::up is non-null\n")
  up = this;
}
unPhone *unPhone::up;                  // pointer to (singleton) this
unPhone& unPhone::me() { return *up; } // static access to the singleton

// MAC address //////////////////////////////////////////////////////////////
char MAC_ADDRESS[13]; // MAC addresses are 12 chars, plus the NULL terminator
char *getMAC(char *buf);                              // read MAC into buffer
const char *unPhone::getMAC() { return MAC_ADDRESS; } // return MAC buffer

// the LCD, touch screen and UI controller //////////////////////////////////
void unPhone::backlight(bool on) {     // turn the backlight on or off
  if(on) IOExpander::digitalWrite(BACKLIGHT, HIGH);
  else   IOExpander::digitalWrite(BACKLIGHT, LOW);
}
void unPhone::expanderPower(bool on) { // expander board power on or off
  if(on) IOExpander::digitalWrite(EXPANDER_POWER, HIGH);
  else   IOExpander::digitalWrite(EXPANDER_POWER, LOW);
}
void unPhone::redraw() { // redraw the UI
  ((UIController *) uiCont)->redraw();
}
void unPhone::provisioned() {   // set the UI's provisioned flag and redraw
  UIController::provisioned = true; // tell UICon done provision
  redraw();                     // perhaps we're on wifi now, redraw
}
void unPhone::uiLoop() { // service the UI from within the main loop
  ((UIController *) uiCont)->run();
}

// FreeRTOS tasks
void powerSwitchTask(void *);   // power switch check task
void powerSwitchTask(void *param) { // check power switch every 10th of sec
  while(true) { unPhone::me().checkPowerSwitch(); delay(100); }
}
void unLoopTask(void *);        // UI and TTN LoRa task
void unLoopTask(void *param) {  // service UI events & lora transactions
  // touchscrn/LCD/LoRa module all use SPI & must all be serviced in one task
  unPhone::me().loraSetup();    // init the RFM95W
  uint32_t loopIter = 0;        // iteration slicing
  while(true) {
    if(unPhone::me().factoryTestMode()) { delay(100); continue; }
    ((UIController *) unPhone::me().uiCont)->run();     // the UI
    unPhone::me().loraLoop();                           // LMIC
    if(loopIter++ % 25000 == 0) delay(100);             // IDLE task
  }
}

// initialise unPhone hardware
void unPhone::begin() {
  Serial.begin(115200);                 // init the serial line
  D("UNPHONE_SPIN: %d\n", UNPHONE_SPIN)
  ::getMAC(MAC_ADDRESS);                // store the MAC address
  beginStore(); // init small persistent store (does nothing if enabled false)

  // fire up I²C, and the unPhone's IOExpander library
  recoverI2C();
  Wire.begin();
  Wire.setClock(400000); // rates > 100k used to trigger an IOExpander bug...?
  IOExpander::begin();

  // start power switch checking
  checkPowerSwitch();
  xTaskCreate(powerSwitchTask, "power switch task", 4096, NULL, 1, NULL);

  // instantiate the display...
  tftp = new Adafruit_HX8357(LCD_CS, LCD_DC, LCD_RESET);
  IOExpander::digitalWrite(BACKLIGHT, LOW);
  tftp->begin(HX8357D);
  IOExpander::digitalWrite(BACKLIGHT, HIGH);
  tftp->setTextWrap(false);

  // ...and the touch screen
  tsp = new XPT2046_Touchscreen(TOUCH_CS); // no IRQ
  bool status = tsp->begin();
  if(!status) {
    E("failed to start touchscreen controller\n")
  } else {
    D("touchscreen controller started\n")
  }

  // init the SD card
  // see Adafruit_ImageReader/examples/FeatherWingHX8357/FeatherWingHX8357.ino
  sdp = new SdFat();
  if(!sdp->begin(SD_CS, SD_SCK_MHZ(25))) { // ESP32 25 MHz limit
    E("sdp->begin failed\n")
  } else {
    D("sdp->begin OK\n")
  }

  // init touch screen GPIOs (used for vibe motor)
  IOExpander::digitalWrite(VIBE, LOW);

  // initialise the buttons
  IOExpander::pinMode(BUTTON1, INPUT_PULLUP);
  IOExpander::pinMode(BUTTON2, INPUT_PULLUP);
  IOExpander::pinMode(BUTTON3, INPUT_PULLUP);

  D("pinmodes done\n")
  D("hardware version = %u, MAC = %s\n", getVersionNumber(), getMAC())
  D("PSRAM: size=%d, free=%d\n", ESP.getPsramSize(), ESP.getFreePsram())

  // expander power control, available from spin 7
  IOExpander::pinMode(EXPANDER_POWER, OUTPUT);
  // this will default LOW, i.e. off, but let's make it explicit anyhow
  expanderPower(false);

  // the accelerometer
#if UNPHONE_SPIN == 7
  accelp = new Adafruit_LSM9DS1(); // on i2c
  if (!accelp->begin()) // problem detecting the sensor?
    E("oops, no LSM9DS1 detected ... check your wiring?!\n")
  else
    D("accelp->begin OK\n")
#elif UNPHONE_SPIN >= 9
  accelp = new Adafruit_LSM6DS3TRC(); // on i2c
  if (!accelp->begin_I2C()) // problem detecting the sensor?
    E("oops, no LSM6DS3TRC detected ... check your wiring?!\n")
  else
    D("accelp->begin OK\n")
#endif

  // set up IR_LED pin and RGB/red
  IOExpander::pinMode(IR_LEDS, OUTPUT);
  IOExpander::pinMode(LED_RED, OUTPUT);

  // display the first screen
  uiCont = new UIController(ui_configure);
  if(((UIController *) uiCont) == NULL || !((UIController *) uiCont)->begin())
    E("WARNING: ui.begin failed!\n")

  // start servicing UI events and LoRa transactions
  xTaskCreate(unLoopTask, "unphone loop task", 8192, NULL, 1, NULL);
} // begin()

uint8_t unPhone::getVersionNumber() { return UNPHONE_SPIN; }

void unPhone::vibe(bool on) {
  if (on)
    IOExpander::digitalWrite(VIBE, HIGH);
  else
    IOExpander::digitalWrite(VIBE, LOW);
}

// IR LEDs on or off
void unPhone::ir(bool on) {
  IOExpander::digitalWrite(IR_LEDS, (on) ? HIGH : LOW);
}

void unPhone::rgb(uint8_t red, uint8_t green, uint8_t blue) {
  IOExpander::digitalWrite(LED_RED, red);
  IOExpander::digitalWrite(LED_GREEN, green);
  IOExpander::digitalWrite(LED_BLUE, blue);
}

bool unPhone::button1() { return IOExpander::digitalRead(BUTTON1) == LOW; }
bool unPhone::button2() { return IOExpander::digitalRead(BUTTON2) == LOW; }
bool unPhone::button3() { return IOExpander::digitalRead(BUTTON3) == LOW; }

// get a (spin-agnostic) accelerometer reading
void unPhone::getAccelEvent(sensors_event_t *eventp)
{
#if UNPHONE_SPIN == 7
  sensors_event_t m, g, temp;
  accelp->getEvent(eventp, &m, &g, &temp);
#elif UNPHONE_SPIN >= 9
  sensors_event_t gyro, temp;
  accelp->getEvent(eventp, &gyro, &temp);
#endif
}

// try to recover I2C bus in case it's locked up...
// NOTE: only do this in setup **BEFORE** Wire.begin!
void unPhone::recoverI2C() {
  pinMode(SCL, OUTPUT);
  pinMode(SDA, OUTPUT);
  digitalWrite(SDA, HIGH);

  for(int i = 0; i < 10; i++) { // 9th cycle acts as NACK
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(SCL, LOW);
    delayMicroseconds(5);
  }

  // a STOP signal (SDA from low to high while SCL is high)
  digitalWrite(SDA, LOW);
  delayMicroseconds(5);
  digitalWrite(SCL, HIGH);
  delayMicroseconds(2);
  digitalWrite(SDA, HIGH);
  delayMicroseconds(2);

  // I2C bus should be free now... a short delay to help things settle
  delay(200);
}

// power management chip API /////////////////////////////////////////////////

float unPhone::batteryVoltage() { return tsp->getVBat(); }

void unPhone::printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();
  char *wakeup_string = (char *) "wakeup reason unknown!";
  char buf[80];

  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      wakeup_string = (char *) "wakeup caused by external signal using RTC_IO";
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      wakeup_string = (char *) "wakeup caused by external signal using RTC_CNTL";
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      wakeup_string = (char *) "wakeup caused by timer";
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      wakeup_string = (char *) "wakeup caused by touchpad";
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      wakeup_string = (char *) "wakeup caused by ULP program";
      break;
    case ESP_SLEEP_WAKEUP_UNDEFINED:
      wakeup_string = (char *) "wakeup cause undefined (not from deep sleep)";
      break;
    default:
      sprintf(buf, "wakeup not caused by deep sleep: %d" + wakeup_reason);
      wakeup_string = buf;
      break;
  }
  store(wakeup_string);
}

// is the power switch turned on?
bool unPhone::powerSwitchIsOn() {
  // what is the state of the power switch? (non-zero = on, which is
  // physically slid away from the USB socket)
  return (bool) IOExpander::digitalRead(POWER_SWITCH);
}

// is USB power connected?
bool unPhone::usbPowerConnected() {
  // bit 2 of status register indicates if USB connected
  return (bool) bitRead(getRegister(BM_I2CADD, BM_STATUS), 2);
}

// check for power off states and do BM shipping mode (when on bat) or ESP
// deep sleep (when on USB 5V); if it returns then the device is switched on
void unPhone::checkPowerSwitch() {
  if(!powerSwitchIsOn()) {  // when power switch off
    // turn off expander power, LEDs, etc.
    turnPeripheralsOff();

    if(!usbPowerConnected()) { // and usb unplugged we go into shipping mode
      store("switch is off, power is OFF: going to shipping mode");
      setShipping(true); // tell BM to stop supplying power until USB connects
    } else { // power switch off and usb plugged in we sleep
      store("switch is off, but power is ON: going to deep sleep");
      wakeOnPowerSwitch();
      esp_sleep_enable_timer_wakeup(60000000); // ea min: USB? else->shipping
      esp_deep_sleep_start(); // deep sleep, wait for wakeup on GPIO
    }
  }
}

// helper to turn off everything we can think of prior to power down or deep sleep
void unPhone::turnPeripheralsOff() {
  expanderPower(false);
  IOExpander::digitalWrite(BACKLIGHT, LOW);
  ir(false);            // TODO invert if logic changes!
  unPhone:rgb(1, 1, 1); // TODO invert if logic changes!
}

// set a wakeup interrupt on the power switch
void unPhone::wakeOnPowerSwitch() {
#if UNPHONE_SPIN == 7
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 0); // 1 = High, 0 = Low
#elif UNPHONE_SPIN >= 9
      esp_sleep_enable_ext0_wakeup((gpio_num_t) POWER_SWITCH, 1);
      // (changed from low in 9 as is no longer on the expander)
#endif
}

// ask BM chip to shutdown or start up
void unPhone::setShipping(bool value) {
  byte result;
  if(value) {
    result=getRegister(BM_I2CADD, BM_WATCHDOG);  // state of timing register
    bitClear(result, 5);                         // clear bit 5...
    bitClear(result, 4);                         // and bit 4 to disable...
    setRegister(BM_I2CADD, BM_WATCHDOG, result); // WDT (REG05[5:4] = 00)

    result=getRegister(BM_I2CADD, BM_OPCON);     // operational register
    bitSet(result, 5);                           // set bit 5 to disable...
    setRegister(BM_I2CADD, BM_OPCON, result);    // BATFET (REG07[5] = 1)
  } else {
    result=getRegister(BM_I2CADD, BM_WATCHDOG);  // state of timing register
    bitClear(result, 5);                         // clear bit 5...
    bitSet(result, 4);                           // and set bit 4 to enable...
    setRegister(BM_I2CADD, BM_WATCHDOG, result); // WDT (REG05[5:4] = 01)

    result=getRegister(BM_I2CADD, BM_OPCON);     // operational register
    bitClear(result, 5);                         // clear bit 5 to enable...
    setRegister(BM_I2CADD, BM_OPCON, result);    // BATFET (REG07[5] = 0)
  }
}

// I2C helpers to drive the power management chip
void unPhone::setRegister(byte address, byte reg, byte value) {
  write8(address, reg, value);
}
byte unPhone::getRegister(byte address, byte reg) {
  byte result;
  result=read8(address, reg);
  return result;
}
void unPhone::write8(byte address, byte reg, byte value) {
  Wire.beginTransmission(address);
  Wire.write((uint8_t)reg);
  Wire.write((uint8_t)value);
  Wire.endTransmission();
}
byte unPhone::read8(byte address, byte reg) {
  byte value;
  Wire.beginTransmission(address);
  Wire.write((uint8_t)reg);
  Wire.endTransmission();
  Wire.requestFrom(address, (byte)1);
  value = Wire.read();
  Wire.endTransmission();
  return value;
}


// the LoRa board and TTN LoRaWAN ///////////////////////////////////////////
void unPhone::loraSetup() { lora_setup(); }     // init the LoRa board
void unPhone::loraLoop() { lora_loop(); }       // service LoRa transactions
void unPhone::loraSend(const char *fmt, ...) {  // send (sprintf style)
  va_list arglist;
  va_start(arglist, fmt);
  lora_send(fmt, arglist);
  va_end(arglist);
}

// save short sequences of strings using the Preferences API /////////////////
// we use a ring buffer with STORE_SIZE elements stored in NVS;
// each value is keyed by its index (0 to STORE_SIZE - 1)
static Preferences prefs; // an NVS store, used for a persistent ring buffer
static const char storeName[] = "unphoneStore";         // prefs namespace
static const char storeIndexName[] = "unphoneStoreIdx"; // key for ring index
static uint8_t currentStoreIndex = 0; // next position in the ring
void unPhone::beginStore() { // init Prefs, get stored index if present //////
  prefs.begin(storeName, false);
  int8_t storedIndex = prefs.getChar(storeIndexName, -1);
  if(storedIndex != -1) // a previously stored index was found (else use 0)
    currentStoreIndex = (uint8_t) storedIndex;
}
void unPhone::store(const char *s) { // store a value at current index ///////
  char key[3]; // up to 255 max as we're using uint8_t for the index
  sprintf(key, "%d", currentStoreIndex++);
  prefs.putString(key, s); // store the value against the next ring position

  if(currentStoreIndex == STORE_SIZE) // wrap at the end of the ring buffer
    currentStoreIndex = 0;
  prefs.putChar(storeIndexName, currentStoreIndex); // store next index point
}
void unPhone::printStore() { // print all stored values //////////////////////
  Serial.println("------------------------------------------");
  Serial.println("stored messages (oldest first):");
  int printed = 0;
  char key[3]; // up to 255 max as we're using uint8_t for the index
  for(int i = currentStoreIndex; printed < STORE_SIZE; printed++) {
    sprintf(key, "%d", i);
    String value = prefs.getString(key, ""); // get value if stored
    if(value != "")
      Serial.printf("store[%d] = %s\n", i, value.c_str());
    if(++i == STORE_SIZE) i = 0; // wrap at the end of the ring buffer
  }
  Serial.println("------------------------------------------");
}
void unPhone::clearStore() { // delete all values, store 0 as index //////////
  prefs.clear();
  currentStoreIndex = 0;
  prefs.putChar(storeIndexName, currentStoreIndex);
}


// get the ESP's MAC address ///////////////////////////////////////////////
char *getMAC(char *buf) { // the MAC is 6 bytes, so needs careful conversion...
  uint64_t mac = ESP.getEfuseMac(); // ...to string (high 2, low 4):
  char rev[13];
  sprintf(rev, "%04X%08X", (uint16_t) (mac >> 32), (uint32_t) mac);

  // the byte order in the ESP has to be reversed relative to normal Arduino
  for(int i=0, j=11; i<=10; i+=2, j-=2) {
    buf[i] = rev[j - 1];
    buf[i + 1] = rev[j];
  }
  buf[12] = '\0';
  return buf;
}


// the TCA9555 i/o expander
// Jon Williamson, Pimoroni, Oct/Nov 2018
// tweaks and comments by Hamish & Gareth
uint16_t IOExpander::directions = 0x00;
uint16_t IOExpander::output_states = 0x00;

void IOExpander::begin() {
  // setup the IO expander intially as all inputs so we
  // don't accidentally drive anything until it's asked for
  // this is done by writing all ones to the two config registers
  IOExpander::writeRegisterWord(0x06, 0xFFFF);

  // read the current port directions and output states
  IOExpander::directions = IOExpander::readRegisterWord(0x06);
  IOExpander::output_states = IOExpander::readRegisterWord(0x02);
}

void IOExpander::pinMode(uint8_t pin, uint8_t mode) {
  if(pin & 0x40) {
    pin &= 0b10111111;  // mask out the high bit

    uint16_t new_directions = IOExpander::directions;
    if(mode==OUTPUT){
      new_directions &= ~(1UL << pin);
    } else {
      new_directions |= (1UL << pin);
    }
    if(new_directions != IOExpander::directions) {
      IOExpander::writeRegisterWord(0x06, new_directions);
      IOExpander::directions = new_directions;
    }
  } else {
    ::pinMode(pin, mode);
  }
}

void IOExpander::digitalWrite(uint8_t pin, uint8_t value) {
  if(pin & 0x40) {
    pin &= 0b10111111;  // mask out the high bit

    // set the output state
    uint16_t new_output_states = IOExpander::output_states;
    if(value == HIGH) {
      new_output_states |=  (1UL << pin);
    } else {
      new_output_states &= ~(1UL << pin);
    }
    if(new_output_states != IOExpander::output_states) {
      IOExpander::writeRegisterWord(0x02, new_output_states);
      IOExpander::output_states = new_output_states;
    }

    // set the pin direction to output
    uint16_t new_directions = IOExpander::directions;
    new_directions &= ~(1UL << pin);
    if(new_directions != IOExpander::directions) {
      IOExpander::writeRegisterWord(0x06, new_directions);
      IOExpander::directions = new_directions;
    }
  } else {
    ::digitalWrite(pin, value);
  }
}

uint8_t IOExpander::digitalRead(uint8_t pin) {
  if(pin & 0x40) {
    pin &= 0b10111111;

    // set the pin direction to input
    uint16_t new_directions = IOExpander::directions;
    new_directions |= (1UL << pin);
    if(new_directions != IOExpander::directions) {
      IOExpander::writeRegisterWord(0x06, new_directions);
      IOExpander::directions = new_directions;
    }

    // read the input register inverted
    uint16_t inputs = IOExpander::readRegisterWord(0x00);
    return (inputs & (1UL << pin)) ? HIGH : LOW;
  } else {
    return ::digitalRead(pin);
  }
}

uint16_t IOExpander::readRegisterWord(uint8_t reg) {
  Wire.beginTransmission(IOExpander::i2c_address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(IOExpander::i2c_address, (uint8_t)(2));
  return Wire.read() | (Wire.read() << 8);
}

void IOExpander::writeRegisterWord(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(IOExpander::i2c_address);
  Wire.write(reg);
  Wire.write(value);
  Wire.write(value >> 8);
  Wire.endTransmission();
}
