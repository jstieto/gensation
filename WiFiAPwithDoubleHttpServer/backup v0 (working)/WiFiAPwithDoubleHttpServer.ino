/*
  Creates a WiFi access point and provides a web server on it.

  Steps:
  1. Connect to the access point "yourAp"
  2. Point your web browser to http://192.168.4.1/H to turn the LED on or http://192.168.4.1/L to turn it off
     OR
     Run raw TCP "GET /H" and "GET /L" on PuTTY terminal with 192.168.4.1 as IP address and 80 as port

  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <M5StickC.h>
#include <HTTPClient.h>

// Use only one of those two names
//String currentStickname = "m5stick1";       // der mit Armband (Johannes ;-)
String currentStickname = "m5stick2";     // der ohne Armband (Johannes ;-)

// The wifi accesspoint will only be started on this stick
String ssid = "m5stick1"; // ssid must be one of the sticks' name. Usually stick #1 is the access point
String partnerIp = "";

const int port = 80;
WiFiServer server(port);

const String xx = ""; // this is a dummy string to easily allow string concatenation. Simply use it as the first element in your concat operation with + symbols...


void setup() {
  //make sure to add M5.begin since otherwise the M5 commands won't work -cough-
  M5.begin();
  delay(10);
  M5.Lcd.fillScreen(BLUE);  
  M5.Lcd.setCursor(0,0);
  M5.Lcd.println("Ready");
  
  Serial.begin(115200);
  Serial.println();

  if (ssid.equalsIgnoreCase(currentStickname)) {
    // This is the Wifi AP provider
    startWifiAccessPoint();
  }
  else {
    // This is the Wifi client
    reconnectWifi();
  }
  
  server.begin();
  Serial.println("HTTP server started on port " + port);
}


void loop() {
  
  M5.update();   
  if (M5.BtnA.wasPressed()) {
    Serial.println("Button A pressed");
    checkedHttpRequest(xx + "clicked?button=A");
  }
  if (M5.BtnB.wasPressed()) {
    Serial.println("Button B pressed");
    checkedHttpRequest(xx + "clicked?button=B");
  }

  
  WiFiClient client = server.available();   // listen for incoming http-requests = clients
  if (client) {                             // if you get a client
    Serial.println("New Client.");          
    String header = "";
    String currentLine = "";
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        
        if (c == '\n') {
          if (currentLine.length() == 0) {    // If the current line is blank, you got two newline characters in a row -> the end of the client HTTP request, so send a response:
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK) and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println(); // blank line -> end of http-header

            
            // Check to see if the client request was "GET /H" or "GET /L":
            if (header.endsWith("GET /H")) {
              Serial.println("      --> GET /H"); 
            }
            if (header.endsWith("GET /L")) {          
              Serial.println("      --> GET /L"); 
            }
            if (header.startsWith("GET /set?myip=")) {
              int startIdx = 14;
              int endIdx = header.indexOf(" ", startIdx);
              if (endIdx == -1) 
                endIdx = header.length()-1;              
              //Serial.println(xx + "      --> The client's ip address ranges from " + startIdx + " to " + endIdx);
              partnerIp = header.substring(startIdx, endIdx);
              Serial.println(xx + "      --> The client's ip address is " + partnerIp);
              client.print(xx + "Your IP is " + partnerIp + "<br><br>");
            }
            if (header.startsWith("GET /clicked?button=")) {
              int startIdx = 20;
              int endIdx = header.indexOf(" ", startIdx);
              if (endIdx == -1) 
                endIdx = header.length()-1;              
              String clickedBtn = header.substring(startIdx, endIdx);
              Serial.println(xx + "      --> Partner clicked button " + clickedBtn);
              client.print(xx + "Partner clicked button " + clickedBtn + "<br><br>");
              M5.Lcd.println(xx + " partnerClick " + clickedBtn);
            }

            


            // the content of the HTTP response follows the header:
            if (partnerIp.length() > 0)
              client.print(xx + "Your IP is: " + partnerIp);
            client.print("Click <a href=\"/H\">here</a> to turn ON the LED.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br>");
            
            client.println(); // The HTTP response ends with another blank line:
            break;  // break out of the while loop:
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }      
    }
    // close the connection:
    header = "";
    client.stop();
    Serial.println("Client Disconnected.");
  }
}



void startWifiAccessPoint() {
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  const char *ssidChr = ssid.c_str();
  //const char *password = "12345678";

  WiFi.softAP(ssidChr /*, password*/);
  Serial.println("Started WIFI accesspoint SSID " + ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void reconnectWifi() {
  connectToWifi();    
  partnerIp = "192.168.4.1";
  sendMyIp();
}

void connectToWifi() {
  const char *ssidChr = ssid.c_str();
  WiFi.begin(ssidChr/*, password*/);

  while (WiFi.status() != WL_CONNECTED) {
    //keep trying every 4 secs
    delay(4000);
    Serial.println("Connecting to Wifi SSID " + ssid);
  }  
  Serial.println("Connected to the WiFi SSID " + ssid);  
}

void sendMyIp() {
    String myIp = WiFi.localIP().toString();
    Serial.println(xx + "My local IP is " + myIp);
    checkedHttpRequest(xx + "set?myip=" + myIp);
}




void checkedHttpRequest(String urlPath) {
  Serial.println(xx + "Trying to send '" + urlPath + "' to " + partnerIp);
  //check whether the connection is still alive

  if (ssid.equalsIgnoreCase(currentStickname)) {
    if (partnerIp.length() > 0) {
      Serial.println("Partner's IP is available -> can start sending http request");
      sendHttpRequest(urlPath);
    }
    else
      Serial.println("The partner has not yet sent his IP -> cannot send anything to him!");
  } else {    
    if ((WiFi.status() == WL_CONNECTED)) {
      Serial.println("Wifi connected -> can start sending http request");
      sendHttpRequest(urlPath);
    }
    else {
      Serial.println("Wifi NOT connected -> can NOT start sending http request");
      //if WifiStatus has changed, try to reconnect to WiFi
      reconnectWifi(); 
    }
  }
}


// This call sends an http request - without any WIFI checks
void sendHttpRequest(String urlPath) {  
  HTTPClient http;

  //send data to echo service and add current counter value
  String url = "http://" + partnerIp + "/" + urlPath;
  Serial.println(xx + "URL is " + url);
  http.begin(url);

  //send httpRequest and check response code
  int httpCode = http.GET();

  if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(xx + "Got httpCode: " + httpCode);
      Serial.println(xx + "Got payload: " + payload);
  }
  else {
    //httpCode is negative -> guess the request failed :P
    Serial.println("Error on HTTP request");
  }
  //stop the connection to free resources
  http.end();
}
