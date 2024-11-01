#include <ESP32Servo.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>

// Drive Train Servos
const int dtServoRPin = 32; // Digital pin of the Servo
const int dtCap = 360;
Servo dtServoR;  // dtServo instance
const int dtServoLPin = 33;
Servo dtServoL;
const int dtNum = 2;
Servo* dtServos[] = {&dtServoR, &dtServoL};

// Belt Drive Servo
const int bdServoRPin = 12; // Digital pin of the Servo
const int bdCap = 360;
Servo bdServoR;  // bdServo instance
const int bdServoLPin = 14;
Servo bdServoL;
const int bdNum = 2;
Servo* bdServos[] = {&bdServoR, &bdServoL};

//Belt Motor Values
const float beltSpeed = 25;
const int beltDelay = 1500;
//Drivetrain Values
const float moveSpeed = 15;
//fw/bw
const int unitDelay = 500;
//rt/lt
const int turnDelay = 250;

//Network Values
String path = "http://192.168.1.106:1880/get/";
const char* ssid = "utexas-iot";
const char* pass = "17981954548150055250";


void setup()
{
  Serial.begin(115200);

  dtServoR.attach(dtServoRPin);
  dtServoL.attach(dtServoLPin);
  bdServoR.attach(bdServoRPin);
  bdServoL.attach(bdServoLPin);

  delay(1000);
  fw(10);
  movePlate(false);
  bw(10);

  delay(5000);

  fw(10);
  movePlate(true);
  bw(10);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  WiFi.begin("utexas-iot", "17981954548150055250");
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print('.');
    delay(1000);
  }
}

void loop()
{
  if(get("pickup"))
  {
    fw(10);
    rt();
    fw(10);
    delay(3000);
    movePlate(false);
    bw(10);
    lt();
    bw(10);
  }

  if(get("deliver"))
  {
    fw(10);
    rt();
    fw(10);
    movePlate(true);
    delay(3000);
    bw(10);
    lt();
    bw(10);
  }
}

bool get(String request)
{
  HTTPClient http;

  String serverPath = path + "request";
  
  // Your Domain name with URL path or IP address with path
  http.begin(serverPath.c_str());
  
  // If you need Node-RED/server authentication, insert user and password below
  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
  
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  String payload = "";
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    payload.toLowerCase();
    Serial.println(payload);
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  if(payload.indexOf("true") > -1)
  {
    return true;
  }
  return false;
}

void readMacAddress()
{
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) 
  {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]);
  } 
  else
  {
    Serial.println("Failed to read MAC address");
  }
}

void movePlate(bool backward)
{
  float speed = beltSpeed;
  if(backward)
  {
    speed = speed * -1;
  }

  move(&bdServoR, -speed);
  move(&bdServoL, speed);
  stop(bdServos, bdNum, beltDelay);
}

void fw(int units)
{
  move(&dtServoR, -moveSpeed); //Motors flipped and other wonky bs
  move(&dtServoL, moveSpeed);
  stop(dtServos, dtNum, unitDelay * units);
}

void bw(int units)
{
  move(&dtServoR, moveSpeed);
  move(&dtServoL, -moveSpeed);
  stop(dtServos, dtNum, unitDelay * units);
}

void rt()
{
  move(&dtServoR, moveSpeed);
  move(&dtServoL, moveSpeed);
  stop(dtServos, dtNum, turnDelay);
}

void lt()
{
  move(&dtServoR, -moveSpeed);
  move(&dtServoL, -moveSpeed);
  stop(dtServos, dtNum, turnDelay);
}

/** Moves the given motor at the given speed. The motors are moved syncronously.
* @param servo The servos to move as a list
* @param speed The speed to move by, (-90, 90). Negative value is reverse.
*/
void move(Servo* servo, float speed)
{
  speed = cappedSpeed(speed);
  servo->write(speed);
}

void stop(Servo* servos[], int numServos, int time)
{
  delay(time);
  for(int i = 0; i < numServos; i++)
  {
    servos[i]->write(90);
  }
}

/** Takes a speed value and converts it into usable write speed for the servo
* @param speed The given speed value to convert
*/
float cappedSpeed(float speed)
{
  if(speed < 0)
  {
    if(speed < -90)
    {
      speed = -90; // caps the maximum reverse speed
    }
    speed = speed + 90;
  }
  else
  {
    speed = speed + 90;
    if(speed > 180)
    {
      speed = 180; // caps the maximum forward speed
    }
  }

  return speed;
}
