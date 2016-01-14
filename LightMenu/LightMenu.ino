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
Add a way to configure the patterns? 
Tidy up the code to remove extraneous lines 
Prevent alarm from firing multiple times during the alarm minute if the fade up is short (i.e. test for rollover of the alarm time, rather than a simple equality test)


 */
 
 
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
#include <Wire.h>
#include <RTCx.h>
#include <EEPROM.h>

#define WAKEUPSTEPS 64 //Number of steps in the wakeup fader.

#include "tStruct.h"
//typedef tStruct timeStruct;
tStruct currentTime;  //global variable to hold the "current time" (when it was last checked)
tStruct alarmTime;
int currentStep; //Current position in the fade from start to end colour in the wakeup sequence. 

enum mode {
  runMode,
  sleepMode,
  wakeupMode
};
mode opMode;
int wakeupAddress = 2;  //Location in the EEPRMOM that will contain the alarm time that's written to it. 


#define PIN            7
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      1
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

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
  uint8_t addressList[] = {RTCx::MCP7941xAddress,
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
  printMenu();
}

void loop() {
  String input;
  tStruct readTime; //a holder space to put time data that's read back from the clock.

  uint32_t startColor, endColor; 
  startColor = strip.Color(255,0,0);
  endColor = strip.Color(0,0,255);
  
  
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
    } else {
      Serial.println("Something \n");
    }
    printMenu();
  }

  //====================================
  //Lighting mode handlers


  //Check that the alarm time hasn't passed
  readTime = getTimeFromRTC();

  if (opMode == runMode) {
    currentStep=0; //Keep resetting this when we're not in alarm mode.
    
    strip.setPixelColor(1, startColor);
      strip.show();
      
    //if (((currentTime.hours < alarmTime.hours) && (currentTime.mins < alarmTime.mins)) && ((readTime.hours > alarmTime.hours) || (readTime.mins > alarmTime.mins))) {
    if ((readTime.hours == alarmTime.hours) && (readTime.mins == alarmTime.mins)) {
      //Since the last time we checked, the time has passed the alarm time. We should perform wakeup.
      opMode = wakeupMode;
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
    Serial.print(currentStep, DEC);
     strip.setPixelColor(1, setColourFade (startColor, endColor, currentStep));
      strip.show();
    currentStep +=1; //Advance for next time round
    if ((currentStep > WAKEUPSTEPS )) {  // && ((readTime.hours != alarmTime.hours) && (readTime.mins != alarmTime.mins))) {
     //      This should make sure that the alarm only fires once since this code should only fire once the time isn't equal to the alarm minute...
      opMode = runMode; //Go back to runMode again.
    }
  } else if (opMode == sleepMode) {
    //should be displaying the sleep pattern.
    //Send it again to be sure
  }


  delay(1000);  //prevent racing
}

tStruct getTimeFromRTC() {
  //Get the current time from the i2c RTC
  //Will set the global time variable
  tStruct myTimeStruct;
  struct RTCx::tm tm;  //From RTCx library, this is a C-style time struct.
  rtc.readClock(tm);  //Load the current timestamp into the tm variable
  printTm(Serial, &tm); //Print the time from the RTC

  myTimeStruct.hours =  tm.tm_hour;
  myTimeStruct.mins = tm.tm_min;

  // Serial.println("getTimeFromRTC");
  return myTimeStruct;
}


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
  Serial.println("3: Set M1");
  Serial.println("4: Set M2");
  Serial.println("M3 is always Rainbow");

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

  Serial.print("almTime");
  Serial.print(alarmTime.hours, DEC);
  Serial.print(":");
  Serial.println(alarmTime.mins, DEC);
  
  EEPROM.put(wakeupAddress, alarmTime ); 
  EEPROM.put(0, 0b10101010);
  //alarmTime = almTime;
}

tStruct getTimeValue() {
  //Get the time from the user over Serial, and then pump it out to the I2C RTC peripheral
  String time;
  tStruct myTimeStruct;
  int hours, mins;
  Serial.setTimeout(5000); //Allow 10s to set the time
  Serial.println("Enter the time hhmm");
  time = Serial.readStringUntil('\n');
  if (time.length() < 4) {
    Serial.println("Error, incorrect format");
  } else {

    myTimeStruct.hours = time.substring(0, 2).toInt();
    myTimeStruct.mins = time.substring(2, 4).toInt();
    Serial.print(time);
    Serial.print(" -> ");
    Serial.print(myTimeStruct.hours, DEC);
    Serial.print(":");
    Serial.println(myTimeStruct.mins, DEC);
  }
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
  Calculate colour step for RGB values round((start-end)/numSteps) (note that this may be negative value)
  Assuming N steps, calculate the RGB values to return by multiplying colour step by current step number.


  */

  int numSteps = WAKEUPSTEPS; //Number of different colours for the fade to happen over


  //Unpack the Color values into bytes (from the ::setPixelColor() function).
  uint8_t
  r = (uint8_t)(startColour >> 16),
  g = (uint8_t)(startColour >>  8),
  b = (uint8_t)startColour;

  uint8_t
  r1 = (uint8_t)(endColour >> 16),
  g1 = (uint8_t)(endColour >>  8),
  b1 = (uint8_t)endColour;

  r = r + round(((r1 - r) / numSteps) * currentStep); //Overwrite the r, g, b, values with the new ones.
  g = g + round(((g1 - g) / numSteps) * currentStep);
  b = b + round(((b1 - b) / numSteps) * currentStep);
  Serial.print(r, DEC);
  Serial.print(", ");
  Serial.print(g, DEC);
  Serial.print(", ");
  Serial.println(b, DEC);

  Serial.println("setColourFade was called");
  return strip.Color(r, g, b);
}


