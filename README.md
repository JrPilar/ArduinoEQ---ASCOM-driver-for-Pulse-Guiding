# ArduinoEQ
Code for Arduino based platform to control EQ3-2 movement, implementing pulse guiding for PHD2.
Code is prepared for any arduino-like platform capable of controlling GPIO pins.

Originally, my platform was built on Arduino Uno Rev3 board. It was talking to my computer through COM port listening to Arduino serial port or COM port commands sent by ASCOM.
Whole platform consisted of two SM 57/56-1006MA step motors (400steps per revolution) controlled with two silent TMC2226 step sticks. There is a 3:1 gear ratio between both motors and both axis, to achieve better resolution of movement. Main board is capable of controlling MS pins to change micro step revolution between 1/8 1/16 1/32 and 1/64.
<img src="https://github.com/JrPilar/ArduinoEQ/assets/143182035/e31d01f7-63a7-47e2-9999-ef7aab70a25f"
idth=25% height=25%>  
Second version was built using QT PY board from Adafruid, witch made all actions faster, and whole setup slim. Also, this solution opens path to future Bluetooth or Wi-Fi control.

Platform is controlled with commands directly from Arduino serial monitor or through COM port with ASCOM driver.
