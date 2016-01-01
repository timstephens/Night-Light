/*

  Light Menu
  Tim Stephens
  31 December 2015
  User interface (and logic for an Arduino powered fading light alarm clock.
*/
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

    //For each 20s, increase the brightness by FADETIME(s)/20s

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
  tStruct almTime;
  Serial.println("SET THE WAKEUP TIME");
  almTime = getTimeValue();

  Serial.print("almTime");
  Serial.print(almTime.hours, DEC);
  Serial.println(almTime.mins, DEC);
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



