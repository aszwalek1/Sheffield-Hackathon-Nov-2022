// TestRigUIElement.cpp

#include "unphone.h"              // unphone specifics
#include "AllUIElement.h"         // this screen's model

static unPhone &u = unPhone::me();

/**
 * Process touches.
 * @returns bool - true if the touch is on the switcher
 */
bool TestRigUIElement::handleTouch(long x, long y) {
  // Serial.printf("test rig touch, x=%ld, y=%ld\n", x, y);
  if(x > 25 && x < 280 && y > 215 && y < 280) {
    m_tft->setTextSize(3);
    m_tft->setTextColor(RED);
    m_tft->setCursor(15, 300); m_tft->print("restart in 3...");

    delay(3000);
    ESP.restart();
  }
  return y < BOXSIZE && x > (BOXSIZE * SWITCHER);
}

void TestRigUIElement::draw() {
  m_tft->setTextSize(3);
  m_tft->setTextColor(YELLOW);
  m_tft->setCursor(15,  45); m_tft->print("to enter factory");
  m_tft->setCursor(15,  80); m_tft->print("test mode please");
  m_tft->setCursor(15, 115); m_tft->print("restart with all 3");
  m_tft->setCursor(15, 150); m_tft->print("buttons pressed");

  drawSwitcher();

  u.tftp->drawRect(35, 200, 250, 70, HX8357_MAGENTA);
  u.tftp->setTextSize(3);
  u.tftp->setCursor(58,220);
  u.tftp->setTextColor(HX8357_CYAN);
  u.tftp->print("restart now");
  return;
}

void TestRigUIElement::runEachTurn() { return; }
