void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  printMenu();
}

void loop() {
  int input;
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    //handle the input
    input = Serial.read();
    Serial.print("Your input was ");
    Serial.println(input, DEC);
  }
  delay(1);
  
}


void printMenu() {
Serial.println("1: Set time");
Serial.println("2: Set Wakeup");
Serial.println("3: Set M1");
Serial.println("4: Set M2");
Serial.println("M3 is always Rainbow");

}
