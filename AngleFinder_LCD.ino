/*
AUTHOR: Isaiah Wagner
FILENAME: AngleFinder_LCD.ino
LANGUAGE: C++
-----------------------------------------
Description: This code is written specifically for the LCD angle displays
-----------------------------------------
Author notes: 
  Units: 16384 LSB = 1 g 
  HEX Addresses: 
    MPU = 0x69 (standard is 0x68 unless AD0 pin is pulled HIGH [or your AD0 pin is broken :)])
    MPU PWR MGMT = 0x6B
    MPU XACCOUT = 0x3B
    MPU YACCOUT = 0x3D
    MPU RST = 0x00 (0x40 for 0x6B reg)
    LCD = 0x27
    LCD CMND = 0x00
    LCD SETCURXY = 0x03
    LCD CLR = 0x0C
  Credit: LCD03, used for PRINT function
*/

#include <Wire.h>
#include <LCD03.h>

#define rad2deg 57.29578

#define B1 7
#define B2 8 

#define MPU 0x69
#define LCD 0x27

LCD03 lcd; 

float errX, errY, xval, yval;
int sXC, sYC;

void setup() {

  Serial.begin(9600); 
  Wire.begin(); 
  lcd.begin(16,2); //start things

  pinMode(B1, INPUT); 
  pinMode(B2, INPUT); //set up buttons for input 

  Wire.beginTransmission(MPU); 
  Wire.write(0x40);
  Wire.write(0x6B); 
  Wire.endTransmission(); //reset MPU

  calibrate(); //get accelerometer error for x and y 
}
void setCur(uint8_t x, uint8_t y) {
  //set new cursor position
  x++;
  y++; //might work
  Wire.beginTransmission(LCD);
  Wire.write(0x00);
  Wire.write(0x03);
  Wire.write(y);
  Wire.write(x); //dont know which order to put these in yet 
  Wire.endTransmission();
}
void clearLCD() {
  //clear screen 
  Wire.beginTransmission(LCD);
  Wire.write(0x00);
  Wire.write(0x0C);
  Wire.endTransmission();
}
void calibrate() { 

  for (int i=0; i< 100; i++) {
    //loop 100 times and find avg, avg will equal error

    Wire.beginTransmission(MPU);
    Wire.write(0x3B); 
    Wire.endTransmission(); //talk to XACCOUT

    Wire.requestFrom(MPU, 2); //get 2 bytes of X acc info
    byte X1 = Wire.read();
    byte X2 = Wire.read(); 

    int XC = int(X1 << 8) | (int)X2; //get one X acc Int

    Wire.beginTransmission(MPU);
    Wire.write(0x3D); //talk to YACCOUT
    Wire.endTransmission();

    Wire.requestFrom(MPU, 2);  //get 2 bytes of Y acc info
    byte Y1 = Wire.read();
    byte Y2 = Wire.read(); 

    int YC = int(Y1 << 8) | (int)Y2; //get one Y acc Int

    sXC = sXC + XC; 
    sYC = sYC + YC; //add most recent reading to total
  }

  errX = sXC/100; 
  errY = sYC/100; //create err variable for X and Y in units of LSB (see notes)
}
void getangle() {

  float sXA=0,sYA=0; //start sum at zero

  for (int i=0; i< 100; i++) {
    //loop 100 times and avg values 

    Wire.beginTransmission(MPU);
    Wire.write(0x3B); //talk to XACCOUT
    Wire.endTransmission(); 

    Wire.requestFrom(MPU, 2); //get 2 bytes of X acc info 
    byte X1 = Wire.read();
    byte X2 = Wire.read();

    int XA = int(X1 << 8) | (int)X2; //get one X acc Int

    Wire.beginTransmission(MPU);
    Wire.write(0x3D); //talk to YACCOUT
    Wire.endTransmission();

    Wire.requestFrom(MPU, 2); //get 2 bytes of Y acc info 
    byte Y1 = Wire.read();
    byte Y2 = Wire.read();

    int YA = int(Y1 << 8) | (int)Y2; //get one Y acc Int

    sXA = sXA + asin((XA-errX)/16384)*rad2deg;
    sYA = sYA + asin((YA-errY)/16384)*rad2deg; //convert to angle in degrees with trig
  }

  float angleX = abs(sXA/100);
  float angleY = abs(sYA/100); //find avg reading

  clearLCD();
  setCur(0,0);
  lcd.print("X Deg: ");
  setCur(0,1);
  lcd.print(angleX,1); 
  setCur(0,1);
  lcd.print("Y Deg: "); 
  setCur(8,1);
  lcd.print(angleY,1); //print x and y info to lcd 
}
void liveangle() {

  float sXA1=0, sYA1=0; //start sum at zero

  for (int i=0; i<50; i++) {
    //loop 50 times and avg values 

    Wire.beginTransmission(MPU);
    Wire.write(0x3B); //talk to XACCOUT
    Wire.endTransmission();

    Wire.requestFrom(MPU, 2); //get 2 bytes of X acc info
    byte X1 = Wire.read();
    byte X2 = Wire.read();

    int AX = int (X1 << 8) | int(X2); //get one X acc Int

    Wire.beginTransmission(MPU);
    Wire.write(0x3D); //talk to YACCOUT
    Wire.endTransmission();

    Wire.requestFrom(MPU, 2); //get 2 bytes of Y acc info
    byte Y1 = Wire.read();
    byte Y2 = Wire.read();

    int AY = int(Y1 << 8) | int(Y2); //get one X acc Int

    xval=asin((AX-errX)/16384)*rad2deg;
    yval=asin((AY-errY)/16384)*rad2deg; //convert to angle in degrees with trig

    sXA1 += xval;
    sYA1 += yval; //add to angle sum
  }

  float xangle1=sXA1/50;
  float yangle1=sYA1/50; //find avg reading

  clearLCD();
  setCur(0,0);
  lcd.print("X Deg: "); 
  setCur(0,1);
  lcd.print(abs(int(xangle1))); 
  setCur(0,1);
  lcd.print("Y Deg: "); 
  setCur(8,1);
  lcd.print(abs(int(yangle1))); //print x and y info to lcd 
  delay(300);
}
void loop() {

  int BS1=digitalRead(B1);
  int BS2=digitalRead(B2); //read button states

  setCur(0,0);
  lcd.print("B1: Live Angle");
  setCur(0,1);
  lcd.print("B2: Find Angle"); //give user options

  while ((BS1 == LOW)&(BS2 == LOW)) {
    //wait until button is pressed
    BS1=digitalRead(B1);
    BS2=digitalRead(B2); 
  }

  if (BS1 == HIGH) {
    
    delay(2000); //give button enough time to return to LOW state after press
    clearLCD();

    BS1=digitalRead(B1);
    BS2=digitalRead(B2);

    while ((BS1 == LOW) & (BS2 == LOW)) {
      //show live reading until button is pressed
      liveangle();
      BS1=digitalRead(B1);
      BS2=digitalRead(B2);
    }

    delay(2000);
  }

  if (B2 == HIGH) {

    delay(2000); //give button enough time to return to LOW state after press

    BS1=digitalRead(B1);
    BS2=digitalRead(B2);
    
    clearLCD();
    setCur(0,0);
    lcd.print("Press B2 when");
    setCur(0,1);
    lcd.print("ready..."); //give user prompt 

    while ((BS1 == LOW) & (BS2 == LOW)) {
      //wait until button is pressed
      BS1=digitalRead(B1);
      BS2=digitalRead(B2);
    }

    delay(2000);

    getangle(); //show current angle

    BS1=digitalRead(B1);
    BS2=digitalRead(B2);   

    while ((BS1 == LOW) & (BS2 == LOW)) {
      //continue to show current angle until button is pressed
      BS1=digitalRead(B1);
      BS2=digitalRead(B2);
    }
  }
} 
