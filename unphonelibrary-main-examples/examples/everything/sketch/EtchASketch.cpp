// EtchASketchUIElement.cpp

#include "AllUIElement.h"

static unPhone &u = unPhone::me();
#define XMID 160
#define YMID 240
int penx = XMID, peny = YMID; // x and y coords of the etching pen

/**
 * Show initial screen.
 */
void EtchASketchUIElement::draw(){
  penx = XMID; // put the pen in the ...
  peny = YMID; // ... middle of the screen

  m_tft->fillScreen(BLACK);
  m_tft->drawLine(0, 0, 319, 0, BLUE);
  m_tft->drawLine(319, 0, 319, 479, BLUE);
  m_tft->drawLine(319, 479, 0, 479, BLUE);
  m_tft->drawLine(0, 479, 0, 0, BLUE);
  drawSwitcher();
}

/**
 * Check the accelerometer, adjust the pen coords and draw a point.
 */
void EtchASketchUIElement::runEachTurn(){
  // get a new sensor event
  sensors_event_t event;
  u.getAccelEvent(&event);

#if UNPHONE_SPIN == 7
  if(event.acceleration.x >  2 && penx < 318) penx = penx + 1;
  if(event.acceleration.x < -2 && penx > 1)   penx = penx - 1;
  if(event.acceleration.y >  2 && peny < 478) peny = peny + 1;
  if(event.acceleration.y < -2 && peny > 1)   peny = peny - 1;
#elif UNPHONE_SPIN >= 9
  if(event.acceleration.x <  2 && penx < 318) penx = penx + 1;
  if(event.acceleration.x > -2 && penx > 1)   penx = penx - 1;
  if(event.acceleration.y >  2 && peny < 478) peny = peny + 1;
  if(event.acceleration.y < -2 && peny > 1)   peny = peny - 1;
#endif

  // draw
  m_tft->drawPixel(penx, peny, HX8357_GREEN);

  // display the results (acceleration is measured in m/s^2)
  /*
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");
  Serial.println("m/s^2 ");
  */

  // delay before the next sample
  delay(20);
}

/**
 * Process touches.
 * @returns bool - true if the touch is on the switcher
 */
bool EtchASketchUIElement::handleTouch(long x, long y) {
  return y < BOXSIZE && x > (BOXSIZE * SWITCHER);
}
