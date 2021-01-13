# gensation

This project contains multiple programs for M5Stick handling in Gensation.

1) WiFiAPwithDoubleHttpServer
Used to allow bidirectional communication between 2 M5StickCs.
One of the sticks "stick #1" opens a Wifi accesspoint with a fixed IP. The other stick "stick #2) continuously tries to connect to the WIFI AP.
Both the open a webserver on port 80.
Once stick #2 shared his IP address to stick #1 (through API call) a two-way communication is possible.
Possible communication: Button clicks on either stick sends the button's name (A or B) to the other stick.