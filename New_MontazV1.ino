#include <string.h>
#include "Arduino.h"
//łączenie się z silnikami i driverami
//mikrokrorki

#define dirPinRA 1
#define stepPinRA 0
#define dirPinDA 4
#define stepPinDA 3

#define stepsPerRevolution 400

//Doba synderyczna i obrót silnika
// 1 dzień synderyczny = 86'164'000 ms
// 1 dzień = 1 BIG_BOY =130 SMALL_BOYRA (w RA)
// 360deg  = 1 BIG_BOY =  65 SMALL_BOYDA (w DA)

// 1 SMALL_BOY_RA = 662'800 ms
// 1 SMALL_BOY_RA = 2 motor step RA 
// 1 motor step RA = 400 steps * 8 (mikrokrorki)
// 1 motor step RA = 103 ms (zaokrąglając)

// 1 SMALL_BOYDA = 1'325'600 ms
// 1 SMALL_BOYDA = 2 motor step DA
// 1 motor step DA = 400 steps * 8 (mikrokrorki)
// 1 motor step DA = 207 ms (zaokrąglając)

long stepDelayRA = 103; //czas między każdym krokiem w RA
long stepDelayDA = 207; //czas między każdym krokiem w DA

boolean isPulseGuiding = false;
boolean isMoving = false;

long czasRA = stepDelayRA;   // nazbierany czas w RA (do wy-krokowania)
long czasDA = 0;             // nazbierany czas w DA

long setTime = 0;
long ileRA = 0;
long ileDA = 0;
long pom = 0;
int del = 225;               //czas między HIGH i LOW na pin STEP

long parkRA = 0;
long parkDA = 0;
//========================================================
//SETUP
//========================================================

void setup() {
  Serial.begin(115200);
  while (!Serial) ; 
  Serial.println("Connection successful.");
  
  // Deklaracja pin'ów jako output:
  pinMode(stepPinRA, OUTPUT);
  pinMode(dirPinRA, OUTPUT);
  pinMode(stepPinDA, OUTPUT);
  pinMode(dirPinDA, OUTPUT);
}

//========================================================
//LOOP
//========================================================

void loop() {
    
    
    //--------------------------------------
    // Obliczenie opóźnienia kroku
    //--------------------------------------
    setTime = millis()-setTime;                         //czas który zajęło wykonanie wszystkich poprzednich funkcji
                                                        //jeśli cokolwiek trafiło do portu COM
    if (setTime>stepDelayRA) {setTime=stepDelayRA;}
    delay(stepDelayRA-setTime);
    
      setTime = millis();     //krok sandardowy, aby kontrować dzień synderyczny Ziemi
                              //zaczęcie mierzenia czasu jednego cyklu programu

    //--------------------------------------
    
    for (int i=0; i<abs(ileRA); i++) {
        digitalWrite(stepPinRA, HIGH);
        delayMicroseconds(del);
        digitalWrite(stepPinRA, LOW);
        delayMicroseconds(del);}
        
    pom = abs(ileRA)*stepDelayRA;
    if (czasRA<0) {czasRA =(czasRA + pom);}else{czasRA = (czasRA-pom);}
    pom = 0;
    
    //--------------------------------------
                              
    for (int i=0; i<abs(ileDA); i++) {
        digitalWrite(stepPinDA, HIGH);
        delayMicroseconds(del);
        digitalWrite(stepPinDA, LOW);
       delayMicroseconds(del);}

    pom = abs(ileDA)*stepDelayDA;
    if (czasDA<0) {czasDA +=pom;}else{czasDA -=pom;}
    pom = 0;
    //--------------------------------------
    //Serial Port Read
    //--------------------------------------
                                                          
    if(Serial.available()>0)                              //jeśli coś przyszło do portu
    {
      String port_in;
      port_in = Serial.readStringUntil('#');              //zczytaj input z portu aż do '#'
                                                          // w następych krokach odczytaj o co chodzi i odbierz dodatkowe dane
      //--------------------------------------
      // PODAJ INFO (CZY DZIAŁA?)
      //--------------------------------------
      if(port_in == "I")
      { if(isPulseGuiding) {Serial.println("TRUE#");} 
                      else {Serial.println("FALSE#");}
      }
      //--------------------------------------
      // KROK NA PÓŁNOC 'N'
      //--------------------------------------
      
      if(port_in == "N") {
        port_in = "";
        port_in = Serial.readStringUntil('#');
        czasDA = czasDA + port_in.toInt();
        isPulseGuiding = true;
      }
      
      //--------------------------------------
      // KROK NA POŁUDNIE 'S'
      //-------------------------------------- 
    
      if(port_in == "S") {
        port_in = "";
        port_in = Serial.readStringUntil('#');
        czasDA = czasDA - port_in.toInt();
        isPulseGuiding = true;
      }
      
      //-------------------------------------
      // KROK NA ZACHÓD 'W'
      //--------------------------------------
      
      if(port_in == "W") {
        port_in = "";
        port_in = Serial.readStringUntil('#');
        czasRA = czasRA - port_in.toInt();
        isPulseGuiding = true;
      }

      //--------------------------------------
      // KROK NA WSCHÓD 'E'
      //--------------------------------------
      
      if(port_in == "E") {
        port_in = "";
        port_in = Serial.readStringUntil('#');
        czasRA = czasRA + port_in.toInt();
        isPulseGuiding = true; 
      }
      
      //--------------------------------------
      // Guiding - ABORDAŻ!!! 'H'
      //--------------------------------------
      
      if(port_in == "H") {
        port_in = "";
        isPulseGuiding = false;
      }

      //--------------------------------------
      // Wszystko -  ABORDAŻ!!! 'STOP'
      //--------------------------------------
      
      if(port_in == "STOP") {
        digitalWrite(stepPinRA, LOW);
        digitalWrite(stepPinDA, LOW);
        digitalWrite(dirPinRA, LOW);
        digitalWrite(dirPinDA, LOW);
        Serial.println("Stopping. Waiting for 'START#'");
        port_in = "";
        while (port_in == "") {
          if (Serial.available()>0) {
            port_in = Serial.readStringUntil('#');
            if (port_in != "START") {port_in="";
                                     }
            delay(100);
          }
        }
        Serial.println("Starting...");
      }

      //--------------------------------------
      // Move - EAST/WEST x hours y minutes z seconds OR NORTH/SOUTH x degrees y minutes z seconds
      //        M#E#xHyMzS                              M#N#xDyMzS
      //--------------------------------------

      if(port_in == "M") {
        port_in = "";
        port_in = Serial.readStringUntil('#');
        //===========================
        //        Moving East
        //===========================
        if (port_in == "E") {
          // Reading x Hours y Minutes z Seconds, converting to degrees and moving 
           Serial.print("Moving EAST  by: ");
           port_in = "";
           port_in = Serial.readStringUntil('H');
           double to_s = port_in.toInt()*3600;
           Serial.print(port_in);
           Serial.print(" Hours ");
           port_in = "";
           port_in = Serial.readStringUntil('M');
           to_s += port_in.toInt()*60;
           Serial.print(port_in);
           Serial.print(" Minutes ");
           port_in = "";
           port_in = Serial.readStringUntil('S');
           to_s += port_in.toInt()*1;
           Serial.print(port_in);
           Serial.println(" Seconds.");
           double to = (to_s*1)/240; //Input converted to degrees
           
           digitalWrite(dirPinRA, LOW); //setting direction to East
           
           // Moving by the fractional part of the number of degrees
           for (int i=0; i< (to-floor(to))*((130*160)/9); i++) {      // 130*2*400*8/360
                 digitalWrite(stepPinRA, HIGH);
                 delayMicroseconds(del);
                 digitalWrite(stepPinRA, LOW);
                 delayMicroseconds(del);
           }
           // Moving by the integer part of the number of degrees
           double zlicz = 0;
           for (int j=0; j<floor(to); j++) {
              for (int i=0; i< ((130*160)/9); i++) {      // 130*2*400*8/360
                  digitalWrite(stepPinRA, HIGH);
                  delayMicroseconds(del);
                  digitalWrite(stepPinRA, LOW);
                  delayMicroseconds(del);
              }
              zlicz += ((130*160)/9);
           }
           // Done!
           parkRA += floor((to-floor(to))*((130*160)/9) + zlicz);
        }
        
        //===========================
        //        Moving West
        //===========================
        else if (port_in == "W") {
          // Reading x Hours y Minutes z Seconds, converting to degrees and moving 
           Serial.print("Moving WEST  by: ");
           port_in = "";
           port_in = Serial.readStringUntil('H');
           double to_s = port_in.toInt()*3600;
           Serial.print(port_in);
           Serial.print(" Hours ");
           port_in = "";
           port_in = Serial.readStringUntil('M');
           to_s += port_in.toInt()*60;
           Serial.print(port_in);
           Serial.print(" Minutes ");
           port_in = "";
           port_in = Serial.readStringUntil('S');
           to_s += port_in.toInt()*1;
           Serial.print(port_in);
           Serial.println(" Seconds.");
           double to = (to_s*1)/240; //Input converted to degrees
           
           digitalWrite(dirPinRA, HIGH); //setting direction to West
           
           // Moving by the fractional part of the number of degrees
           for (int i=0; i< (to-floor(to))*((130*160)/9); i++) {      // 130*2*400*8/360
                 digitalWrite(stepPinRA, HIGH);
                 delayMicroseconds(del);
                 digitalWrite(stepPinRA, LOW);
                 delayMicroseconds(del);
           }
           
           // Moving by the integer part of the number of degrees
           double zlicz = 0;
           for (int j=0; j<floor(to); j++) {
              for (int i=0; i< ((130*160)/9); i++) {      // 130*2*400*8/360
                  digitalWrite(stepPinRA, HIGH);
                  delayMicroseconds(del);
                  digitalWrite(stepPinRA, LOW);
                  delayMicroseconds(del);
              }
              zlicz += ((130*160)/9);
           }
           // Done!
           parkRA -= floor((to-floor(to))*((130*160)/9) + zlicz);
        }
        //===========================
        //        Moving North
        //===========================
        else if (port_in == "N") {
          // Reading x Hours y Minutes z Seconds, converting to degrees and moving
           Serial.print("Moving NORTH by: ");
           port_in = "";
           port_in = Serial.readStringUntil('D');
           Serial.print(port_in);
           Serial.print(" Degrees ");
           double to_s = port_in.toInt()*3600;
           port_in = "";
           port_in = Serial.readStringUntil('M');
           Serial.print(port_in);
           Serial.print(" Minutes ");
           to_s += port_in.toInt()*60;
           port_in = "";
           port_in = Serial.readStringUntil('S');
           Serial.print(port_in);
           Serial.println(" Seconds.");
           to_s += port_in.toInt()*1;
           double to = (to_s*1)/3600; //Input converted back to degrees (double)
           
           digitalWrite(dirPinDA, LOW); //setting direction to North
           
           // Moving by the fractional part of the number of degrees
           for (int i=0; i< (to-floor(to))*((65*160)/9); i++) {      // 130*2*400*8/360
                 digitalWrite(stepPinDA, HIGH);
                 delayMicroseconds(del);
                 digitalWrite(stepPinDA, LOW);
                 delayMicroseconds(del);
           }
           // Moving by the integer part of the number of degrees
           double zlicz = 0;
           for (int j=0; j<floor(to); j++) {
              for (int i=0; i< ((65*160)/9); i++) {      // 65*2*400*8/360
                  digitalWrite(stepPinDA, HIGH);
                  delayMicroseconds(del);
                  digitalWrite(stepPinDA, LOW);
                  delayMicroseconds(del);
              }
              zlicz += ((65*160)/9);
           }
           //Done!
           
           parkDA += floor((to-floor(to))*((130*160)/9) + zlicz);
        }
        //===========================
        //        Moving South
        //===========================
        else if (port_in == "S") {
          // Reading x Hours y Minutes z Seconds, converting to degrees and moving
          Serial.print("Moving SOUTH by: ");
           port_in = "";
           port_in = Serial.readStringUntil('D');
           Serial.print(port_in);
           Serial.print(" Degrees ");
           double to_s = port_in.toInt()*3600;
           port_in = "";
           port_in = Serial.readStringUntil('M');
           Serial.print(port_in);
           Serial.print(" Minutes ");
           to_s += port_in.toInt()*60;
           port_in = "";
           port_in = Serial.readStringUntil('S');
           Serial.print(port_in);
           Serial.println(" Seconds.");
           to_s += port_in.toInt()*1;
           double to = (to_s*1)/3600; //Input converted back to degrees (double)
           
           digitalWrite(dirPinDA, HIGH); //setting direction to South
           
           // Moving by the fractional part of the number of degrees
           for (int i=0; i< (to-floor(to))*((65*160)/9); i++) {      // 130*2*400*8/360
                 digitalWrite(stepPinDA, HIGH);
                 delayMicroseconds(del);
                 digitalWrite(stepPinDA, LOW);
                 delayMicroseconds(del);
           }
           // Moving by the integer part of the number of degrees
           double zlicz = 0;
           for (int j=0; j<floor(to); j++) {
              for (int i=0; i< ((65*160)/9); i++) {      // 65*2*400*8/360
                  digitalWrite(stepPinDA, HIGH);
                  delayMicroseconds(del);
                  digitalWrite(stepPinDA, LOW);
                  delayMicroseconds(del);
              }
              zlicz += ((65*160)/9);
           }
           //Done!
           parkDA -= floor((to-floor(to))*((130*160)/9) + zlicz);
        }
         
      }
      
      //========================================
      //    Setting parking position to (O,0)
      //========================================
      
      if(port_in == "setPARK") {
        String mystring = "Found a parking spot. Remembering...";
        for (int i=0; i<33; i++) {
           delay(70);
           Serial.print(mystring.charAt(i));
           if (i==21) {delay(1500);}
        }
        for (int i=0; i<3; i++) {
           delay(1000);
           Serial.print('.');
        }
        delay(1200);
        port_in = "";
        long parkRA = 0;
        long parkDA = 0;
        Serial.println(" Done!");
      }
      
      //==========================
      //    Execution of  parking
      //==========================

      if(port_in == "PARK") {
        port_in = "";
        Serial.println("Executing parking... Got to move:");
        Serial.print(abs(parkRA));
        Serial.println(" steps in RA axis,");
        Serial.print(abs(parkDA));
        Serial.println(" steps in DA axis.");
       if (ileRA<0) {digitalWrite(dirPinRA, LOW);} else {digitalWrite(dirPinRA, HIGH);}
       if (ileDA<0) {digitalWrite(dirPinDA, LOW);}else {digitalWrite(dirPinDA, HIGH);}

       long count;
       if (abs(parkRA)>abs(parkDA)) {count=abs(parkRA);} else {count = abs(parkDA);}
       long wRA=parkRA;
       long wDA=parkDA;
       for (int i=0; i<count+1; i++) {
          if (wRA>0) {
             digitalWrite(stepPinRA, HIGH);
             delayMicroseconds(del);
             digitalWrite(stepPinRA, LOW);
             wRA--;
             if (wRA==0) {Serial.print("Parked in RA axis. ");}
          }
          if (wDA>0) {
             digitalWrite(stepPinDA, HIGH);
             delayMicroseconds(del);
             digitalWrite(stepPinDA, LOW);
             wDA--;
             if (wDA==0) {Serial.print("Parked in DA axis. ");}
          }
          delayMicroseconds(del);
       }
       Serial.println ("Stopping...");
       digitalWrite(stepPinRA, LOW);
       digitalWrite(stepPinDA, LOW);
       digitalWrite(dirPinRA, LOW);
       digitalWrite(dirPinDA, LOW);
       Serial.println("Stopped. Sleeping until 'START#'");
       port_in = "";
       while (port_in == "") {
         if (Serial.available()>0) {
           port_in = Serial.readStringUntil('#');
           if (port_in != "START") {port_in="";}
           delay(100);
         }
       }
       Serial.println("Starting...");
      }
      
      //===========================
      //        Merridian Flip
      //===========================
      
      if (port_in=="MERRY") {
           port_in="";
           long timo = millis();
           digitalWrite(dirPinRA, HIGH);
           digitalWrite(dirPinDA, LOW);
           for (long i=0; i<208000 ;i++) {
               digitalWrite(stepPinRA, HIGH);
               delayMicroseconds(del);
               digitalWrite(stepPinRA, LOW);
               delayMicroseconds(del);
               
               digitalWrite(stepPinRA, HIGH);
               digitalWrite(stepPinDA, HIGH);
               delayMicroseconds(del);
               digitalWrite(stepPinRA, LOW);
               digitalWrite(stepPinDA, LOW);
               delayMicroseconds(del);
           }
          timo = millis()-timo;
          czasRA +=timo;
          parkDA -= 208000;
          parkRA -= 416000;
      }
    }
    
    //--------------------------------------
    // Dodanie stałego kroku RA
    //--------------------------------------
    
    czasRA +=stepDelayRA;
    
    //--------------------------------------
    // Stały ruch na kompensacje ruchu Ziemii
    //--------------------------------------

    ileRA = czasRA/stepDelayRA;
    ileDA = czasDA/stepDelayDA;

    if (ileRA>0) {digitalWrite(dirPinRA, LOW);} else {digitalWrite(dirPinRA, HIGH);}
    if (ileDA>0) {digitalWrite(dirPinDA, LOW);}else {digitalWrite(dirPinDA, HIGH);}
    parkRA += ileRA;
    parkDA += ileDA;
}

//koniec:)
