#include <Adafruit_NeoPixel.h>

#define BUTTON_PIN   0              // the pin where the push button is connected to
#define PIXEL_PIN    1              // Digital IO pin connected to the NeoPixels.
#define PIXEL_COUNT 10              // How many LEDs are connected in a row

#define CONFIG_START 5000L          // time to press the push until config mode (milliseconds)
#define CONFIG_WAIT_FOR_SAVE 10000L // time to wait until config is saved (milliseconds)
#define CONFIG_UNIT 60000L          // how many ms is one pixel in the config (60000 = 1 minute)
#define DEFAULT_UNITS 2             // default units at startup

// State definitions
#define STATE_PAUSE 0
#define STATE_RUN 1
#define STATE_CONFIG 2
#define STATE_WILL_GO_CONFIG 3

// configure the Neopixel strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_RGB + NEO_KHZ800);

// internal variables
bool oldButton = HIGH;
long lowSince = 0L;
long highSince = 0L;
int state;
long timer;     
int pauseLED = 0;
int configLED = DEFAULT_UNITS - 1;
long timerStarted = 0L;
long currentTime = 0L;

// setup the whole thing
void setup() {
  // use the button pin in mode "INPUT_PULLUP" so we don't need a resistor
  // the button then needs to connect GND
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // initialize the neopixel strip
  strip.begin();
  strip.show();

  // set the default timer
  timer = DEFAULT_UNITS * CONFIG_UNIT;

  // we assume the the push button is not pressed at startup
  highSince = millis();
  
  // start the pause mode
  startPause();
}

// the main loop
//
// as we need a immediate reaction on the push button,
// the loop should return very fast.
// This has big impact on the realization of the LED control.
void loop() {
  // Get current button state.
  bool newButton = digitalRead(BUTTON_PIN);
  currentTime = millis();

  // translate the button state and the old button  state
  // into what happened and call the respective function
  if (oldButton == HIGH) {
    if (newButton == LOW) {
      buttonChangedToLow();
    } else {
      buttonIsStillHigh();
    }
  } else {
    if (newButton == LOW) {
      buttonIsStillLow();
    } else {
      buttonChangedToHigh();
    }
  }

  // show the new colors in the strip
  strip.show();
  // remember the current button state
  oldButton = newButton;
  // have a break, have a ...
  delay(50);
}

void buttonChangedToLow() {
  // note the time for the change
  lowSince = currentTime;
  // nothing more to do
}

void buttonIsStillLow() {
  // check if we should go into config mode
  if (currentTime - lowSince >= CONFIG_START) {
    state = STATE_WILL_GO_CONFIG;
    showConfigUnits();
  }
}

void buttonChangedToHigh() {
  highSince = currentTime;

  switch (state) {
    case STATE_PAUSE:
         // go into the RUN state
         state = STATE_RUN;
    case STATE_RUN:
         timerStarted = currentTime;
         showTimerRun();
         break;
    case STATE_CONFIG:
         configLED = (configLED + 1) % PIXEL_COUNT;
         showConfigUnits();
         break;
    case STATE_WILL_GO_CONFIG:
         state = STATE_CONFIG;
         break;  
  }
}

void buttonIsStillHigh() {
  switch (state) {
    case STATE_PAUSE:
         showPause();
         break;
    case STATE_RUN:
         if (currentTime - timerStarted < timer) {
          showTimerRun();
         } else {
          // Party is over
          showEnd();
          startPause();
         }
         break;
    case STATE_CONFIG:
         if (currentTime - highSince >= CONFIG_WAIT_FOR_SAVE) {
          // waiting time has elapsed -> save the config
          timer = (configLED + 1) * CONFIG_UNIT;
          showConfigEnd();
          startPause();
         }
         break;
  }
}


// set all pixels to the same color
void colorWipe(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
}

// initiate the pause
void startPause() {
  state = STATE_PAUSE;
  colorWipe(strip.Color(0,0,0));
  currentTime = highSince;
  pauseLED = 0;
}

// show pause
void showPause() {
  if (currentTime - highSince >= 1000) {
    strip.setPixelColor(pauseLED, strip.Color(0,0,0));
    pauseLED = (pauseLED + 1) % PIXEL_COUNT;
    strip.setPixelColor(pauseLED, strip.Color(255,0,0));
    highSince = currentTime;
  }
}

// show the timer in percent
void showTimerRun() {
  int percent = int (((currentTime - timerStarted) * 100) / timer);
  colorWipe(strip.Color(0,0,0));

  strip.setPixelColor(percent / PIXEL_COUNT, strip.Color(0,0,255));
}

// show that the timer ended
void showEnd () {
  colorWipe(strip.Color(255,255,255));
  strip.show();

  delay(500);
}

//show the config units
void showConfigUnits() {
  colorWipe(strip.Color(0,0,0));

  strip.setPixelColor(configLED, strip.Color(0,255,0));
}

// show that the config ended
void showConfigEnd() {
  colorWipe(strip.Color(0,255,0));
  strip.show();
  delay(500);
}
