/*--------------------
  Includes for the NeoPixel
*/
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

/*--------------------
  Includes for the Bluefruit
*/
#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"


/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"

/*=========================================================================*/

// Create the bluefruit object, either software serial...uncomment these lines
/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


#define NEOPIXEL_DATA_PIN 6
#define PIXEL_NUM 3

#define STRIP_ANIMATION_DELAY 100

// constants won't change. They're used here to
// set pin numbers:
const int buttonPin = 3;    // the number of the pushbutton pin


#define STATE_DISCONNECTED 0 // chasing red
#define STATE_CONNECTED 1    // pulse blue
#define STATE_GOOD 2         // green
#define STATE_WARN 3         // amber
#define STATE_ERROR 4        // red
#define STATE_PENDING 5      // chasing white
#define STATE_UNKNOWN 6      // white

// the current state of the NeoPixel strip
volatile int currentState = STATE_DISCONNECTED;


// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_NUM, NEOPIXEL_DATA_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

long debouncing_time = 100;
volatile unsigned long last_micros;

void debounceInterrupt() {
  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    buttonPressed();
    last_micros = micros();
  }
}

void buttonPressed() {
  if (ble.isConnected()) {

    Serial.print("[Send] ");
    Serial.println("GO");
    ble.println("AT+BLEUARTTX=GO\\n");
    
    currentState = STATE_PENDING;
    Serial.print("Changed state to ");
    Serial.println(currentState);
  }
}

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setupBluefruit() {
  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }

  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("******************************"));
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    Serial.println(F("******************************"));
  }
  
}

void setup() {
  Serial.begin(9600);

  setupBluefruit();

  pinMode(buttonPin, INPUT);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // initialize the array of colors to cycle through


  attachInterrupt(1, debounceInterrupt, RISING); //Modes: LOW, RISING, FALLING, CHANGE      // Interrupt for event
}

boolean hasFlashed = false;
void loop() {

  /* Wait for connection */
  if (! ble.isConnected()) {
      currentState = STATE_DISCONNECTED;
      hasFlashed = false;
  } else if(!hasFlashed) {
      currentState = STATE_CONNECTED;
  }

  checkIncoming();

  switch(currentState) {
    case STATE_DISCONNECTED:
        theaterChase(strip.Color(255, 0, 0), STRIP_ANIMATION_DELAY);
        break;
    case STATE_CONNECTED:
        colorPulse(strip.Color(0, 0, 255), 3, STRIP_ANIMATION_DELAY);
        hasFlashed = true;
        currentState = STATE_UNKNOWN;
        break;
    case STATE_GOOD:
        colorFade(strip.Color(0, 255, 0), STRIP_ANIMATION_DELAY);
        break;
    case STATE_WARN:
        colorFade(strip.Color(255, 128, 0), STRIP_ANIMATION_DELAY);
        break;
    case STATE_ERROR:
        colorFade(strip.Color(255, 0, 0), STRIP_ANIMATION_DELAY);
        break;
    case STATE_UNKNOWN:
        colorFade(strip.Color(255, 255, 255), STRIP_ANIMATION_DELAY);
        break;
    case STATE_PENDING:
        theaterChase(strip.Color(255, 255, 255), STRIP_ANIMATION_DELAY);
        break;
  }

}

void checkIncoming() {
  ble.setMode(BLUEFRUIT_MODE_DATA);  
  boolean state;
  while ( ble.available() )
  {
    int c = ble.read();

    currentState = c-48;
    if(currentState < STATE_DISCONNECTED || currentState > STATE_UNKNOWN) {
      currentState = STATE_UNKNOWN;
    }
    Serial.print((char)c);

    // Hex output too, helps w/debugging!
    Serial.print(" [0x");
    if (c <= 0xF) Serial.print(F("0"));
    Serial.print(c, HEX);
    Serial.print("] ");
  } 
  ble.setMode(BLUEFRUIT_MODE_COMMAND);
}

// Fill the entire strip with one color
void colorFade(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
  delay(wait);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

// Pulse the entire strip the given number or times
void colorPulse(uint32_t c, uint8_t pulseCount, uint8_t wait) {
  for(uint8_t j=0; j<pulseCount; j++) {
    // Turn the whole strip on to the color
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
    }
    strip.show();
    delay(wait);

    // Turn the whole strip off
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, darken(c,0.5));
    }
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
#define CHASE_LENGTH 3
float chasePattern[CHASE_LENGTH];
void theaterChase(uint32_t c, uint16_t wait) {
  chasePattern[0] = 0.25;
  chasePattern[1] = 0.75;
  chasePattern[2] = 1.0;
  for (int q=0; q < CHASE_LENGTH; q++) {
    for (int i=0; i < strip.numPixels(); i=i+CHASE_LENGTH) {
      strip.setPixelColor(i, darken(c, chasePattern[q % CHASE_LENGTH]));                   //turn every third pixel on
      strip.setPixelColor(i+1, darken(c, chasePattern[(q+1) % CHASE_LENGTH]));        
      strip.setPixelColor(i+2, darken(c, chasePattern[(q+2) % CHASE_LENGTH]));   //turn every third pixel off
    }
    strip.show();

    delay(wait);
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}


uint8_t r(uint32_t c) {
  return c >> 16 & 255;
}

uint8_t g(uint32_t c) {
  return c >> 8 & 255;
}

uint8_t b(uint32_t c) {
  return c & 255;
}

/**
 * @param amount 0.0-1.0, where 1.0 has no effect on input and 0.0 will output a black color.
 */
uint32_t darken(uint32_t color, float amount) {
  return strip.Color(r(color)*amount, g(color)*amount, b(color)* amount);
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
