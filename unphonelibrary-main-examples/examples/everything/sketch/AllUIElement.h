//  AllUIElement.h

#ifndef ALLUIELEMENT_H_
#define ALLUIELEMENT_H_

#include "UIController.h"

class MenuUIElement: public UIElement { /////////////////////////////////////
  private:
    void drawTextBoxes();
    int8_t mapTextTouch(long, long);
    int8_t menuItemSelected = -1;
  public:
    MenuUIElement (Adafruit_HX8357* tft, XPT2046_Touchscreen* ts, SdFat* sd)
    : UIElement(tft, ts, sd) {
      // nothing to initialise
    };
    bool handleTouch(long x, long y);
    void draw();
    void runEachTurn();
    int8_t getMenuItemSelected() { return menuItemSelected; }
};

class ConfigUIElement: public UIElement { ///////////////////////////////////
  private:
    long m_timer;
  public:
    ConfigUIElement (Adafruit_HX8357* tft, XPT2046_Touchscreen* ts, SdFat* sd)
     : UIElement(tft, ts, sd) { m_timer = millis(); };
    bool handleTouch(long x, long y);
    void draw();
    void runEachTurn();
};

class TouchpaintUIElement: public UIElement { ///////////////////////////////
  private:
    void drawSelector();
    void colourSelector(long, long);
    uint16_t oldcolour;
    uint16_t currentcolour;
  public:
    TouchpaintUIElement(Adafruit_HX8357* tft, XPT2046_Touchscreen* ts, SdFat* sd)
      : UIElement(tft, ts, sd) { };
    bool handleTouch(long, long);
    void draw();
    void runEachTurn();
};

class TestCardUIElement: public UIElement { /////////////////////////////////
  private:
    void drawBBC();
    void drawTestcard();
  public:
    TestCardUIElement(Adafruit_HX8357* tft, XPT2046_Touchscreen* ts, SdFat* sd)
      : UIElement(tft, ts, sd) { };
    bool handleTouch(long, long);
    void draw();
    void runEachTurn();
};

class TextPageUIElement: public UIElement { /////////////////////////////////
  private:
    void drawTextBoxes();
    int8_t mapTextTouch(long, long);
    void printHistory(uint16_t x, uint16_t y);
  public:
    TextPageUIElement(Adafruit_HX8357* tft, XPT2046_Touchscreen* ts, SdFat* sd)
      : UIElement(tft, ts, sd) { };
    bool handleTouch(long, long);
    void draw();
    void runEachTurn();
};

class EtchASketchUIElement: public UIElement { //////////////////////////////
  private:
  public:
    EtchASketchUIElement(Adafruit_HX8357* tft, XPT2046_Touchscreen* ts, SdFat* sd)
      : UIElement(tft, ts, sd) { };
    bool handleTouch(long, long);
    void draw();
    void runEachTurn();
};

class TestRigUIElement: public UIElement { ///////////////////////////////////
  private:
    long m_timer;
  public:
    TestRigUIElement (Adafruit_HX8357* tft, XPT2046_Touchscreen* ts, SdFat* sd)
     : UIElement(tft, ts, sd) { m_timer = millis(); };
    bool handleTouch(long x, long y);
    void draw();
    void runEachTurn();
};

#endif
