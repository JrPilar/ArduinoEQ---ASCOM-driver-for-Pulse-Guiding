#include "Arduino.h"
#include <string.h>

// ============= Table of constants ==================
#define dirPinRA 12
#define stepPinRA 11
#define dirPinDA 7
#define stepPinDA 6

#define MS1RA 9
#define MS2RA 10
#define MS1DA 3
#define MS2DA 5

#define StepTime 200
#define maxSpeed 90

float accelerationTerm = 0.00000001;
// Slewing state
boolean isPulseGuiding = true;
boolean isMoving = true;
boolean isParked = false;

//Sindereal day and RA motor revolution
// 1 sindereal day = 86'164'000 ms
// 1 day   = 1 BIG_BOY = 130 SMALL_BOY_RA (in RA)
// 1 SMALL_BOY_RA = 662'800 ms
// 1 SMALL_BOY_RA = 3.75 motor step RA 
// 1 motor step RA = 400 steps * 8 (microsteps)
// 1 motor step RA = 103 ms (floor)
// (86 164 000 / 130*3.75*400*8)
int RASindDay = 55;

// number of arcmilliseconds per step per axis
// 360*deg*60min*60sec*1000millis = 1 296 000 000 millis
// RA: 1 296 000 000 millis / (130*3.75*400*8 = 1 560 000 ) = 831 ms
// DA: 1 296 000 000 millis / ( 65*3*400*8 = 624 000 ) = 2077 ms
long stepDelayRA = 831;
long stepDelayDA = 2077;

//number of steps to do to park
long parkRA = 0;
long parkDA = 0;

//Position in millis counting from:
//          {0} going east from merridian 0*deg
//          {0} at equator, positive going north
long posRA = 0;
long posDA = 0;

//time it took for all other actions to be done
int setTime=0;

void setup() {
  Serial.begin(250000);
  while (!Serial); 
  Serial.println("Connection successful.");
  pinMode(stepPinRA, OUTPUT);
  pinMode(dirPinRA, OUTPUT);
  pinMode(stepPinDA, OUTPUT);
  pinMode(dirPinDA, OUTPUT);
  
  // Microstep pins
  pinMode(MS1RA, OUTPUT);
  pinMode(MS2RA, OUTPUT);
  pinMode(MS1DA, OUTPUT);
  pinMode(MS2DA, OUTPUT);
}

//======== Acceleration & Deceleration ========
float positiveAcceleration(float waitTime) {
  waitTime = 1/(waitTime*accelerationTerm + 1/(waitTime));
  if (waitTime < maxSpeed) {waitTime = maxSpeed;}
  return waitTime;
}
float negativeAcceleration(float waitTime) {
  waitTime = -1/(waitTime*(15*accelerationTerm) - 1/(waitTime));
  if (waitTime >maxSpeed) {waitTime = maxSpeed;}
  return waitTime;
}

//======== moving num milliarcseconds in k ridection ========
void krok(String Dir, unsigned long num) {
  int j;
  if (Dir == "E") {
    digitalWrite(dirPinRA, HIGH);
    j=stepPinRA;
    num=num/stepDelayRA;
    parkRA+=num*stepDelayRA;
    posRA+=num*stepDelayRA;
  } else if (Dir == "W") {
    digitalWrite(dirPinRA, LOW);
    j=stepPinRA;
    num=num/stepDelayRA;
    parkRA-=num*stepDelayRA;
    posRA-=num*stepDelayRA;
  } else if (Dir == "N") {
    digitalWrite(dirPinDA, LOW);
    j=stepPinDA;
    num=num/stepDelayDA;
    posDA+=num*stepDelayDA;
    parkDA+=num*stepDelayDA;
  } else if (Dir == "S") {
    digitalWrite(dirPinDA, HIGH);
    j=stepPinDA;
    num=num/stepDelayDA;
    posDA-=num*stepDelayDA;
    parkDA-=num*stepDelayDA;
  }
  //Serial.println(num);
  float StepDel = StepTime;
  float multi = 1.5;
  boolean pointer = false;
  if (num<100) {pointer = true;}  
  if (Dir=="DA") {multi=3;}
  
  if (isMoving == true) {
    for (int i=0; i<num; i++) {
      digitalWrite(j, HIGH);
      digitalWrite(j, LOW);
      delayMicroseconds(floor(StepDel*multi)+90);
      if (num-i<2000) {pointer = true;}
      if (pointer==true) 
        {StepDel=negativeAcceleration(StepDel);}
      else
        {StepDel=positiveAcceleration(StepDel);}
    };
  };
}

//======== making H#M#S#/D#M#S steps in Dir direction ========
void MoveMe() {
  String Dir= Serial.readStringUntil('#');
  String port_in;
  Serial.print("Moving ");
  Serial.print(Dir);
  Serial.print(" by: ");
  if (Dir=="E" || Dir == "W")
    {
      port_in = Serial.readStringUntil('H');
  } else{
      port_in = Serial.readStringUntil('D');
  }
  short hourdeg = port_in.toInt();
  port_in = Serial.readStringUntil('M');
  short minute = port_in.toInt();
  port_in = Serial.readStringUntil('S');
  short second = port_in.toInt();

  double toMillis;
  // converting hour/deg + minutes + seconds back to number of milliseconds for the motor to do
  if (Dir=="E" || Dir == "W")
  {
    toMillis=floor((hourdeg*15.*60.*60. + minute*15.*60. + second*15.)*1000.);
    Serial.print(hourdeg);
    Serial.print("H ");
  }else{
    toMillis=floor((hourdeg*60.*60. + minute*60. + second)*1000.);
    Serial.print(hourdeg);
    Serial.print("D ");
  }
  Serial.print(minute);
  Serial.print("M ");
  Serial.print(second);
  Serial.println("S");
  // doing steps
  krok(Dir, toMillis);
}

//======== Setting parking position / Executing parking / Unparking =======
void SetPark() {
  parkDA = 0;
  parkRA = 0;
  Serial.println("Parking spot saved.");
}
void Park() {
  if (parkRA<0) {krok("E", abs(parkRA)/2);}else{krok("W", abs(parkRA)/2);}
  if (parkDA<0) {krok("N", abs(parkDA));}else{krok("S", abs(parkDA));}
  if (parkRA<0) {krok("E", abs(parkRA)/2);}else{krok("W", abs(parkRA)/2);}
  parkRA=0;
  parkDA=0;
  Serial.println("Parking done");
  stopAllMoving();
  isParked = true;
}

void UnPark() {
  isParked = false;
  isMoving = true;
  Serial.println("Unparked. Started moving.");
}
//========= Stops/starts moving ==========

void stopAllMoving() {
  if (isParked==true) {Serial.println("Already Parked. Stopped automaticaly.");}
  isMoving = false;
}

void startAllMoving() {
  if (isParked==false) {
    isMoving = true;
  }else{
    Serial.println("Mount Parked. To start moving use 'UNPARK#'.");
  }
}

//========= Stops/starts Pulse guiding =========
void stopPulseGuiding() {
  if (isPulseGuiding==true) {
    isPulseGuiding=false;
  }
}
void startPulseGuiding() {
  if (isPulseGuiding==false) {
    isPulseGuiding=true;
    Serial.println("Started.");
  }else{
    Serial.println("Already going.");
  }
}

//============ Is Pulse Guiding? ============
void IsPulse() {
  if (isPulseGuiding==true)
  {
    Serial.println("TRUE#");
  }else{
    Serial.println("FALSE#");
  }
}

void IsParked() {
  if (isParked==true)
  {
    Serial.println("TRUE#");
  }else{
    Serial.println("FALSE#");
  }
}

//============ Merridian Flip ================
void Merri() {
    krok("W", 324000000);
    krok("N", 648000000-posDA);
    krok("W", 324000000);
    Serial.println("Merridian flip done!");
}

//============ Set Position ===============
void SetPos() {
  String port_in;
  port_in = Serial.readStringUntil('#');
  // I've decided to use arcseconds which originated from degrees, not hours
  if (port_in=="RA") {
      port_in=Serial.readStringUntil('H');
      posRA =port_in.toInt()*15*60*60*1000;           //HOURS -> DEG -> milliarcsec
      port_in=Serial.readStringUntil('M');
      posRA+=port_in.toInt()*15*60*1000;              //MINUTES -> HOURS -> DEG -> milliarcsec
      port_in=Serial.readStringUntil('S');
      posRA+=port_in.toInt()*15*1000;                 //SECONDS -> HOURS 0> DEG -> milliarcsec
  }else {
    // convertion for declination is tricky because we have negative coordinates
    // in this system thus we have to check for the sign of degrees and only then
    // add/subtrackt rest
      port_in=Serial.readStringUntil('D');
      posDA =port_in.toInt()*60*60*1000;
      port_in=Serial.readStringUntil('M');
      if (posDA<0) {posDA-=port_in.toInt()*60*1000;}else{posDA+=port_in.toInt()*60*1000;}
      port_in=Serial.readStringUntil('S');
      if (posDA<0) {posDA-=port_in.toInt()*1000;}else{posDA+=port_in.toInt()*1000;}
  }
  Serial.println("Position set succesfully!");
}

// ============== Tell me position ============

void TellMyPosition() {
    int hour, RAminute, RAsecond;
    int deg, DAminute, DAsecond;
    
    double poss = posRA;
    
    hour=int(poss/(1000.*60.*60.*15.));
    poss-=double(hour)*1000.*60.*60.*15.;
    RAminute=int(poss/(1000.*60.*15.));
    poss-=double(RAminute)*1000.*60.*15.;
    RAsecond=int(poss/(1000.*15.));

    poss=posDA;

    deg=int(poss/(1000.*60.*60.));
    poss-=double(deg)*1000.*60.*60.;
    DAminute=int(poss/(1000.*60.));
    poss-=double(DAminute)*1000.*60.;
    DAsecond=int(poss/(1000.));

    Serial.print("My position is:"); 
    Serial.print(hour);
    Serial.print("H ");
    Serial.print(RAminute);
    Serial.print("M ");
    Serial.print(RAsecond);
    Serial.print("S and ");
    Serial.print(deg);
    Serial.print("D ");
    Serial.print(abs(DAminute));
    Serial.print("' ");
    Serial.print(abs(DAsecond));
    Serial.println("''");
}
// ============== Sync celestial coordinates =============

void Sync() {
    String SyncHours, SyncDegrees;
    
    SyncHours=Serial.readStringUntil('#');
    SyncDegrees=Serial.readStringUntil('#');

    posRA=long(SyncHours.toFloat()*(15.*60.*60.*1000.));  // Syncing to milliarcseconds in RA Hours(15*minutes*seconds*millis)
    posDA=long(SyncDegrees.toFloat()*(60.*60.*1000.));   // Syncing to milliarcseconds in DA Degrees(minutes*seconds*millis)
    Serial.println("Sync done.");
}

// ============== Go to position ===========

void SlewTo() {
    long destinationRA, destinationDA;
    String pom = Serial.readStringUntil('#');
    destinationRA = long(pom.toDouble()*(15.*60.*60.*1000.));
    pom = Serial.readStringUntil('#');
    destinationDA = long(pom.toDouble()*(60.*60.*1000.));
    
    Serial.print(destinationRA-posRA);
    Serial.print(" ");
    Serial.println(destinationDA-posDA);
    double stepsInRA, stepsInDA;
    if (1296000000-abs(destinationRA - posRA) < abs( posRA - destinationRA)) { //RightAccension
      krok("E", 1296000000-abs(destinationRA - posRA));
    } else {
      krok("W", abs(posRA-destinationRA));
    }

    if (destinationDA - posDA > 0) { // Declination
      krok("N", abs(destinationDA - posDA));
    } else {
      krok("S", abs(destinationDA - posDA));
    }
    Serial.println("Slew done.");
}

//============ Main loop ================
void loop() {
  //setting time delay between every sinderelal step
  //and making the sindereal day step
  SetMicroStep("RA", 0, 1);
  SetMicroStep("RA", 0, 1);
  setTime = millis()-setTime;
  if (setTime>RASindDay)
  {
    posRA -= (setTime*15);
    krok("E", (floor(setTime/RASindDay)*stepDelayRA));
    setTime-=(floor(setTime/RASindDay)*stepDelayRA)/15;
  }
  //starting the counting with missing millis from previous steps
  setTime=millis()+setTime;
  SetMicroStep("RA", 0, 0);
  SetMicroStep("RA", 0, 0);
  
  if (posRA<0) {posRA += 1296000000;}
  if (posRA>1296000000) {posRA -=1296000000;}
  
  if (Serial.available()>0) {
    String port_in = "";
    port_in = Serial.readStringUntil('#');
    if (port_in=="I") {IsPulse();} else {
      if (port_in=="E" || port_in=="W" || port_in=="N" || port_in=="S") {
        SetMicroStep("RA", 0, 1);
        SetMicroStep("RA", 0, 1);
        if (isPulseGuiding==false) {
          Serial.println("Mount stopped Pulse Guiding, none further action will be taken.");
          Serial.println("Waiting for 'START#' to start Guiding again.");
        } else {      
          String numOfSec;
          numOfSec=Serial.readStringUntil('#');
          krok(port_in, numOfSec.toInt()*15 );
        }
        SetMicroStep("RA", 0, 0);
        SetMicroStep("RA", 0, 0);
      }
      if (isParked == false) {
        if (port_in=="M") {MoveMe();}
        else if (port_in=="PARK") {Park();}
        else if (port_in=="MERRY") {Merri();}
        else if (port_in=="STOP") {stopAllMoving();}
        else if (port_in=="START") {startAllMoving();}
      }else if (port_in == "M" || port_in == "PARK" || port_in == "MERRY" || port_in == "STOP" || port_in == "START") {startAllMoving();}
      
      if (port_in=="SYNC") {Sync();}
      else if (port_in=="P") {IsParked();}
      else if (port_in=="SETPARK") {SetPark();}
      else if (port_in=="UNPARK") {UnPark();}
      else if (port_in=="TELLPOS") {TellMyPosition();}
      else if (port_in=="TELLPARK") {TellParkSpot();}
      else if (port_in=="SETPOS") {SetPos();}
      else if (port_in == "STARTPULSE") {startPulseGuiding();}
      else if (port_in=="STOPPULSE") {stopPulseGuiding();}
      else if (port_in=="SLEWTO") {SlewTo();}
    }
  }
}

void SetMicroStep(String Axis, bool MS1Axis, bool MS2Axis) {
    if (Axis=="RA") {
      digitalWrite(MS1RA, MS1Axis);
      digitalWrite(MS2RA, MS2Axis);
      if (MS1Axis==0 && MS2Axis==0) { // 8 microsteps //classical set
        RASindDay = 55;
        stepDelayRA = 831;
      }
      else if (MS1Axis==1 && MS2Axis==0) { // 32 microsteps
        RASindDay = 14;
        stepDelayRA = 207;   
      }
      else if (MS1Axis==0 && MS2Axis==1) { //64 microsteps
        RASindDay = 7;
        stepDelayRA = 104;   
      }
      else if (MS1Axis==1 && MS2Axis==1) { //16microsteps
        RASindDay = 28;
        stepDelayRA = 415;   
      }
    } 
    else if (Axis =="DA") {
      digitalWrite(MS1DA, MS1Axis);
      digitalWrite(MS2DA, MS2Axis);
      if (MS1Axis==0 && MS2Axis==0) {stepDelayDA = 2077;} // 8 microsteps
      else if (MS1Axis==1 && MS2Axis==0) {stepDelayDA = floor(2077/4);} // 32 microsteps
      else if (MS1Axis==0 && MS2Axis==1) {stepDelayDA = floor(2077/8);} //64 microsteps
      else if (MS1Axis==1 && MS2Axis==1) {stepDelayDA = floor(2077/2);} //16microsteps
    }
}

void TellParkSpot() {
  Serial.print(parkRA);
  Serial.println("RA");
  Serial.print(parkDA);
  Serial.println("DA");
}
/*
// ========== Used to flush Serial() ===========
void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}*/