
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>
#include <config.h>

#define USE_SERIAL Serial

#define trig D1
#define echo D2
#define pump D3
#define valve D4
#define ledRed D6
#define ledGreen D7
#define ledBlue D8

ESP8266WebServer server;
WebSocketsServer webSocket = WebSocketsServer(81);

const char *ssid = DEVICE_WIFI_SSID;
const char *password = DEVICE_WIFI_PASS;

const char *indexFile = "index.html";

const char AUTO_MODE = 'A';
const char CONTROL_MODE = 'C';
const char SLEEP_MODE = 'S';

const int LOW_LEVEL = 1;
const int NEUTRAL_LEVEL = 2;
const int HIGH_LEVEL = 3;

int rgbLeds[] = {ledRed, ledGreen, ledBlue};

const int SENSOR_HEIGHT_OFFSET = 5; // MUST NOT BE ZERO or else water will reach device
int tankHeight = 50;
int lowLevelThreshold = 50;  // high value means how far water is from device
int highLevelThreshold = 20; // lower value means how close water is from device

String state;

char currentMode = CONTROL_MODE;

int sensorDistance; // current sensorDistance value read by ultrasonic sensor
int flightDuration; // flightDuration variable used by ultrasonic sensor

char activeLedColor; // current active LED color
int currentLevel;    // current distance level

// =================== COMPONENT FUNCTIONS =======================

void showLed(int ledToOn)
{
  for (uint8_t i = 0; i < 3; i++)
  {
    if (rgbLeds[i] == ledToOn)
    {
      digitalWrite(rgbLeds[i], HIGH);
    }
    else
    {
      digitalWrite(rgbLeds[i], LOW);
    }
  }
}

void togglePump()
{
  digitalWrite(pump, !digitalRead(pump));
}

void toggleValve()
{
  digitalWrite(valve, !digitalRead(valve));
}

void setPump(int status)
{
  digitalWrite(pump, status);
}

void setValve(int status)
{
  digitalWrite(valve, status);
}

void handleLevel()
{

  if (sensorDistance <= lowLevelThreshold && sensorDistance >= highLevelThreshold)
  {
    currentLevel = NEUTRAL_LEVEL;
  }
  else if (sensorDistance < lowLevelThreshold)
  {
    currentLevel = HIGH_LEVEL;
  }

  else
  {
    currentLevel = LOW_LEVEL;
  }
}

void handleLed()
{
  if (currentLevel == HIGH_LEVEL)
  {
    showLed(ledBlue);
    activeLedColor = 'B';
  }
  else if (currentLevel == NEUTRAL_LEVEL)
  {
    showLed(ledGreen);
    activeLedColor = 'G';
  }
  else
  {
    showLed(ledRed);
    activeLedColor = 'R';
  }
}

void handleAutoMode()
{
  if (currentLevel == LOW_LEVEL)
  {
    setPump(HIGH);
    setValve(LOW);
  }
  else if (currentLevel == HIGH_LEVEL)
  {
    setPump(LOW);
    setValve(HIGH);
  }
}

void handleSensor()
{
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  flightDuration = pulseIn(echo, HIGH);
  sensorDistance = flightDuration * 0.034 / 2;

  delay(500);
}

void handleModes()
{
  if (currentMode == AUTO_MODE)
  {
    handleAutoMode();
  }
}

// =================== SERVER AND WEB SOCKET =======================

void webSocketBroadcast(String str)
{
  char *tab2 = new char[str.length() + 1];
  strcpy(tab2, str.c_str());
  webSocket.broadcastTXT(tab2, str.length());
}

void broadcastSystemState()
{
  StaticJsonDocument<200> doc;

  state = ""; // reset state string info

  doc["sensorDistance"] = sensorDistance;
  doc["activeLedColor"] = (char)activeLedColor;
  doc["pumpState"] = digitalRead(pump);
  doc["valveState"] = digitalRead(valve);
  doc["currentMode"] = (char)currentMode;
  doc["currentLevel"] = currentLevel;
  doc["tankHeight"] = tankHeight;
  doc["lowLevelThreshold"] = lowLevelThreshold;
  doc["highLevelThreshold"] = highLevelThreshold;

  // uncomment to monitor system state
  // serializeJson(doc, USE_SERIAL);
  // serializeJsonPretty(doc, USE_SERIAL);

  serializeJsonPretty(doc, state);
}

void parseJsonData(uint8_t *payload)
{
  DynamicJsonDocument doc(200);
  DeserializationError error = deserializeJson(doc, payload);

  if (error)
  {
    USE_SERIAL.print(F("deserializeJson() failed: "));
    USE_SERIAL.println(error.f_str());
    return;
  }

  int userTankHeight = (int)doc["tankHeight"];
  int highLevelValue = (int)doc["highLevelValue"];
  int lowLevelValue = (int)doc["lowLevelValue"];

  if (userTankHeight <= highLevelValue || userTankHeight <= (highLevelValue + SENSOR_HEIGHT_OFFSET))
  {
    USE_SERIAL.println("Invalid highLevelValue");
    server.send(500, "");
    return;
  }

  if (lowLevelValue >= highLevelValue || highLevelValue <= lowLevelValue)
  {
    USE_SERIAL.println("Invalid user values");
    server.send(500, "");
    return;
  }

  tankHeight = userTankHeight;
  lowLevelThreshold = userTankHeight - lowLevelValue;
  highLevelThreshold = userTankHeight - highLevelValue;
  server.send(204, "");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_CONNECTED:
    USE_SERIAL.println("WebSocket Connected");
    webSocket.sendTXT(num, "Connected");
    break;
  case WStype_DISCONNECTED:
    USE_SERIAL.println("WebSocket Disconnected");
    break;
  case WStype_ERROR:
    USE_SERIAL.println("WebSocket Disconnected");
    break;
  case WStype_PING:
    USE_SERIAL.printf("WebSocket Ping\n");
    break;
  case WStype_PONG:
    USE_SERIAL.printf("WebSocket Pong\n");
    broadcastSystemState();
    break;
  case WStype_TEXT:
    USE_SERIAL.printf("WebSocket Message: %s\n", payload);
    char actionCode = payload[0];

    USE_SERIAL.print("Action Code: ");
    USE_SERIAL.println(actionCode);

    if (actionCode == AUTO_MODE || actionCode == CONTROL_MODE || actionCode == SLEEP_MODE)
    {
      currentMode = actionCode;
    }

    if (currentMode == CONTROL_MODE)
    {
      if (actionCode == '3')
      {
        togglePump();
      }
      else if (actionCode == '4')
      {
        toggleValve();
      }
    }

    parseJsonData(payload);
    break;
  }
}

// =================== SETUP & LOOP =======================

void setup()
{
  WiFi.begin(ssid, password);
  USE_SERIAL.begin(115200);

  for (uint8_t t = 4; t > 0; t--)
  {
    USE_SERIAL.printf("[SETUP] BOOTING %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  USE_SERIAL.println("AUTOTANK v0.1");

  while (WiFi.status() != WL_CONNECTED)
  {
    USE_SERIAL.print(".");
    delay(500);
  }

  USE_SERIAL.printf("\nConnected successfully to %s\n", DEVICE_WIFI_SSID);
  USE_SERIAL.print("IP Address: ");
  USE_SERIAL.println(WiFi.localIP());

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(pump, OUTPUT);
  pinMode(valve, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);

  server.serveStatic("/", LittleFS, "/");

  // specific file streaming
  server.on("/stream-index", HTTPMethod::HTTP_GET, []()
            {
    LittleFS.begin();

    File file = LittleFS.open("index.html", "r");

    if (!file) {
      USE_SERIAL.println("Could not open file for reading...");
      server.send(500, "application/json",
                  "{\"error\":\"could not open file\"}");
    } else {
      server.streamFile<File>(file, "text/html");
      file.close();
    }

    LittleFS.end(); });

  server.begin();
  LittleFS.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  handleSensor();
  handleLevel();
  handleLed();
  handleModes();

  webSocket.loop();
  server.handleClient();

  if (USE_SERIAL.available() > 0)
  {
    char c[] = {(char)USE_SERIAL.read()};
    webSocket.broadcastTXT(c, sizeof(c));
  }

  broadcastSystemState();
  webSocket.broadcastTXT(state);
  delay(50);
}
