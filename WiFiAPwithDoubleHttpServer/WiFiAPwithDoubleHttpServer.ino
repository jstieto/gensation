#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <M5StickC.h>
#include <HTTPClient.h>
#include <analogWrite.h>

// Config area
// Use only one of those two names
//String stickname = "1";      // mit Armband
//String stickname = "2";      // ohne allem
String stickname = "3";      // mit Gummiband (JS privater M5Stick)

// The wifi accesspoint will only be started on this stick
String ssid = "1";                     // ssid must be one of the sticks' name. Suggestion: Stick "1" is the access point


// Main area
const int MAX_BROADCAST_LISTENERS = 5;
String broadcastListenerIPs[MAX_BROADCAST_LISTENERS] ;
const String GATEWAY_IP = "192.168.4.1";
const int port = 80;
WiFiServer server(port);
const int analogOutPin = 26; // 0 (G0), 26 (G26) works 36 (G36) does not work
bool vibrationClickStatusOn = false; 
const String xx = ""; // this is a dummy string to easily allow string concatenation. Simply use it as the first element in your concat operation with + symbols...
String myIp = "";
String enqueuedUrlPath = "";

void setup() {
  //make sure to add M5.begin since otherwise the M5 commands won't work -cough-
  M5.begin();
  delay(10);
  clearScreen(BLUE);
  M5.Lcd.println("Ready");
  
  Serial.begin(115200);
  Serial.println();

  if (amIaccesspoint())    
    startWifiAccessPoint();  
  else     
    reconnectToWifi();
    
  server.begin();
  Serial.println("HTTP server started on port " + port);

  pinMode( analogOutPin , OUTPUT);  // Must be a PWM pin. This is probably optional.
  
  for (int i=0; i<MAX_BROADCAST_LISTENERS; i++) 
    broadcastListenerIPs[i]="";
}


void loop() {
  
  M5.update();   
  if (M5.BtnA.isPressed()) {    
    Serial.println("Button A isPressed"); 
    if (vibrationClickStatusOn == false) { 
      vibrationClickStatusOn = true; 
      String cmd = "broadcastVibration?status=ON&sourceIp="+myIp;
      if (amIaccesspoint())
        broadcast(cmd);
      else
        checkedHttpRequest(xx + "http://" + GATEWAY_IP + "/" + cmd);
    }
  }
  if (M5.BtnA.isReleased()) {             // This code block is always true while BtnA is not pressed!!    
    if (vibrationClickStatusOn == true) {      
      vibrationClickStatusOn = false;     
      String cmd = "broadcastVibration?status=OFF";
      if (amIaccesspoint())
        broadcast(cmd);
      else
        checkedHttpRequest(xx + "http://" + GATEWAY_IP + "/" + cmd);
    }
  }

  if (M5.BtnB.wasPressed()) {
    Serial.println("Button B was pressed");    
    /*
    // Send button click event
    String cmd = "/broadcastButtonClick?buttonName=B";
    if (amIaccesspoint())
      broadcast(cmd);
    else
      checkedHttpRequest(xx + "http://" + GATEWAY_IP + "/" + cmd);
    */

    // Re-setup
    setup();
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

            // Relevant for gateway device only
            if (header.startsWith("GET /registerBroadcastListener")) {                  
              String newPartnerIp = getUrlParameter(header, "ip");
              registerBroadcastIP(newPartnerIp);
              Serial.println(xx + "      --> Ip registered for broadcasts: " + newPartnerIp);
              M5.Lcd.println(xx + "New partner");
            }
            else
            // Relevant for any device
            if (header.startsWith("GET /broadcastButtonClick")) {
              String clickedButtonName = getUrlParameter(header, "buttonName");              
              Serial.println(xx + "      --> Partner clicked button " + clickedButtonName);
              M5.Lcd.println(xx + " partnerClick " + clickedButtonName);
            }
            else       
            if (header.startsWith("GET /broadcastVibration")) {
              String vibrationStatusStr = getUrlParameter(header, "status");
              String sourceIp = getUrlParameter(header, "sourceIp");
              bool vibrationStatus = vibrationStatusStr.equalsIgnoreCase("ON")?true:false;          
              Serial.println(xx + "      --> Partner sent a vibrationStatus=" + vibrationStatus);              
              clearScreen(vibrationStatus?RED:BLACK);            
              analogWrite(analogOutPin, vibrationStatus?255:0);
              if (amIaccesspoint())
                enqueueBroadcast("/broadcastVibration?status=" + vibrationStatusStr + "&sourceIp=" + sourceIp);
            }
            else
              Serial.println(xx + "Command not recognized: '" + header + "'");

            client.print("Yeah!<br>");
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

    dequeueBroadcast();
  }
}

void clearScreen(int color) {
  M5.Lcd.fillScreen(color);
  M5.Lcd.setCursor(0,0);  
}

void startWifiAccessPoint() {
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  const char *ssidChr = ssid.c_str();
  //const char *password = "12345678";

  WiFi.softAP(ssidChr /*, password*/);
  Serial.println("Started WIFI accesspoint SSID " + ssid);  
  M5.Lcd.println("AP started");
  
  myIp = WiFi.softAPIP().toString();
  Serial.print("AP IP address: " + myIp);  
}

void reconnectToWifi() {
  connectToWifi();
  registerForBroadcast();
}

void connectToWifi() {
  const char *ssidChr = ssid.c_str();
  WiFi.begin(ssidChr/*, password*/);  
  M5.Lcd.print("Searching for WiFi");    
  while (WiFi.status() != WL_CONNECTED) {
    //keep trying every 4 secs
    delay(4000);
    Serial.println("Searching for Wifi SSID " + ssid);
    M5.Lcd.print(".");    
  }  
  clearScreen(BLACK);
  Serial.println("Connected to the WiFi SSID " + ssid);
  M5.Lcd.println("Wifi connected!");
}

void registerForBroadcast() {
  myIp = WiFi.localIP().toString();
  Serial.println(xx + "My local IP is " + myIp);
  checkedHttpRequest(xx + "http://" + GATEWAY_IP + "/registerBroadcastListener?ip=" + myIp);
  M5.Lcd.println("Sent my IP to Gateway");
}

void broadcast(String urlPath) {
  broadcast(urlPath, "");  
}

void broadcast(String urlPath, String excludedIp) {
  Serial.println(xx + "Starting broadcast");
  for (int i=0; i < MAX_BROADCAST_LISTENERS; i++) {
    String ip = broadcastListenerIPs[i];
    if (ip.length() >= 0 && !ip.equals(excludedIp)) 
      checkedHttpRequest(xx + "http://" + ip + "/" + urlPath);     
  }    
}


bool amIaccesspoint() {
  return ssid.equalsIgnoreCase(stickname);
}


/*
 *  Checks Wifi connection before sending a http-request. 
 *  -> Tries to reconnect to Wifi if not connected.
*/
void checkedHttpRequest(String url) {
  Serial.println(xx + "Trying to call '" + url + "'");
  //check whether the connection is still alive

  if (amIaccesspoint())
    simpleHttpRequest(url);
  else {    
    if ((WiFi.status() == WL_CONNECTED)) {
      Serial.println("Wifi connected -> can start sending http request");
      simpleHttpRequest(url);
    }
    else {
      Serial.println("Wifi NOT connected -> can NOT start sending http request");
      //if WifiStatus has changed, try to reconnect to WiFi
      reconnectToWifi(); 
    }
  }
}

// This call sends an http request - without any checks
void simpleHttpRequest(String url) {  
  HTTPClient http;
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



String getUrlParameter(String header, String paramName) {
  int startIdx = header.indexOf(paramName + "=");
  if (startIdx == -1)
    return "";
  startIdx = header.indexOf("=", startIdx)+1;
  int endIdx = header.indexOf("&", startIdx);
  if (endIdx == -1)
    endIdx = header.indexOf(" ", startIdx);
  if (endIdx == -1)
    endIdx = header.length()-1;  
  return header.substring(startIdx, endIdx);
}


void registerBroadcastIP(String partnerIp) {
  int firstAvailableIPslot=-1;
  for (int i=0; i < MAX_BROADCAST_LISTENERS; i++) {
    String ip = broadcastListenerIPs[i];    
    if (ip.length() == 0) {         // found an empty slot
      firstAvailableIPslot = i;
      break;    
    }
    else if (ip.equals(partnerIp))  // -> the new partner's ip is already registered. Do not register again.
      break;  
  }

  if (firstAvailableIPslot >= 0) 
    broadcastListenerIPs[firstAvailableIPslot] = partnerIp;
  else
    Serial.println("Could not store a partner's IP to the broadcastlisteners since none is available or it was registered before already!");  
}


void enqueueBroadcast(String urlPath) {
  enqueuedUrlPath = urlPath;
}

void dequeueBroadcast() {
  if (enqueuedUrlPath.length() > 0) {
    broadcast(enqueuedUrlPath);
    enqueuedUrlPath = "";    
  }
}
