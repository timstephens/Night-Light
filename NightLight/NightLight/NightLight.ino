//#include <CapacitiveSensor.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define PIN            7
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      16


Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int delayval = 500; // delay for half a second
int lightMode = 0;

//CapacitiveSensor   cs_4_2 = CapacitiveSensor(4, 2);

void setup() {
  //cs_4_2.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  //Serial.begin(9600);

  strip.begin(); // This initializes the NeoPixel library.


  //attachInterrupt(digitalPinToInterrupt(7), isr, RISING);
}

void loop() {
  //long start = millis();
  //long total1 =  cs_4_2.capacitiveSensor(30);

  // put your main code here, to run repeatedly:
  //rainbowCycle(20);
  //colorWipe(strip.Color(128, 128, 128), 50); // Red
  
  /* 
  mode 0 == night time
  mode 1 == bedtime (should default back to night-time eventually 
  mode 2 == wakeup (rainbow)
  */
      rainbowCycle(20);

 /* switch(lightMode){
  case 0:
    colorSet(strip.Color(26, 16, 0), 50, 3); // 
    break;
  case 1:
    colorWipe(strip.Color(128, 128, 128), 50); // 
    break;
  case 2:
    rainbowCycle(20);
    break;
  default:
    colorSet(strip.Color(32, 32, 32), 50, 2); // Red
  }
  
  if (digitalRead(9) == 1) { 
    isr();
  }
  
  //Serial.print(millis() - start);        // check on performance in milliseconds
  Serial.print("\t");                    // tab character for debug windown spacing

 // Serial.println(total1);                  // print sensor output 1
//*/
}


void isr() {
  //increment the mode counter by one (and wrap around if it's > than #modes)
  lightMode++;
  if (lightMode > 2) {
    lightMode = 0;
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 ; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}


// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void colorSet(uint32_t c, uint8_t wait, uint8_t stepsize) {
  //Set pixels to a particular colour, possibly turning some off.
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    if (i % stepsize == 0) {
      strip.setPixelColor(i, c);
    } else {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
    delay(wait);
  }
}
