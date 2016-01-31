/*

  Light Menu
  Tim Stephens
  31 December 2015
  User interface (and logic for an Arduino powered fading light alarm clock.

  NOTE
  =====

  This uses the RTCx library, which relies on passing the current Unix time around.
  Since the UI here is supposed to be simpler than that and will hide the date from
  the user, we're going to use a separate time struct for our handling, and pass
  values from that around outside the routines that actually check in with the clock
  module.


TODO:
=====

Add sensible method for alarm wakeup pattern
Add a way to configure the patterns? @DONE
Tidy up the code to remove extraneous lines @DONE
Prevent alarm from firing multiple times during the alarm minute if the fade up is short (i.e. test for rollover of the alarm time, rather than a simple equality test) @DONE
Add a way to use switches to fade lamp up and down @done(?)
Add a way to select the mode that the lamp is in (perhaps using the same switches in a sensible way).
Add a Sleep to the code so that the microcontroller isn't spinning away consuming power the whole time.


*/


#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
#include <Wire.h>
#include <RTCx.h>
#include <EEPROM.h>
#include "tStruct.h"



#define WAKEUPSTEPS 64 //Number of steps in the wakeup fader.
#define PIN            7
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      4
#define M1ADDR  16
#define M2ADDR  (4*NUMPIXELS)+M1ADDR  //Location for the two array locations in EEPROM. Note that each pattern array is in 32b format for each element. 

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
uint32_t m1[NUMPIXELS]; //Globals to hold the two different fade patterns.
uint32_t m2[NUMPIXELS];

const int upButton = 2;
const int downButton = 3;
const int armSwitch = 4; //This is an on/off switch that determines whether the alarm is armed or not...

//typedef tStruct timeStruct;
tStruct currentTime;  //global variable to hold the "current time" (when it was last checked)
tStruct alarmTime;
int gBrightness; //Current position in the fade from start to end colour in the wakeup sequence.
/*
gBrightness = 0 will set the strip to it's 'off' colour, which isn't necessarily actually off; it's just the values that are stored in m1
gBrightness = WAKEUPSTEPS will set the strip to its 'on' colour, which is the values that are stored in m2
*/




enum mode {
  runMode,
  sleepMode,
  wakeupMode
};
mode opMode;
int wakeupAddress = 2;  //Location in the EEPRMOM that will contain the alarm time that's written to it.



void printTm(Stream &str, struct RTCx::tm *tm)
{
  Serial.print(tm->tm_year + 1900);
  Serial.print('-');
  Serial.print(tm->tm_mon + 1);
  Serial.print('-');
  Serial.print(tm->tm_mday);
  Serial.print('T');
  Serial.print(tm->tm_hour);
  Serial.print(':');
  Serial.print(tm->tm_min);
  Serial.print(':');
  Serial.print(tm->tm_sec);
  Serial.print(" yday=");
  Serial.print(tm->tm_yday);
  Serial.print(" wday=");
  Serial.println(tm->tm_wday);
}

void setup() {
  int eepromReady;

  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial)

    opMode = runMode;
  strip.begin();
  Wire.begin();
  // The address used by the DS1307 is also used by other devices (eg
  // MCP3424 ADC). Test for a MCP7941x device first.
  uint8_t addressList[] = {
    RTCx::MCP7941xAddress,
    RTCx::DS1307Address
  };

  // Autoprobe to find a real-time clock.
  if (rtc.autoprobe(addressList, sizeof(addressList))) {
    // Found something, hopefully a clock.
    Serial.print("Autoprobe found ");
    switch (rtc.getDevice()) {
      case RTCx::DS1307:
        Serial.print("DS1307");
        break;
      case RTCx::MCP7941x:
        Serial.print("MCP7941x");
        break;
      default:
        // Ooops. Must update this example!
        Serial.print("unknown device");
        break;
    }
    Serial.print(" at 0x");
    Serial.println(rtc.getAddress(), HEX);
  }
  else {
    // Nothing found at any of the addresses listed.
    Serial.println("No RTCx found");
    return;
  }

  currentTime = getTimeFromRTC();

  EEPROM.get(0, eepromReady);  //Check to see whether the first byte of EEPROM contains a magic character (which is written when the alarm is set).
  //If it's not the magic byte, this means that the alarm should be set to something before first run.
  //This is to stop the case where EEPROM values are undefined and cause the alarm to be set to a deeply inconventient time in the middle of the night.

  if (eepromReady == 0b10101010) {
    EEPROM.get(wakeupAddress, alarmTime);
  } else {
    alarmTime.hours = 7;
    alarmTime.mins = 0;
  }


  //Read the alarm patterns into RAM
  EEPROM.get(M1ADDR, m1);
  EEPROM.get(M2ADDR, m2);

  printMenu();


  gBrightness = 0; //Default to off when the system first starts up.

  pinMode(upButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  //Handle the pressing of the buttons.
  //  attachInterrupt(upButton, handleUpButton, RISING);
  //  attachInterrupt(downButton, handleDownButton, RISING);
}

//==============================================================================================================================
void loop() {
  String input;
  tStruct readTime; //a holder space to put time data that's read back from the clock.



  //========================================
  //UI stuff with the serial port
  if (Serial.available()) {
    //handle the input
    // while(input !='\n'){
    input = Serial.readStringUntil('\n');
    //}
    Serial.print("Your input was ");
    Serial.println(input);
    if (input.charAt(0) == '1') {
      setTime();
    } else if (input.charAt(0) == '2') {
      setWakeup();
    } else if (input.charAt(0) == '3') {
      getPixelColour(m1);
    } else if (input.charAt(0) == '4') {
      getPixelColour(m2);
    }  else if (input.charAt(0) == '5') {
      if (saveSettings()) { //write the globals to the EEPROM
        Serial.println("Settings saved OK");
      } else {
        Serial.println("ERROR. Settings not saved");
      }
    } else if (  input.charAt(0) == '6') {
      for (int i = 0; i < ( (M2ADDR + (4 * NUMPIXELS))); i++) {
        Serial.print(i, DEC);
        Serial.print(" ");
        Serial.println(EEPROM.read(i), DEC);
      }
    } else {
      Serial.println("Command not recognised");
    }
    printMenu();
  }


 

  //====================================
  //Lighting mode handlers


  //Check that the alarm time hasn't passed
  readTime = getTimeFromRTC();
  Serial.print("Brightness = ");
  Serial.println(gBrightness, DEC);


  if (opMode == runMode) {
    //if (((currentTime.hours < alarmTime.hours) && (currentTime.mins < alarmTime.mins)) && ((readTime.hours > alarmTime.hours) || (readTime.mins > alarmTime.mins))) {
    if ((readTime.hours == alarmTime.hours) && (readTime.mins == alarmTime.mins)) {
      //Since the last time we checked, the time has passed the alarm time. We should perform wakeup.
      opMode = wakeupMode;
      Serial.println("->wakeupMode");
      Serial.println("***********************");
      Serial.println("******* ALARM *********");
      Serial.println("***********************");
    }

    currentTime = readTime; //Advance the stored time value to the 'current time' for the next time we check.

    //Display whatever pattern we're supposed to be showing
  } else if (opMode == wakeupMode) {
    //should be fading up the lights
    //Check elapsed time since wakeup mode start, and ensure that the fade is progressing properly
    //We'll use a granularity of 20s, which seems to be smooth with the rainbow effect,
    //and then use similar logic to get the brightness fade over.
    /*
    What needs to happen here is that the pattern needs to swap from one to another.
    Probably we need to have both patterns stored in arrays as Color values, and then 'simply' fade fron one to another. This may be a pain in the proverbial though.
    //For each 20s, increase the brightness by FADETIME(s)/20s
    //setColourFade (startColour, endColour, currentStep);
    */
    gBrightness += 1; //Advance for next time round
    if ((gBrightness > WAKEUPSTEPS )) {  // && ((readTime.hours != alarmTime.hours) && (readTime.mins != alarmTime.mins))) {
      //      This should make sure that the alarm only fires once since this code should only fire once the time isn't equal to the alarm minute...
      opMode = runMode; //Go back to runMode again.
      Serial.println("->runMode");
    }
  } else if (opMode == sleepMode) {
    //should be displaying the sleep pattern.
    //Send it again to be sure
    gBrightness = 0;  //Note that gBrightness = 0 does not necessarily mean that the strip is off. If the off pattern is non-zero, you'll get light here.

  }
  
  
   //WAKEUPSTEPS are used as the quanta of fading brightnesses in the current fade-up routine. Also will use them for the manual fading settings.
  if (gBrightness < 0) {
    gBrightness = 0;
    opMode = runMode;  //How to get the thing into sleep mode
  }

  if (gBrightness > WAKEUPSTEPS) {
    gBrightness = WAKEUPSTEPS;
  }


  setStripColour();

  delay(1000);  //prevent racing
}

void handleUpButton() {
  //Handle the interrupt that's fired by the upButton being pressed.
  //This is going to set the value of the global brightness to something other than the current value
  gBrightness += 1;

  //Switch to runMode (i.e. armed for an alarm).
  //runMode tests to see whether an alarm should be fired, so the logic path if the switch is off is to enter runMode, discover the switch is off, and  then exit runMode for sleep mode
  opMode = runMode;
  Serial.println("->runMode");
}

void handleDownButton() {
  //opMode = runMode;
  gBrightness -= 1;
}

void setStripColour() {
  //A single place to set the colours of all the pixels in a loop from the various places that they could be set...
  Serial.print("setStripColour brightness=");
  Serial.println(gBrightness, DEC);
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, setColourFade (m1[i], m2[i], gBrightness));
  }
  strip.show();
}

tStruct getTimeFromRTC() {
  //Get the current time from the i2c RTC
  //Will set the global time variable
  tStruct myTimeStruct;
  struct RTCx::tm tm;  //From RTCx library, this is a C-style time struct.
  rtc.readClock(tm);  //Load the current timestamp into the tm variable
  //printTm(Serial, &tm); //Print the time from the RTC

  myTimeStruct.hours =  tm.tm_hour;
  myTimeStruct.mins = tm.tm_min;

  // Serial.println("getTimeFromRTC");
  return myTimeStruct;
}


//=================================
// GET PIXEL COLOUR

void getPixelColour(uint32_t pattern[]) {

  /*
  The idea in this function is that we're passed in the reference to an arbitrary array of memory (this is in order to make the coding more generic than hard-coding in the one or two patterns that we could have)
  The code then gets the RGB vlaues for a particular pixel, and then sets it in the global.
  Arrays are always passed by reference in C++, so this makes things a little bit easier.

  The patterns are going to be stored in the EEPROM, so we'll make sure to write them to the chip once the setting process has happened.

  //*/

  String input;
  int pixel, r, g, b;
  // uint32_t colorReturner;
  //Get a pixel colour and return it as a Color value thingy into the pixel
  Serial.setTimeout(10000); //Allow 10s to set the time

  Serial.println("Enter Pixel Number");
  input = Serial.readStringUntil('\n');
  pixel = input.toInt();
  Serial.println(pixel, DEC);

  Serial.println("Enter Red Value");
  input =  Serial.readStringUntil('\n');
  r = input.toInt();
  Serial.println(r, DEC);
  Serial.println("Enter Green Value");
  input = Serial.readStringUntil('\n');
  g = input.toInt();
  Serial.println(g, DEC);

  Serial.println("Enter Blue");
  input = Serial.readStringUntil('\n');
  b = input.toInt();
  Serial.println(b, DEC);

  pattern[pixel] =  strip.Color(r, g, b);
  Serial.setTimeout(1000); //Allow 10s to set the time

}


//=================================
// PRINTMENU

void printMenu() {
  currentTime = getTimeFromRTC();
  Serial.print("Time  ");
  Serial.print(currentTime.hours, DEC);
  Serial.print(":");
  Serial.println(currentTime.mins, DEC);

  Serial.print("Alarm ");
  Serial.print(alarmTime.hours, DEC);
  Serial.print(":");
  Serial.println(alarmTime.mins, DEC);
  //Display the menu on tty
  Serial.println("\n1: Set time"); //Add the newline to make the menu more readable
  Serial.println("2: Set Wakeup");
  Serial.println("3: Set Wakeup pattern");
  Serial.println("4: Set Sleep pattern");
  Serial.println("5: Save patterns to EEPROM");
  Serial.println("6: Read EEPROM");

}


void setTime() {
  struct RTCx::tm tm;  //From RTCx library, this is a C-style time struct.
  tStruct setTime;
  Serial.println("SET THE TIME");
  setTime = getTimeValue();

  rtc.readClock(&tm); //Load this to keep the date value etc. in place, and to give us a valid time struct to send back to the RTC (which otherwise will be full of undefined values)
  tm.tm_hour = setTime.hours;
  tm.tm_min = setTime.mins;
  rtc.setClock(tm);
  Serial.print("The time is ");
  Serial.print(setTime.hours, DEC);
  Serial.print(":");
  Serial.println(setTime.mins, DEC);

}


void setWakeup() {
  //Get the wakeup time from the user, and then set it into the global alarm time.
  //Wakeup should happen in a sequence of a few minutes after the alarm time that's set here.
  //We're initially going to fade up the lights

  //There's mileage in just setting the global to free a bit or RAM if necessary
  //tStruct almTime;
  Serial.println("SET THE WAKEUP TIME");
  alarmTime = getTimeValue();

  //  Serial.print("almTime");
  //  Serial.print(alarmTime.hours, DEC);
  //  Serial.print(":");
  //  Serial.println(alarmTime.mins, DEC);

  EEPROM.put(wakeupAddress, alarmTime );
  EEPROM.put(0, 0b10101010);
  //alarmTime = almTime;
}

tStruct getTimeValue() {
  //Get the time from the user over Serial, and then pump it out to the I2C RTC peripheral
  String time;
  tStruct myTimeStruct;
  int hours, mins;
  Serial.setTimeout(10000); //Allow 10s to set the time
  Serial.println("Enter the time hhmm");
  time = Serial.readStringUntil('\n');
  if (time.length() < 4) {
    Serial.println("Error, incorrect format");
  } else {

    myTimeStruct.hours = time.substring(0, 2).toInt();
    myTimeStruct.mins = time.substring(2, 4).toInt();
    //    Serial.print(time);
    //    Serial.print(" -> ");
    //    Serial.print(myTimeStruct.hours, DEC);
    //    Serial.print(":");
    //    Serial.println(myTimeStruct.mins, DEC);
  }
  Serial.setTimeout(1000); //Allow 10s to set the time

  return myTimeStruct;
}


uint32_t setColourFade (uint32_t startColour, uint32_t endColour, int currentStep) {
  //Do some computation to work out the current fade colour position, and then return it ready for setting to the strip.
  //For a first attempt, we just linearly fade from the first setpoint on each channel to the last.
  //Check out the Adafruit library for the definition of the Color member...
  /*
  Here's the line from the library that shows how it's converted:

  // Convert separate R,G,B into packed 32-bit RGB color.
  // Packed format is always RGB, regardless of LED strand color order.
  uint32_t Adafruit_NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
  }

  ALGORITHM

  Unpack start and end colour into separate RGB members (as Uint8s)
  Calculate colour step for RGB values round((start-end)/WAKEUPSTEPS) (note that this may be negative value)
  Assuming N steps, calculate the RGB values to return by multiplying colour step by current step number.
  There's a bug/issue with the logic here that causes the final value to be incorrect by a few; it's caused by the loss of precision and rounding that's going on.
  - We'll get around it for now by checking whether currentStep == WAKEUPSTEPS (the max value) and if it is, we'll just return the pattern for the max value. This will cause a slightly larger jump at the end of a fade, but since there are 64 (or more) steps, it's equal to a difference of ~1.5% in LED brightness, which I suspect is not obvious.

  */

  // WAKEUPSTEPS = Number of different colours for the fade to happen over



  //Unpack the Color values into bytes (from the ::setPixelColor() function).
  uint8_t
  r = (uint8_t)(startColour >> 16),
  g = (uint8_t)(startColour >>  8),
  b = (uint8_t)startColour;

  uint8_t
  r1 = (uint8_t)(endColour >> 16),
  g1 = (uint8_t)(endColour >>  8),
  b1 = (uint8_t)endColour;
  if (currentStep == WAKEUPSTEPS) {
    r = r1; //If we're at the max value, of brightness, just return the pattern for the max value.
    g = g1;
    b = b1;
  } else {
    //TODO Logic fail here
    //What's happening, I think is that the code is using the current strip value as r, so it approaches the correct value, but as the values converge, the different decreases and the change doesn't work properly.
    //Need to think of a better way to get this to behave.


// WHAT'S happening is that the stepsize (r1-r)/WAKEUPSTEPS is in the range 1-5, depending on the values in the pattern, so if the fade goes downwards (i.e. to darker values), the minium valu9e that you can have is 64 (i.e. 64*1), which is obviously not correct.
// Need a more robust algorithm here. 
    r = (int)r + round((((int)r1 - (int)r) / WAKEUPSTEPS) * currentStep); //Overwrite the r, g, b, values with the new ones.
    g = g + round(((g1 - g) / WAKEUPSTEPS) * currentStep);
    b = b + round(((b1 - b) / WAKEUPSTEPS) * currentStep);
  }
    Serial.print(":");
    Serial.print(r, DEC);
    Serial.print(", ");
    Serial.print(g, DEC);
    Serial.print(", ");
    Serial.println(b, DEC);
  

  //  Serial.println("setColourFade was called");
  return strip.Color(r, g, b);
}

int saveSettings() {
  //Write the M1, M2 values to the EEPROM
  //This is implemented as a separate thing, rather than doing it each time to preserve the lifetime of the EEPROM in the Arduino.
  //In reality, this may be overkill and we'll just end up overwriting it each time. Let's see.

  for (int i = 0; i < NUMPIXELS; i++) {
    EEPROM.put(M1ADDR + (4 * i), m1[i]);
  }
  for (int i = 0; i < NUMPIXELS; i++) {
    EEPROM.put(M2ADDR + (4 * i), m2[i]);

  }
  return 1;
}


