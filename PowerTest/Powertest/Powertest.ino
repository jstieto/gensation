#include <M5StickC.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Black";
const char* password =  "6f2159d0ca1d";
const String hostURL = "http://v22017125277557049.nicesrv.de:5000/save?username=johannes20200722_b&counter=";
int counter = 0;

void setup() {

  Serial.begin(115200);
  delay(4000);

  //make sure to add M5.begin since otherwise the M5 commands won't work -cough-
  M5.begin();
  delay(10);
  startWifiConnection();
}

void startWifiConnection() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    //keep trying every 4 secs
    delay(4000);
    Serial.println("Connecting to Wifi Network");
  }

  Serial.println("Connected to the WiFi network");
  M5.Lcd.fillScreen(BLUE);  
  M5.Lcd.setCursor(0,0);
}

void loop() {

  M5.update();
  delay(10000);
  sendButtonPressed();
  
  //if (M5.BtnA.isPressed()) {
    //sendButtonPressed();
  //}
  if (M5.BtnB.isPressed()) {
    M5.Lcd.fillScreen(BLUE);
    M5.Lcd.setCursor(0,0);
  }
}

void sendButtonPressed() {
  //check whether the connection is still alive
  if ((WiFi.status() == WL_CONNECTED)) {

    HTTPClient http;
    counter++;

    //send data to echo service and add current counter value
    http.begin(hostURL + counter);

    //send httpRequest and check response code
    int httpCode = http.GET();

    if (httpCode > 0) {

        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);

        //also show the contents on the screen
        M5.Lcd.print(payload);
      }
    else {
      //httpCode is negative -> guess the request failed :P
      Serial.println("Error on HTTP request");
      counter--;
    }
     //stop the connection to free resources
    http.end();
  } else {
    //if WifiStatus has changed, try to reconnect to WiFi
    startWifiConnection();
  }
}
