/*

  Light Menu
  Tim Stephens
  31 December 2015
  User interface (and logic for an Arduino powered fading light alarm clock.
*/
#include <Adafruit_NeoPixel.h>
#include "tStruct.h"
//typedef tStruct timeStruct;
tStruct currentTime;  //global variable to hold the "current time" (when it was last checked)
tStruct alarmTime;


enum mode {
  runMode,
  sleepMode,
  wakeupMode
};
mode opMode;

#define PIN            7
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      16
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial)
    printMenu();
  opMode = runMode;
}

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
    } else {
      Serial.println("Something \n");
    }
    printMenu();
  }


  //====================================
  //Lighting mode handlers

  if (opMode == runMode) {
    //Check that the alarm time hasn't passed
    readTime = getTimeFromRTC();
    if (((currentTime.hours < alarmTime.hours) && (currentTime.mins < alarmTime.mins)) && ((readTime.hours > alarmTime.hours) || (readTime.mins > alarmTime.mins))) {
      //Since the last time we checked, the time has passed the alarm time. We should perform wakeup. 
      opMode = wakeupMode;
    } else {
      currentTime = readTime; //Advance the stored time value to the 'current time' for the next time we check.
    }
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
    

  } else if (opMode == sleepMode) {
    //should be displaying the sleep pattern.
    //Send it again to be sure
  }


  delay(1);  //prevent racing
}

tStruct getTimeFromRTC() {
  //Get the current time from the i2c RTC
  //Will set the global time variable
 
 //TODO: Needs actual code in here
 //THIS IS A STUB FOR NOW
 tStruct myTimeStruct;

  myTimeStruct.hours = 11;
  myTimeStruct.mins = 32;
  return myTimeStruct;
}


void printMenu() {
  //Display the menu on tty
  Serial.println("\n1: Set time"); //Add the newline to make the menu more readable
  Serial.println("2: Set Wakeup");
  Serial.println("3: Set M1");
  Serial.println("4: Set M2");
  Serial.println("M3 is always Rainbow");

}


void setTime() {
  tStruct setTime;
  Serial.println("SET THE TIME");
  setTime = getTimeValue();

  Serial.print("getTime");
  Serial.print(setTime.hours, DEC);
  Serial.println(setTime.mins, DEC);
}


void setWakeup() {
  //Get the wakeup time from the user, and then set it into the global alarm time.
  //Wakeup should happen in a sequence of a few minutes after the alarm time that's set here.
  //We're initially going to fade up the lights
  
  //There's mileage in just setting the global to free a bit or RAM if necessary
  tStruct almTime;
  Serial.println("SET THE WAKEUP TIME");
  almTime = getTimeValue();

  Serial.print("almTime");
  Serial.print(almTime.hours, DEC);
  Serial.println(almTime.mins, DEC);
  alarmTime = almTime; 
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
  
  int numSteps = 32; //Number of different colours for the fade to happen over
 

  //Unpack the Color values into bytes (from the ::setPixelColor() function). 
  uint8_t
      r = (uint8_t)(startColour >> 16),
      g = (uint8_t)(startColour >>  8),
      b = (uint8_t)startColour;
  
 uint8_t
      r1 = (uint8_t)(endColour >> 16),
      g1 = (uint8_t)(endColour >>  8),
      b1 = (uint8_t)endColour;
  
  r=round(((r-r1)/numSteps)*currentStep);  //Overwrite the r, g, b, values with the new ones.
  g=round(((g-g1)/numSteps)*currentStep);
  b=round(((b-b1)/numSteps)*currentStep);
  
  return strip.Color(r,g,b);
}


