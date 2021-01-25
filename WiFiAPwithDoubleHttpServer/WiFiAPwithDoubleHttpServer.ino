#include <WiFi.h>           
#include <WiFiClient.h> 
#include <WiFiAP.h>
#include <M5StickC.h>
#include <HTTPClient.h>
#include <analogWrite.h>    // ESP32 AnalogWrite       by ERROPiX                    v 0.1.0
#include <Kalman.h>         // Kalman Filter Library   by Kristian Lauszus (TKJ...)  v 1.0.2


/* IMPORTANT 1/3  - Stick names
 * - Uncomment the stick which you would like to place your code on.
 * - Choose the COM-Port the stick is conncted to (See Menu: Tools -> PORT). Hint: Johannes derived his stick names from the COM-PORT which each respective stick automatically connected to. 
 *   This makes testing a lot easier. It also helps to write the name/number on the stick ;-)
 */
String stickname = "3";
//String stickname = "4";
//String stickname = "11";

/* IMPORTANT 2/3  - Accesspoint configuration
 * - The ssid must be the name of the stick which should act as accesspoint.
 * - The wifi accesspoint will only be started on this stick.
 */
const String ssid = "3";        
const String GATEWAY_IP = "192.168.4.1";  // fixed IP of the accesspoint

/* IMPORTANT 3/3  - Operation modes
 * Modify the following constants to define the applications behaviour.
 */
const bool BROADCAST_DRINKING_ANGLE = true;           // defaults to true
const bool BROADCAST_VIBRATION_ON_BTN_A = true;       // defaults to true
const bool BROADCAST_CLICKEVENT_ON_BTN_B = false;     // defaults to false; Mainly used for testing
const bool DRAW_REGISTERED_IPS_ON_BTN_B = true;       // defaults to false; Mainly used for testing
const bool DRAW_DRINKING_ANGLE_LOCALLY = false;       // defaults to false; Mainly used for testing

    

/* General default constants
 */ 
const int BROADCAST_DRINKING_ANGLE_MIN_ANGLE = 10;             // As soon as this angle is met -> broadcast 
const int BROADCAST_DRINKING_ANGLE_MIN_ANGLE_CHANGE = 5;      // Only resend a broadcast if the measured angle changed by at least this value
const int MAX_BROADCAST_LISTENERS = 5;
const int ANALOG_OUTPIN = 26;                                 // 0 (G0), 26 (G26) works 36 (G36) does not work with vibration motor
const String xx = "";                                         // This is a dummy string to easily allow string concatenation. Simply use it as the first element in your concat operation with + symbols...

// General variables
bool vibrationClickStatusOn = false;
bool angleIsBackToVerticalAlreadySent = false;                // If the glass is standing normal on the table (angle = 0°) we need to broadcast a single signal to all sticks to tell them that we stopped drinking.
String myIp = "";
String enqueuedUrlPath = "";
String broadcastListenerIPs[MAX_BROADCAST_LISTENERS];
String broadcastListenerSticks[MAX_BROADCAST_LISTENERS];      // the use of this variable was not fully completed (not finished developing)
WiFiServer server(80);
bool vibrationEnabled = false;

// Gyro initialisations
#define RESTRICT_PITCH // Comment out to restrict roll to ±90deg instead - please read: http://www.freescale.com/files/sensors/doc/app_note/AN3461.pdf
float accX = 0;
float accY = 0;
float accZ = 0;
float gyroX = 0;      
float gyroY = 0;
float gyroZ = 0;
float accOffsetX = 0;
float accOffsetY = 0;
float accOffsetZ = 0;
float gyroOffsetX = 11;
float gyroOffsetY = 0;
float gyroOffsetZ = 0;

#define USE_CALIBRATION 
float calibrationX = 10;
float calibrationY = -5.6;
float lowPassFilter = 0.01; // depends to delay in loop and max frequency
float lowPassGyroX = 0;
float lowPassGyroY = 0;
float lowPassGyroZ = 0;
uint32_t timer;
double dt;
Kalman kalmanX;
Kalman kalmanY;
double gyroXangle, gyroYangle;  // Angle calculate using the gyro only
double compAngleX, compAngleY;  // Calculated angle using a complementary filter
double kalAngleX, kalAngleY;    // Calculated angle using a Kalman filter

const double radConversionFactor = 3.141592 / 180;
int xStart = 40;
int yStart = 80;
int xDiff = 0;
int yDiff = 0;
int prevDegrees=-1;







void setup() {
  //make sure to add M5.begin since otherwise the M5 commands won't work -cough-
  M5.begin();
  delay(10);
  clearScreen(BLUE);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("Ready #" + stickname);
  
  Serial.begin(115200);
  Serial.println();

  if (amIaccesspoint())    
    startWifiAccessPoint();  
  else     
    reconnectToWifi();
    
  server.begin();
  Serial.println("HTTP server started on port 80");

  pinMode(ANALOG_OUTPIN , OUTPUT);  // Must be a PWM pin. This is probably optional.
  
  resetRegisteredIps();
  initGyroStaff();
}


void loop() {
  if (DRAW_DRINKING_ANGLE_LOCALLY || BROADCAST_DRINKING_ANGLE) {          //// Get gyro data
    if (!vibrationEnabled) {
      dt = (double)(micros() - timer) / 1000000; // Calculate delta time
      timer = micros();  
      // put your main code here, to run repeatedly:
      M5.MPU6886.getGyroData(&gyroX, &gyroY, &gyroZ);
      M5.MPU6886.getAccelData(&accX, &accY, &accZ);
      executeKalmanFilterOnAngles();
      if (DRAW_DRINKING_ANGLE_LOCALLY) {
        if (((int) kalAngleY) != prevDegrees) {        
          drawDrink(kalAngleY, stickname);
          prevDegrees = kalAngleY;
        } 
      } else
      if (BROADCAST_DRINKING_ANGLE) {
        if (abs(kalAngleY) > BROADCAST_DRINKING_ANGLE_MIN_ANGLE) {
          if (abs(kalAngleY-prevDegrees) > BROADCAST_DRINKING_ANGLE_MIN_ANGLE_CHANGE) {     // Resend the current drinking angle only if it changed at least by 5 degrees
            String cmd = "broadcastAngle?angle=" + String(kalAngleY,0) + "&sourceIp="+myIp+"&sourceStickname="+stickname;
            broadcast(cmd);
            prevDegrees = kalAngleY;
            angleIsBackToVerticalAlreadySent = false;
          }
        }
        else {
          if (angleIsBackToVerticalAlreadySent == false) {
            String cmd = "broadcastAngle?angle=" + String(kalAngleY,0) + "&sourceIp="+myIp+"&sourceStickname="+stickname;
            broadcast(cmd);
            prevDegrees = kalAngleY;
            angleIsBackToVerticalAlreadySent = true;
          }
        }
      }
    }
    //else
    //  Serial.println("Vibration is on -> I cannot use the gyro!");
  } else
  if (BROADCAST_VIBRATION_ON_BTN_A) {
    M5.update();   
    if (M5.BtnA.isPressed()) {    
      Serial.println("Button A isPressed"); 
      if (vibrationClickStatusOn == false) { 
        vibrationClickStatusOn = true; 
        String cmd = "broadcastVibration?status=ON&sourceIp="+myIp+"&sourceStickname="+stickname;
        broadcast(cmd);    
      }
    }
    else /* if (M5.BtnA.isReleased()) */ {
      if (vibrationClickStatusOn == true) {      
        vibrationClickStatusOn = false;     
        String cmd = "broadcastVibration?status=OFF&sourceIp="+myIp+"&sourceStickname="+stickname;
        broadcast(cmd);      
      }
    }
  } else
  if (BROADCAST_CLICKEVENT_ON_BTN_B) {
    if (M5.BtnB.wasPressed()) {
      Serial.println("Button B was pressed");
      String cmd = "broadcastButtonClick?buttonName=B&sourceIp="+myIp+"&sourceStickname="+stickname;
      broadcast(cmd);
    }
  } else
  if (DRAW_REGISTERED_IPS_ON_BTN_B) {    
    if (M5.BtnB.wasPressed()) {
      Serial.println("Button B was pressed");    
      resetRegisteredIps();
      registerMeForBroadcastRemotely();  
      clearScreen(BLUE);
      M5.Lcd.println("Registered IPs:");    
      M5.Lcd.println(getRegisteredIpsFormatted("\n"));
    }
  }

  //// Handle incoming http requests
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

            if (header.startsWith("GET /registerMe")) {                  
              String sourceIp = getUrlParameter(header, "sourceIp");
              String sourceStickname = getUrlParameter(header, "sourceStickname");
              saveBroadcastListenerIPLocally(sourceIp, sourceStickname);
              Serial.println(xx + "      --> Ip registered for broadcasts: " + sourceIp);
              setBrightness(100);
              M5.Lcd.setTextSize(1);
              M5.Lcd.println(xx + "New stick #" + sourceStickname);
              if (amIaccesspoint())
                client.println(getRegisteredIpsFormatted(";"));
            }
            else
            if (header.startsWith("GET /broadcastButtonClick")) {
              String clickedButtonName = getUrlParameter(header, "buttonName");
              String sourceStickname = getUrlParameter(header, "sourceStickname");
              Serial.println(xx + "      --> Partner clicked button " + clickedButtonName);
              setBrightness(100);
              M5.Lcd.setTextSize(1);
              M5.Lcd.println(xx + "Btn" + clickedButtonName + " by " + sourceStickname);
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
                M5.Lcd.setTextSize(1);
              }
              vibrate(vibrationStatus);              
              setBrightness(vibrationStatus?100:0);
            }
            else
            if (header.startsWith("GET /broadcastAngle")) {
              String angle = getUrlParameter(header, "angle");
              String sourceIp = getUrlParameter(header, "sourceIp");
              String sourceStickname = getUrlParameter(header, "sourceStickname");
              Serial.println(xx + "      --> Stick #" + sourceStickname + " (" + sourceIp + ") sent an angle=" + angle);
              int angleInt = angle.toInt();
              if (abs(angleInt) > BROADCAST_DRINKING_ANGLE_MIN_ANGLE) {
                setBrightness(100);
                drawDrink(angleInt, sourceStickname);
                vibrate(true);
              }
              else {
                clearScreen(BLACK);
                vibrate(false);
                setBrightness(0);
              }              
            }
            else
              Serial.println(xx + "Command not recognized: '" + header + "'");

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
  
  // if e.g. the AP needs to reboot and we loose Wifi -> we need to reconnect + re-register our IP for broadcasts
  if (!amIaccesspoint() && WiFi.status() != WL_CONNECTED) {
    Serial.println("I am NOT the accesspoint and lost wifi -> reconnect!");
    reconnectToWifi();
  }    

  //delay(50);
  delay(100); // this might save battery and reduces flickering
}


String getRegisteredIpsFormatted(String delimiter) {
  String ret="";
  for (int i=0; i < MAX_BROADCAST_LISTENERS; i++) {
    String ip = broadcastListenerIPs[i];
    if (ip.length() > 0)
      ret+=ip+delimiter;
  }
  return ret;
}

void resetRegisteredIps() {
  for (int i=0; i<MAX_BROADCAST_LISTENERS; i++) {
    broadcastListenerIPs[i]="";
    broadcastListenerSticks[i]="";
  }
  if (!amIaccesspoint()) {
    broadcastListenerIPs[0]=GATEWAY_IP;
    broadcastListenerSticks[0]=ssid;
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
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("AP/SSID " + ssid + " started");
  
  myIp = WiFi.softAPIP().toString();
  Serial.print("AP IP address: " + myIp);  
}

void reconnectToWifi() {
  connectToWifi();
  resetRegisteredIps();
  registerMeForBroadcastRemotely();  
}

void connectToWifi() {
  const char *ssidChr = ssid.c_str();
  WiFi.begin(ssidChr/*, password*/);  
  M5.Lcd.setTextSize(1);
  M5.Lcd.print("Searching for WiFi" + ssid);    
  while (WiFi.status() != WL_CONNECTED) {    
    clearScreen(BLACK);
    Serial.println("Searching for Wifi SSID " + ssid);
    M5.Lcd.println("Searching Wifi");
    M5.Lcd.print(".");    delay(1000);
    M5.Lcd.print(".");    delay(1000);
    M5.Lcd.print(".");    delay(1000);
    M5.Lcd.print(".");    delay(1000);
  }  
  clearScreen(BLACK);
  Serial.println("Connected to the WiFi SSID " + ssid);
  M5.Lcd.println("Wifi connected!");
}

void registerMeForBroadcastRemotely() {
  myIp = WiFi.localIP().toString();
  Serial.println(xx + "My local IP is " + myIp);  
  String payload = simpleHttpRequest(xx + "http://" + GATEWAY_IP + "/registerMe?sourceIp=" + myIp + "&sourceStickname=" + stickname);   
  // The payload contains a list of all IPs registered at the gateway device. Let's store the IPs locally!
  Serial.println("The following IPs are registered at the gateway: '" + payload + "'");
  if (payload.length() > 0) {
    int startIdx = 0;
    int endIdx=payload.indexOf(";", startIdx);
    while (endIdx > 0) {
      //Serial.println(xx + "StartIdx=" + startIdx + "; EndIdx=" + endIdx);
      String ip = payload.substring(startIdx, endIdx);
      Serial.println(" Registered at gateway: " + ip);
      if (!myIp.equalsIgnoreCase(ip))
        saveBroadcastListenerIPLocally(ip, "");
      else 
        Serial.println(" I'm not adding myself/my IP to my local broadcastIP-List");
      startIdx = endIdx+1;
      endIdx=payload.indexOf(";", startIdx);
    }
  }
  else
    Serial.println("No payload was received when I registered for broadcasts!");
    
  broadcast("registerMe?sourceIp=" + myIp + "&sourceStickname="+stickname, GATEWAY_IP);  // Let everybody know
  
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("Sent my IP to Gateway");
}

void broadcast(String urlPath) {
  broadcast(urlPath, "");  
}

void broadcast(String urlPath, String excludedIp) {
  Serial.println(xx + "Starting broadcast");
  if (excludedIp.length() > 0)
    Serial.println(xx + "Excluding IP " + excludedIp + " from broadcast");
  Serial.println("Registered IPs for broadcasts are:");
  for (int i=0; i < MAX_BROADCAST_LISTENERS; i++) {
    String ip = broadcastListenerIPs[i];
    String stickname = broadcastListenerSticks[i];
    Serial.println(xx + " bcIP[" + i + "]: '" + ip + "' = " + stickname);
  }
  
  for (int i=0; i < MAX_BROADCAST_LISTENERS; i++) {
    String ip = broadcastListenerIPs[i];
    if (ip.length() > 0) {
      if (!ip.equals(excludedIp)) {
        Serial.println("Sending urlPath '" + urlPath + "' to IP '" + ip + "'");
        simpleHttpRequest(xx + "http://" + ip + "/" + urlPath);      
      }
      else
        Serial.println("Not sending to '" + ip + "' as it is excluded!");
    } 
  }    
}


bool amIaccesspoint() {
  return ssid.equalsIgnoreCase(stickname);
}


String simpleHttpRequest(String url) {  
  HTTPClient http;
  Serial.println("httpRequest: " + url);
  //http.setTimeout(3000);
  http.begin(url);
  String payload="";
  int httpCode = http.GET();  
  if (httpCode > 0) {
      payload = http.getString();
      Serial.println(xx + "Got httpCode: " + httpCode);
      //Serial.println(xx + "Got payload: " + payload);
  }
  else {
    //httpCode is negative -> guess the request failed :P
    Serial.println("Error on HTTP request. httpCode=" + httpCode);
  }
  //stop the connection to free resources
  http.end();
  return payload;
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


void saveBroadcastListenerIPLocally(String partnerIp, String partnerStickname) {
  Serial.println("Trying to save IP " + partnerIp + " to local broadcastListeners!");
  int firstAvailableIPslot=-1;
  for (int i=0; i < MAX_BROADCAST_LISTENERS; i++) {
    String ip = broadcastListenerIPs[i];    
    if (ip.length() == 0) {         // found an empty slot
      firstAvailableIPslot = i;
      break;    
    }
    else if (ip.equals(partnerIp)) {  // -> the new partner's ip is already registered. Do not register again.
      firstAvailableIPslot=-2;
      break;  
    }
  }

  if (firstAvailableIPslot >= 0) {
    broadcastListenerIPs[firstAvailableIPslot] = partnerIp;
    Serial.println(" Saved IP " + partnerIp + " to local broadcastListeners on index=" + firstAvailableIPslot);
  }
  else {
    if (firstAvailableIPslot == -1)
      Serial.println(" Could not save IP " + partnerIp + " to the local broadcastListeners since there was no free slot!");
    else
      Serial.println(" Could not save IP " + partnerIp + " to the local broadcastListeners since it was registered before already!");
  }
}

void initGyroStaff() {
  M5.MPU6886.Init();
  M5.MPU6886.SetGyroFsr(M5.MPU6886.GFS_500DPS);
  M5.MPU6886.SetAccelFsr(M5.MPU6886.AFS_4G);
  pinMode(M5_BUTTON_HOME, INPUT);  
  delay(100); // Wait for sensor to stabilize
  timer = micros();
}


void drawDrink(double angle, String sourceStickname) {
  double dispAngle = (360-(int)angle) % 360;
  double dispAngleYrad = getRad(dispAngle);
  double radius = 40/sin(getRad(90-angle));     // calc the actual radius from the center of the screen to the screen's edge. 40=screenwidth/2;
  xDiff = cos(dispAngleYrad) * radius;
  yDiff = sin(dispAngleYrad) * radius;
  clearScreen(BLACK);

  M5.Lcd.setCursor(3, 3);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print("Prost von");
  M5.Lcd.setCursor(5, 40);
  M5.Lcd.setTextSize(4);
  M5.Lcd.print(sourceStickname);  
    
  // Point on the right
  int xP1 = xStart + xDiff;
  int yP1 = yStart + yDiff; 
  //M5.Lcd.drawCircle(xP1, yP1, 6, GREEN);

  // Point on the left
  int xP2 = xStart - xDiff ;
  int yP2 = yStart - yDiff;
  //M5.Lcd.drawCircle(xP2, yP2, 6, BLUE);

  int xP3 = angle>0?xP1:xP2;
  int yP3 = max(yP1, yP2);
  //M5.Lcd.drawCircle(xP3, yP3, 6, RED);

  // Foam
  M5.Lcd.drawLine(xP1, yP1, xP2, yP2, WHITE);
  M5.Lcd.drawLine(xP1, yP1-1, xP2, yP2-1, WHITE);
  M5.Lcd.drawLine(xP1, yP1-2, xP2, yP2-2, WHITE);
  M5.Lcd.drawLine(xP1, yP1-3, xP2, yP2-3, WHITE);
  M5.Lcd.drawLine(xP1, yP1-4, xP2, yP2-4, WHITE);

  // Yellow drink
  M5.Lcd.fillTriangle(xP1, yP1,
                      xP2, yP2,
                      xP3, yP3,
                      YELLOW);                    
  M5.Lcd.fillRect(0, yP3,
                  80, 160,
                  YELLOW);
}

void executeKalmanFilterOnAngles (){
  #ifdef RESTRICT_PITCH // Eq. 25 and 26
    double roll  = atan2(accY, accZ) * RAD_TO_DEG;
    double pitch = atan(-accX / sqrt(accY * accY + accZ * accZ)) * RAD_TO_DEG;
  #else // Eq. 28 and 29
    double roll  = atan(accY / sqrt(accX * accX + accZ * accZ)) * RAD_TO_DEG;
    double pitch = atan2(-accX, accZ) * RAD_TO_DEG;
  #endif
  
  double gyroXrate = gyroX / 131.0; // Convert to deg/s
  double gyroYrate = gyroY / 131.0; // Convert to deg/s
  double gyroZrate = gyroZ / 131.0; // Convert to deg/s
  
  #ifdef RESTRICT_PITCH // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
    if ((roll < -90 && kalAngleX > 90) || (roll > 90 && kalAngleX < -90)) {
      kalmanX.setAngle(roll);
      compAngleX = roll;
      kalAngleX = roll;
      gyroXangle = roll;
    } else
      kalAngleX = kalmanX.getAngle(roll, gyroXrate, dt); // Calculate the angle using a Kalman filter
        
    if (abs(kalAngleX) > 90)
      gyroYrate = -gyroYrate; // Invert rate, so it fits the restriced accelerometer reading
        
    kalAngleY = kalmanY.getAngle(pitch, gyroYrate, dt);
  #else
    // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
    if ((pitch < -90 && kalAngleY > 90) || (pitch > 90 && kalAngleY < -90)) {
      kalmanY.setAngle(pitch);
      compAngleY = pitch;
      kalAngleY = pitch;
      gyroYangle = pitch;
    } else
      kalAngleY = kalmanY.getAngle(pitch, gyroYrate, dt); // Calculate the angle using a Kalman filter
        
    if (abs(kalAngleY) > 90) 
      gyroXrate = -gyroXrate; // Invert rate, so it fits the restriced accelerometer reading
    
    kalAngleX = kalmanX.getAngle(roll, gyroXrate, dt); // Calculate the angle using a Kalman filter
  #endif
  
  compAngleX = 0.93 * (compAngleX + gyroXrate * dt) + 0.07 * roll; // Calculate the angle using a Complimentary filter
  compAngleY = 0.93 * (compAngleY + gyroYrate * dt) + 0.07 * pitch;
  
  // Reset the gyro angle when it has drifted too much
  if (gyroXangle < -180 || gyroXangle > 180) 
    gyroXangle = kalAngleX;
  
  if (gyroYangle < -180 || gyroYangle > 180)
    gyroYangle = kalAngleY;
}

double getRad(double degrees) {
  return degrees * radConversionFactor;
}

void setBrightness(int brightnessPercent) {
  // Method 1
  if (brightnessPercent == 0)
    M5.begin(0,1,1);  // disable LCD      
  else 
    M5.begin();

  // Method 2
  M5.Axp.SetLDO2(brightnessPercent!=0);
  M5.Axp.SetLDO3(brightnessPercent!=0);

  // Method 3
  float breath = 7 + ((float)(8*brightnessPercent))/100;
  M5.Axp.ScreenBreath(breath);         // takes a value from 7 to 15
}


void vibrate(bool enable) {  
  analogWrite(ANALOG_OUTPIN, enable?255:0);
  vibrationEnabled = enable;
}
