# gensation

This project contains multiple programs for M5Stick handling in Gensation.

# WiFiAPwithDoubleHttpServer
Used to allow bidirectional communication between 2 M5StickCs.
One of the sticks "stick #1" opens a Wifi accesspoint with a fixed IP. The other stick "stick #2) continuously tries to connect to the WIFI AP.
Both the open a webserver on port 80.
Once stick #2 shared his IP address to stick #1 (through API call) a two-way communication is possible.
Possible communication: Button clicks on either stick sends the button's name (A or B) to the other stick.


# M5StickC Pins
  0 - 	G0: 	ADC, PWM, touch, has the microphone connected to it
  26 - 	G26: 	ADC, PWM, DAC
  32 - 	 		ADC, PWM, touch, I2C: On the Grover connector 			-> andere Seite des M5Sticks im weißen Anschluss
  33 - 	 		ADC, PWM, touch, I2C, on the Grover connector			-> andere Seite des M5Sticks im weißen Anschluss 
  36 - 	G36: 	ADC, PWM, input only,  has the ESP’s hall effect sensor


## How to connect the Vibration motor
Vibration Motor					M5StickC
  IN				->			  G26
  VCC				->			  5V (out)
  GND				->			  GND