# ArduinoEQ
Code for arduino based platform to control EQ3-2 movement, implementing pulse guiding for PHD2.  
Code is prepared for any arduino-like platform capable of controling GPIO pins.  
  
Oryginally, my platform was built on Arduino Uno Rev3 board. It was talking to my computer through COM port listening to arduino serial port or COM port commands sent by ASCOM.
Whole platform consisted of two SM 57/56-1006MA stepmotors (400steps per revolution) controled with two scilent TMC2226 stepsticks. There is a 3:1 gear ratio between both motors and both axis, to acive better resolution of movement. Main board is cpable of controling MS pins to change microstep revolution between 1/8 1/16 1/32 and 1/64.  
Second version was built usin QT PY board from Adafruid, witch made all actions faster, and whole setup slim. Also this solution opens path to future bluetooth or wifi control.
  
Platform is controled with commands directly from arduino serial monitor or through COM port with ASCOM driver.
