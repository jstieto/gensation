# gensation

This project contains multiple programs for M5Stick handling in Gensation.

# WiFiAPwithDoubleHttpServer
Used to allow m:n communication between a fixed number of M5StickCs.
One of the sticks opens a Wifi accesspoint with a fixed IP. The other sticks try to connect to this WIFI AP.
All sticks open a webserver on port 80.
The program servers 3 basic functionalities which can be en-/disabled at compile time:
1) Send the angle of the stick to all other registered sticks. This is an extension of the Prost2gether program. See the description below for usage details!
2) Send a signal to all other registered sticks and let them vibrate when button-A is clicked
3) Show all registered sticks on the screen when button-B is clicked

# Prost2gether
Used to measure the the current angle (y axis) of the gyro of the stick.
The measured angle is used to draw a simulated surface of a liquid on the stick.
If you attach the stick on a drinking glass, do it like that:
- the usb-connector of the stick should look downwards
- the stick should attached on the opposite side of the glasses drinking handle.

# PowerTest
Used to test how long the M5Sticks can ping a server and process a simple answer from the server.
The server code (node.js) is also contained.


# M5StickC Pins
  0  - 	G0: 	ADC, PWM, touch, has the microphone connected to it
  26 - 	G26: 	ADC, PWM, DAC
  32 - 	 		ADC, PWM, touch, I2C: On the Grover connector 			-> andere Seite des M5Sticks im weißen Anschluss
  33 - 	 		ADC, PWM, touch, I2C, on the Grover connector			-> andere Seite des M5Sticks im weißen Anschluss 
  36 - 	G36: 	ADC, PWM, input only,  has the ESP’s hall effect sensor


## How to connect the Vibration motor to the M5StickC
Vibration Motor					M5StickC
  IN				->			  G26
  VCC				->			  5V (out)
  GND				->			  GND