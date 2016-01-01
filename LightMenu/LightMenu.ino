
//
//typedef struct  {
//  int hours;
//  int mins;
//}tStruct;
#include "tStruct.h"
//typedef tStruct timeStruct; 
tStruct timeStruct;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while(!Serial)
  
  
  printMenu();
}

void loop() {
  String input;
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    //handle the input
   // while(input !='\n'){
    input = Serial.readStringUntil('\n');
    //}
    Serial.print("Your input was ");
    Serial.println(input);
    if (input.charAt(0) == '1') {
      getTime();
    } else if (input.charAt(0) == '2'){
      getWakeup();
    } else {
      Serial.println("Something \n");
    }
    printMenu();
  }
  delay(1);
  
}


void printMenu() {
Serial.println("\n1: Set time"); //Add the newline to make the menu more readable
Serial.println("2: Set Wakeup");
Serial.println("3: Set M1");
Serial.println("4: Set M2");
Serial.println("M3 is always Rainbow");

}


void getTime() {
  tStruct curTime;
  Serial.println("SET THE TIME");
  curTime = getTimeValue();
  
  Serial.print("getTime");
  Serial.print(curTime.hours, DEC);
  Serial.println(curTime.mins, DEC);
}


void getWakeup(){
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
    
    myTimeStruct.hours = time.substring(0,2).toInt();
    myTimeStruct.mins = time.substring(2,4).toInt();
    Serial.print(time);
    Serial.print(" -> ");
    Serial.print(myTimeStruct.hours, DEC);
    Serial.println(myTimeStruct.mins, DEC);
  }
  return myTimeStruct;
}



