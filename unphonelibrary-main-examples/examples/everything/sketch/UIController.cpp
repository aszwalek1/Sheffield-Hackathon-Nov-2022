// UIController.cpp

#include "AllUIElement.h"

static unPhone &u = unPhone::me();

// initialisation flag, not complete until parent has finished config
bool UIController::provisioned = false;

// the UI elements types (screens) /////////////////////////////////////////
const char *ui_mode_names[] = {
  "Menu",
  "Testcard: basic graphics",
  "Touchpaint",
  "Predictive text",
  "Etch-a-sketch",
  "Factory test rig",
  "Home",
};
uint8_t NUM_UI_ELEMENTS = 7;  // number of UI elements

// keep Arduino IDE compiler happy /////////////////////////////////////////
UIElement::UIElement(Adafruit_HX8357* tftp, XPT2046_Touchscreen* tsp, SdFat *sdp) {
  m_tft = tftp;
  m_ts = tsp;
  m_sd = sdp;
}
void UIElement::someFuncDummy() { }

// constructor for the main class ///////////////////////////////////////////
UIController::UIController(ui_modes_t start_mode) {
  m_mode = start_mode;
}

bool UIController::begin() { ////////////////////////////////////////////////
  //Serial.println("UIController.begin 1");
  begin(true);
  return true;
}
bool UIController::begin(boolean doDraw) { //////////////////////////////////
  //Serial.println("UIController.begin 2");
  D("UI.begin()\n")

  u.tftp->fillScreen(HX8357_GREEN);
  WAIT_MS(50)
  u.tftp->fillScreen(HX8357_BLACK);
  
  // define the menu element and the first m_element here 
  //Serial.println("UIController.begin 3");
  m_menu = new MenuUIElement(u.tftp, u.tsp, u.sdp);
  if(m_menu == NULL) {
    Serial.println("ERROR: no m_menu allocated");
    return false;
  }
  //Serial.println("UIController.begin 4");
  allocateUIElement(m_mode);

  //Serial.println("UIController.begin 5");
  if(doDraw)
    redraw();
  //Serial.println("UIController.begin 6");
  return true;
}

UIElement* UIController::allocateUIElement(ui_modes_t newMode) {
  // TODO trying to save memory here, but at the expense of possible
  // fragmentation; perhaps maintain an array of elements and never delete?
  if(m_element != 0 && m_element != m_menu) delete(m_element);

  switch(newMode) {
    case ui_menu:
      m_element = m_menu;                                               break;
    case ui_configure:
      m_element = new ConfigUIElement(u.tftp, u.tsp, u.sdp);            break;
    case ui_testcard:
      m_element = new TestCardUIElement(u.tftp, u.tsp, u.sdp);          break;
    case ui_touchpaint:
      m_element = new TouchpaintUIElement(u.tftp, u.tsp, u.sdp);        break;
    case ui_text:
      m_element = new TextPageUIElement(u.tftp, u.tsp, u.sdp);          break;
    case ui_etchasketch:
      m_element = new EtchASketchUIElement(u.tftp, u.tsp, u.sdp);       break;
    case ui_testrig:
      m_element = new TestRigUIElement(u.tftp, u.tsp, u.sdp);           break;
    default:
      Serial.printf("invalid UI mode %d in allocateUIElement\n", newMode);
      m_element = m_menu;
  }

  return m_element;
}

// touch management code ////////////////////////////////////////////////////
TS_Point nowhere(-1, -1, -1);    // undefined coordinate
TS_Point firstTouch(0, 0, 0);    // the first touch defaults to 0,0,0
TS_Point p(-1, -1, -1);          // current point of interest (signal)
TS_Point prevSig(-1, -1, -1);    // the previous accepted touch signal
bool firstTimeThrough = true;    // first time through gotTouch() flag
uint16_t fromPrevSig = 0;        // distance from previous signal
unsigned long now = 0;           // millis
unsigned long prevSigMillis = 0; // previous signal acceptance time
unsigned long sincePrevSig = 0;  // time since previous signal acceptance
uint16_t DEFAULT_TIME_SENSITIVITY = 150; // min millis between touches
uint16_t TIME_SENSITIVITY = DEFAULT_TIME_SENSITIVITY;
uint16_t DEFAULT_DIST_SENSITIVITY = 200; // min distance between touches
uint16_t DIST_SENSITIVITY = DEFAULT_DIST_SENSITIVITY;
uint16_t TREAT_AS_NEW = 600;     // if no signal in this period treat as new
uint8_t MODE_CHANGE_TOUCHES = 1; // number of requests needed to switch mode
uint8_t modeChangeRequests = 0;  // number of current requests to switch mode
bool touchDBG = false;           // set true for diagnostics

void setTimeSensitivity(uint16_t s = DEFAULT_TIME_SENSITIVITY) { ////////////
  TIME_SENSITIVITY = s;
}
void setDistSensitivity(uint16_t d = DEFAULT_DIST_SENSITIVITY) { ////////////
  DIST_SENSITIVITY = d;
}
uint16_t distanceBetween(TS_Point a, TS_Point b) { // coord distance ////////
  uint32_t xpart = b.x - a.x, ypart = b.y - a.y;
  xpart *= xpart; ypart *= ypart;
  return sqrt(xpart + ypart);
}
void dbgTouch() { // print current state of touch model /////////////////////
  if(touchDBG) {
    D("p(x:%04d,y:%04d,z:%03d)", p.x, p.y, p.z)
    D(", now=%05lu, sincePrevSig=%05lu, prevSig=", now, sincePrevSig)
    D("p(x:%04d,y:%04d,z:%03d)", prevSig.x, prevSig.y, prevSig.z)
    D(", prevSigMillis=%05lu, fromPrevSig=%05u", prevSigMillis, fromPrevSig)
  }
}
const char *UIController::modeName(ui_modes_t m) {
  switch(m) {
    case ui_menu:               return "ui_menu";          break;
    case ui_configure:          return "ui_configure";     break;
    case ui_testcard:           return "ui_testcard";      break;
    case ui_touchpaint:         return "ui_touchpaint";    break;
    case ui_text:               return "ui_text";          break;
    case ui_etchasketch:        return "ui_etchasketch";   break;
    case ui_testrig:            return "ui_testrig";       break;
    default:
      D("invalid UI mode %d in allocateUIElement\n", m)
      return "invalid UI mode";
  }
}

// accept or reject touch signals ///////////////////////////////////////////
bool UIController::gotTouch() { 
  if(!u.tsp->touched()) {
    return false; // no touches
  }
    
  // set up timings
  now = millis();
  if(firstTimeThrough) {
    sincePrevSig = TIME_SENSITIVITY + 1;
  } else {
    sincePrevSig = now - prevSigMillis;
  }

  // retrieve a point
  p = u.tsp->getPoint();
  // add the following if want to dump the rest of the buffer:
  // while (! u.tsp->bufferEmpty()) {
  //   uint16_t x, y;
  //   uint8_t z;
  //   u.tsp->readData(&x, &y, &z);
  // }
  // delay(300);
  if(touchDBG)
    D("\n\np(x:%04d,y:%04d,z:%03d)\n\n", p.x, p.y, p.z)

  // if it is at 0,0,0 and we've just started then ignore it
  if(p == firstTouch && firstTimeThrough) {
    dbgTouch();
    if(touchDBG) D(", rejecting (0)\n\n")
    return false;
  }
  firstTimeThrough = false;
  
  // calculate distance from previous signal
  fromPrevSig = distanceBetween(p, prevSig);
  dbgTouch();

  if(touchDBG)
    D(", sincePrevSig<TIME_SENS.: %d...  ", sincePrevSig<TIME_SENSITIVITY)
  if(sincePrevSig < TIME_SENSITIVITY) { // ignore touches too recent
    if(touchDBG) D("rejecting (2)\n")
  } else if(
    fromPrevSig < DIST_SENSITIVITY && sincePrevSig < TREAT_AS_NEW
  ) {
    if(touchDBG) D("rejecting (3)\n")
#if UNPHONE_SPIN >= 9
  } else if(p.z < 400) { // ghost touches in 9 (on USB power) are ~300 pressure
    // or ignore: x > 1200 && x < 1700 && y > 2000 && y < 3000 && z < 450 ?
    if(touchDBG) D("rejecting (4)\n") // e.g. p(x:1703,y:2411,z:320)
#endif
  } else {
    prevSig = p;
    prevSigMillis = now;
    if(false) // delete this line to debug touch debounce
      D("decided this is a new touch: p(x:%04d,y:%04d,z:%03d)\n", p.x, p.y, p.z)
    return true;
  }
  return false;
}

/////////////////////////////////////////////////////////////////////////////
void UIController::changeMode() {
  D("changing mode from %d (%s) to...", m_mode, modeName(m_mode))
  u.tftp->fillScreen(HX8357_BLACK);
  setTimeSensitivity();         // set TIME_SENS to the default
  nextMode = (ui_modes_t) ((MenuUIElement *)m_menu)->getMenuItemSelected();
  if(nextMode == -1) nextMode = ui_menu;

  // allocate an element according to nextMode and 
  if(m_mode == ui_menu) {       // coming OUT of menu
    if(nextMode == ui_touchpaint)
      setTimeSensitivity(25);   // TODO make class member and move to TPUIE
    m_mode =    nextMode;
    m_element = allocateUIElement(nextMode);
  } else {                      // going INTO menu
    m_mode =    ui_menu;
    m_element = m_menu;
  }
  D("...%d (%s)\n", m_mode, modeName(m_mode))

  redraw();
  return;
}

/////////////////////////////////////////////////////////////////////////////
void UIController::handleTouch() {
  int temp = p.x;
  p.x = map(p.y, u.TS_MAXX, u.TS_MINX, 0, u.tftp->width());
  p.y = map(temp, u.TS_MAXY, u.TS_MINY, 0, u.tftp->height());
  // Serial.print("dbgTouch from handleTouch: "); dbgTouch(); Serial.flush();
  
  if(m_element->handleTouch(p.x, p.y)) {
    if(++modeChangeRequests >= MODE_CHANGE_TOUCHES) {
      changeMode();
      modeChangeRequests = 0;
    }
  } 
}

/////////////////////////////////////////////////////////////////////////////
void UIController::run() {
  if(gotTouch())
    handleTouch();
  m_element->runEachTurn();
}

////////////////////////////////////////////////////////////////////////////
void UIController::redraw() {
  u.tftp->fillScreen(HX8357_BLACK);
  m_element->draw();
}

////////////////////////////////////////////////////////////////////////////
void UIController::message(char *s) {
  u.tftp->setCursor(0, 465);
  u.tftp->setTextSize(2);
  u.tftp->setTextColor(HX8357_CYAN, HX8357_BLACK);
  u.tftp->print("                          ");
  u.tftp->setCursor(0, 465);
  u.tftp->print(s);
}

////////////////////////////////////////////////////////////////////////////
void UIElement::drawSwitcher(uint16_t xOrigin, uint16_t yOrigin) {
  uint16_t leftX = xOrigin;
  if(leftX == 0)
    leftX = (SWITCHER * BOXSIZE) + 8; // default is on right hand side
    m_tft->fillRect(leftX, 15 + yOrigin, BOXSIZE - 15, HALFBOX - 10, WHITE);
    m_tft->fillTriangle(
      leftX + 15, 35 + yOrigin,
      leftX + 15,  5 + yOrigin,
      leftX + 30, 20 + yOrigin,
      WHITE
    );
}

////////////////////////////////////////////////////////////////////////////
void UIElement::showLine(const char *buf, uint16_t *yCursor) {
  *yCursor += 20;
  m_tft->setCursor(0, *yCursor);
  m_tft->print(buf);
}
