#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <M5StickC.h>
#include <HTTPClient.h>
#include <analogWrite.h>


/* IMPORTANT 1/2
 * - Uncomment the stick which you would like to place your code on.
 * - Choose the COM-Port the stick conncted to (See Menu: Tools -> Port). Hint: Johannes derived his stick names from the COM-Port which each respective stick automatically connected to.
 */
//String stickname = "3";
//String stickname = "4";
String stickname = "11";

/* IMPORTANT 2/2 
 * - The ssid must be the name of the stick which should act as accesspoint.
 * - The wifi accesspoint will only be started on this stick.
 */
const String ssid = "3";        
const String GATEWAY_IP = "192.168.4.1";  // fixed IP of the accesspoint



// General constants
const int MAX_BROADCAST_LISTENERS = 5;
const int analogOutPin = 26; // 0 (G0), 26 (G26) works 36 (G36) does not work with vibration motor
const String xx = ""; // this is a dummy string to easily allow string concatenation. Simply use it as the first element in your concat operation with + symbols...
const int port = 80;

// General variables
bool vibrationClickStatusOn = false; 
String myIp = "";
String enqueuedUrlPath = "";
String broadcastListenerIPs[MAX_BROADCAST_LISTENERS] ;
WiFiServer server(port);

void setup() {
  //make sure to add M5.begin since otherwise the M5 commands won't work -cough-
  M5.begin();
  delay(10);
  clearScreen(BLUE);
  M5.Lcd.println("Ready #" + stickname);
  
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
      String cmd = "broadcastVibration?status=ON&sourceIp="+myIp+"&sourceStickname="+stickname;
      if (amIaccesspoint())
        broadcast(cmd);
      else
        simpleHttpRequest(xx + "http://" + GATEWAY_IP + "/" + cmd);
    }
  }
  else /* if (M5.BtnA.isReleased()) */ {
    if (vibrationClickStatusOn == true) {      
      vibrationClickStatusOn = false;     
      String cmd = "broadcastVibration?status=OFF&sourceIp="+myIp+"&sourceStickname="+stickname;      // sourceIP must be sent because this signal must also be broadcast from the AP to all clients except the sourceIP.
      if (amIaccesspoint())
        broadcast(cmd);
      else
        simpleHttpRequest(xx + "http://" + GATEWAY_IP + "/" + cmd);
    }
  }

  if (M5.BtnB.wasPressed()) {
    Serial.println("Button B was pressed");    
    
    // Send button click event
    String cmd = "broadcastButtonClick?buttonName=B&sourceIp="+myIp+"&sourceStickname="+stickname;
    if (amIaccesspoint())
      broadcast(cmd);
    else
      simpleHttpRequest(xx + "http://" + GATEWAY_IP + "/" + cmd);
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
              String newPartnerIp = getUrlParameter(header, "sourceIp");
              String sourceStickname = getUrlParameter(header, "sourceStickname");
              registerBroadcastIP(newPartnerIp);
              Serial.println(xx + "      --> Ip registered for broadcasts: " + newPartnerIp);
              M5.Lcd.println(xx + "New stick #" + sourceStickname);
            }
            else
            // Relevant for any device
            if (header.startsWith("GET /broadcastButtonClick")) {
              String clickedButtonName = getUrlParameter(header, "buttonName");              
              Serial.println(xx + "      --> Partner clicked button " + clickedButtonName);
              M5.Lcd.println(xx + " partnerClick " + clickedButtonName);
              if (amIaccesspoint())                
                enqueueBroadcast("broadcastButtonClick?buttonName=B&sourceIp=" + myIp + "&sourceStickname="+stickname);
            }
            else       
            if (header.startsWith("GET /broadcastVibration")) {
              String vibrationStatusStr = getUrlParameter(header, "status");
              String sourceIp = getUrlParameter(header, "sourceIp");
              String sourceStickname = getUrlParameter(header, "sourceStickname");
              bool vibrationStatus = vibrationStatusStr.equalsIgnoreCase("ON")?true:false;          
              Serial.println(xx + "      --> Stick #" + sourceStickname + " (" + sourceIp + ") sent a vibrationStatusStr=" + vibrationStatusStr);
              clearScreen(vibrationStatus?RED:BLACK);            
              if (vibrationStatus) {
                M5.Lcd.setTextColor(WHITE);
                M5.Lcd.setTextSize(4);
                M5.Lcd.println(xx + " " + sourceStickname);
              }
              analogWrite(analogOutPin, vibrationStatus?255:0);
              if (amIaccesspoint())
                enqueueBroadcast("broadcastVibration?status=" + vibrationStatusStr + "&sourceIp=" + sourceIp + "&sourceStickname="+sourceStickname);
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
  }
  
  dequeueBroadcast();


  // if e.g. the AP needs to reboot and we loose Wifi -> we need to reconnect + re-register our IP for broadcasts
  if (!amIaccesspoint() && WiFi.status() != WL_CONNECTED) {
    Serial.println("I am NOT the accesspoint and lost wifi -> reconnect!");
    reconnectToWifi();
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
  M5.Lcd.println("Accesspiont/SSID '" + ssid + "' started");
  
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
  M5.Lcd.print("Searching for WiFi" + ssid);    
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
  simpleHttpRequest(xx + "http://" + GATEWAY_IP + "/registerBroadcastListener?sourceIp=" + myIp + "&sourceStickname=" + stickname);
  M5.Lcd.println("Sent my IP to Gateway");
}

void broadcast(String urlPath) {
  broadcast(urlPath, "");  
}

void broadcast(String urlPath, String excludedIp) {
  Serial.println(xx + "Starting broadcast");
  if (excludedIp.length() > 0)
    Serial.println(xx + "  > Excluding IP " + excludedIp + " from broadcast");
  Serial.println("Registered IPs for broadcasts are:");
  for (int i=0; i < MAX_BROADCAST_LISTENERS; i++) {
    String ip = broadcastListenerIPs[i];
    Serial.println(xx + " BroadcastIP[" + i + "]: '" + ip + "'");
  }
  
  for (int i=0; i < MAX_BROADCAST_LISTENERS; i++) {
    String ip = broadcastListenerIPs[i];
    if (ip.length() > 0) {
      if (!ip.equals(excludedIp)) {
        Serial.println("Sending urlPath '" + urlPath + "' to IP '" + ip + "'");
        simpleHttpRequest(xx + "http://" + ip + "/" + urlPath);      
      }
      else
        Serial.println("The found ip '" + ip + "' is excluded!");
    } 
  }    
}


bool amIaccesspoint() {
  return ssid.equalsIgnoreCase(stickname);
}


/*
 *  Checks Wifi connection before sending a http-request. 
 *  -> Tries to reconnect to Wifi if not connected.
*/
/*
void checkedHttpRequest(String url) {
  Serial.println(xx + "Trying to call '" + url + "'");
  //check whether the connection is still alive

  if (amIaccesspoint())
    simpleHttpRequest(url);
  else {    
    if ((WiFi.status() == WL_CONNECTED)) {
      //Serial.println("Wifi connected -> Sending http request!");
      simpleHttpRequest(url);
    }
    else {
      Serial.println("Wifi NOT connected -> Http request is not sent!");
      //if is disconnected -> try to reconnect to WiFi
      reconnectToWifi(); 
    }
  }
}
*/

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
  Serial.println("Enqueueing the broadcast urlPath '" + urlPath + "'");
  enqueuedUrlPath = urlPath;
}

void dequeueBroadcast() {
  if (enqueuedUrlPath.length() > 0) {
    Serial.println("Dequeueing the broadcast urlPath '" + enqueuedUrlPath + "'");
    String excludeIp = getUrlParameter(enqueuedUrlPath, "sourceIp");
    broadcast(enqueuedUrlPath, excludeIp);
    enqueuedUrlPath = "";    
  }
}
