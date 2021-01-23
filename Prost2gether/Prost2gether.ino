// http://www.dieletztedomain.de/wasserwaage-mit-dem-mpu6886-des-m5stickc/

#include <M5StickC.h>
#include <Kalman.h>

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

double gyroXangle, gyroYangle; // Angle calculate using the gyro only
double compAngleX, compAngleY; // Calculated angle using a complementary filter
double kalAngleX, kalAngleY; // Calculated angle using a Kalman filter

const double radConversionFactor = 3.141592 / 180;
int xStart = 40;
int yStart = 80;
int xDiff = 0;
int yDiff = 0;
int prevDegrees=-1;

void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.MPU6886.Init();
  M5.MPU6886.SetGyroFsr(M5.MPU6886.GFS_500DPS);
  M5.MPU6886.SetAccelFsr(M5.MPU6886.AFS_4G);
  pinMode(M5_BUTTON_HOME, INPUT);

  delay(100); // Wait for sensor to stabilize
  timer = micros();
}

void loop() {
  dt = (double)(micros() - timer) / 1000000; // Calculate delta time
  timer = micros();
  
  // put your main code here, to run repeatedly:
  M5.MPU6886.getGyroData(&gyroX, &gyroY, &gyroZ);
  M5.MPU6886.getAccelData(&accX, &accY, &accZ);

  executeKalmanFilterOnAngles();
  //serialPrintDataAngleData();
  
  if (((int) kalAngleY) != prevDegrees) {        
    drawDrink(kalAngleY);
        
    //M5.Lcd.setCursor(10, 10);
    //M5.Lcd.printf("%3.0f", kalAngleY);  
    prevDegrees = kalAngleY;
  }
  
  //delay(50);
  delay(100); // this might save battery and reduces flickering
}

void drawDrink(double angle) {
  double dispAngle = (360-(int)angle) % 360;
  double dispAngleYrad = getRad(dispAngle);
  double radius = 40/sin(getRad(90-angle));     // calc the actual radius from the center of the screen to the screen's edge. 40=screenwidth/2;
  xDiff = cos(dispAngleYrad) * radius;
  yDiff = sin(dispAngleYrad) * radius;
  M5.Lcd.fillScreen(BLACK);    

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


void serialPrintDataAngleData()
{
  /*
  Serial.print(gyroXangle);
  Serial.print(",");
  Serial.print(gyroYangle);
  Serial.print(",");
  Serial.print(compAngleX);
  Serial.print(",");
  Serial.print(compAngleY);
  Serial.print(",");
  */
  Serial.print(kalAngleX);
  Serial.print(",");
  Serial.print(kalAngleY);
  Serial.println("");
}

double getRad(double degrees) {
  return degrees * radConversionFactor;
}
