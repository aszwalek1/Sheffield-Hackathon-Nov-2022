// factory.cpp
// factory test mode, mostly by Gareth and Gee

#include "unphone.h"              // unphone specifics

static unPhone &u = unPhone::me();

static bool slideState;
static int loopCounter = 0;
static bool doFlash = true;
static sensors_event_t a, m, g, temp;
static bool doneSetup = false;
static void screenTouched(void);
static void screenError(const char* message);
static bool inFactoryTestMode = false;

bool unPhone::factoryTestMode() {           // read factory test mode
  return inFactoryTestMode;
}
void unPhone::factoryTestMode(bool mode) {  // toggle factory test mode
  inFactoryTestMode = mode;
}

void screenDraw() {
  u.tftp->fillScreen(HX8357_BLACK);
  u.tftp->drawRect(0, 0, 320, 480, HX8357_WHITE);
  u.tftp->setTextSize(3);
  u.tftp->setCursor(0,0);
  u.tftp->setTextColor(HX8357_RED);
  u.tftp->print("Red");
  u.tftp->setCursor(230,0);
  u.tftp->setTextColor(HX8357_GREEN);
  u.tftp->print("Green");
  u.tftp->setCursor(0,460);
  u.tftp->setTextColor(HX8357_BLUE);
  u.tftp->print("Blue");
  u.tftp->setCursor(212,460);
  u.tftp->setTextColor(HX8357_YELLOW);
  u.tftp->print("Yellow");
  u.rgb(1,0,1);
  delay(2000);

  u.tftp->fillScreen(HX8357_BLACK);
  u.tftp->setTextColor(HX8357_RED);
  u.tftp->setCursor(30,50);
  u.tftp->print("But3");

  u.tftp->setCursor(135,50);
  u.tftp->print("But2");

  u.tftp->setCursor(240,50);
  u.tftp->print("But1");

  u.tftp->setCursor(100,100);
  u.tftp->print("Slide ");
  if (slideState) u.tftp->print("<-"); else u.tftp->print("->");
  u.tftp->drawRect(40, 200, 250, 70, HX8357_MAGENTA);
  u.tftp->setTextSize(4);
  u.tftp->setCursor(70,220);
  u.tftp->setTextColor(HX8357_CYAN);
  u.tftp->print("Touch me");

  u.tftp->setTextColor(HX8357_MAGENTA);
  u.tftp->setTextSize(2);
  u.tftp->setCursor(30,160);
  u.tftp->print("(note power switch");
  u.tftp->setCursor(30,175);
  u.tftp->print("now switches power :)");
}

void screenTouched(void) {
  u.tftp->fillRect(40, 200, 250, 70, HX8357_BLACK);
  u.tftp->drawRect(15, 200, 290, 70, HX8357_CYAN);
  u.tftp->setTextSize(4);
  u.tftp->setCursor(25,220);
  u.tftp->setTextColor(HX8357_GREEN);
  u.tftp->print("I'm touched");
  doFlash=false;
  u.vibe(false);
  u.ir(false);
  u.rgb(0,1,0);
}

void screenError(const char* message) {
  u.rgb(1,0,0);
  u.tftp->fillScreen(HX8357_BLACK);
  u.tftp->setCursor(0,10);
  u.tftp->setTextSize(2);
  u.tftp->setTextColor(HX8357_WHITE);
  u.tftp->print(message);
  delay(5000);
  u.backlight(false);
  u.rgb(0,0,0);
  while(true);
}

void unPhone::factoryTestSetup() {
  u.checkPowerSwitch();
  slideState = IOExpander::digitalRead(POWER_SWITCH);
  Serial.print("Spin 8 test rig, spin ");
  Serial.println(String(UNPHONE_SPIN));
  Serial.print("getVersionNumber() says spin ");
  Serial.println(u.getVersionNumber());
  u.backlight(true);

  screenDraw();

  Serial.println("screen displayed");
  u.tftp->setTextColor(HX8357_GREEN);
  u.rgb(1,1,1);
  if(!u.tsp->begin()) {
    Serial.println("failed to start touchscreen controller");
    screenError("failed to start touchscreen controller");
  } else {
    u.tsp->setRotation(1); // should actually be 3 I think
    Serial.println("Touchscreen started OK");
    u.tftp->setCursor(30,380);
    u.tftp->setTextSize(2);
    u.tftp->print("Touchscreen started");
  }

#if UNPHONE_SPIN == 7
  if(!u.accelp->begin()) {
#elif UNPHONE_SPIN >= 9
  if(!u.accelp->begin_I2C()) {
#endif
    Serial.println("failed to start accelerometer");
    screenError("failed to start accelerometer");
  } else {
    Serial.println("Accelerometer started OK");
    u.tftp->setCursor(30,350);
    u.tftp->setTextSize(2);
    u.tftp->print("Accelerometer started");
  }

  // init the SD card
  // see Adafruit_ImageReader/examples/FeatherWingHX8357/FeatherWingHX8357.ino
  IOExpander::digitalWrite(SD_CS, LOW);
  if(!u.sdp->begin(SD_CS, SD_SCK_MHZ(25))) { // ESP32 25 MHz limit
    Serial.println("failed to start SD card");
    screenError("failed to start SD card");
  } else {
    Serial.println("SD Card started OK");
    u.tftp->setCursor(30,410);
    u.tftp->setTextSize(2);
    u.tftp->setTextColor(HX8357_GREEN);
    u.tftp->print("SD Card started");
  }
  IOExpander::digitalWrite(SD_CS, HIGH);

  if(u.getRegister(BM_I2CADD, BM_VERSION)!=192) {
    Serial.println("failed to start Battery management");
    screenError("failed to start Battery management");
  } else {
    Serial.println("Battery management started OK");
    u.tftp->setCursor(30,440);
    u.tftp->setTextSize(2);
    u.tftp->print("Batt management started");
  }
  u.tftp->setCursor(30,320);
  u.tftp->setTextSize(2);
  u.tftp->println("LoRa failed");
  Serial.printf("Calling u.loraSetup()\n");
  u.loraSetup(); // init the board
  u.tftp->fillRect(30, 320, 250, 32, HX8357_BLACK);
  u.tftp->setCursor(30,320);
  u.tftp->println("LoRa started");

  u.tftp->setTextSize(2);
  u.tftp->setCursor(120,140);
  u.tftp->print("spin: ");
  u.tftp->println(UNPHONE_SPIN);

  u.rgb(255,0,0);
  delay(300);
  u.rgb(0,255,0);
  delay(300);
  u.rgb(0,0,255);
  delay(300);
}

static bool sentLora = false;
void unPhone::factoryTestLoop() {
  if (doFlash) {
    loopCounter++;
    if (loopCounter<50) {
      u.vibe(true);
      u.expanderPower(true);            // enable expander power supply
      u.ir(true);
    } else if (loopCounter>=50) {
      u.vibe(false);
      u.expanderPower(false);           // disable expander power supply
      u.ir(false);
    } else if (loopCounter>=100000 && !sentLora) {
      u.loraSend("unphone factory test mode message");
      sentLora = true;
    }
    if (loopCounter>=99) loopCounter=0;
  }

  if (u.tsp->touched()) {
    TS_Point p = u.tsp->getPoint();
    if (p.z>40 && p.x>1000 && p.x<3500 && p.y>1300 && p.y<3900) {
      screenTouched();
    }
  }

  if (digitalRead(u.BUTTON1)==LOW) {
    u.tftp->setTextSize(3);
    u.tftp->setCursor(240,50);
    u.tftp->print("But1");
  }

  if (digitalRead(u.BUTTON2)==LOW) {
    u.tftp->setTextSize(3);
    u.tftp->setCursor(135,50);
    u.tftp->print("But2");
  }

  if (digitalRead(u.BUTTON3)==LOW) {
    u.tftp->setTextSize(3);
    u.tftp->setCursor(30,50);
    u.tftp->print("But3");
  }

  if (IOExpander::digitalRead(POWER_SWITCH)!=slideState) {
    slideState=!slideState;
    u.tftp->fillRect(100, 100, 150, 32, HX8357_BLACK);
    u.tftp->setTextSize(3);
    u.tftp->setCursor(130,100);
    u.tftp->print("Slid");
  }

  u.tftp->fillRect(50, 280, 250, 32, HX8357_BLACK);
  u.tftp->setTextSize(2);
  u.tftp->setCursor(50,280);
#if UNPHONE_SPIN == 7
  u.accelp->getEvent(&a, &m, &g, &temp);
#elif UNPHONE_SPIN >= 9
  u.accelp->getEvent(&a, &g, &temp);
#endif
  u.tftp->print("X: "); u.tftp->println(a.acceleration.x);
  u.tftp->setCursor(150,280);
  u.tftp->print("Y: "); u.tftp->println(a.acceleration.y);

  // require 3 button presses to check power switch
  if(u.button1() && u.button2() && u.button3())
    u.checkPowerSwitch();
}
