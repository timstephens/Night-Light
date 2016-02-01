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


