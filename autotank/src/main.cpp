
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

ESP8266WebServer server;
WebSocketsServer webSocket = WebSocketsServer(81);

// const char *ssid = "ZTE_2.4G_dtgg7C";
// const char *password = "hSLdzYah";

// const char *ssid = "AutoTankMaster";
// const char *password = "tank_admin_01";

const char *ssid = "DelCorro";
const char *password = "Mind_Valley21";

// #define resetPin D0
#define trig D1
#define echo D2

#define pump D3
#define valve D4

// #define trig D5
// #define echo D6

#define ledRed D6
#define ledGreen D7
#define ledBlue D8

const char AUTO_MODE = 'A';
const char CONTROL_MODE = 'C';
const char SLEEP_MODE = 'S';

int leds[] = {ledRed, ledGreen, ledBlue};

char tankMode = AUTO_MODE;

int distance;
int duration;

char webpage[] PROGMEM = R"=====(
<html>
  <head>
    <style>
      .header {
        text-align: center;
      }

      .main {
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: stretch;
        width: 100%;
        max-width: 500px;
        gap: 3rem;
        margin: 0 auto;
      }

      .form {
        width: 100%;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: stretch;
        gap: 1rem;
      }

      .form-label {
        display: block;
        width: 100%;
      }

      .form-input {
        padding: 0.5rem 0.5rem;
        width: 100%;
      }

      .btn {
        padding: 0.5rem 3rem;
      }

      .w-full {
        width: 100%;
      }

      #rxConsole {
        height: 10em;
        width: 100%;
      }
    </style>
  </head>
  <body onload="javascript:init()">
    <header class="header">
      <div>
        <h1>Auto Tank Control Panel</h1>
      </div>
    </header>
    <main class="main">
      <div>
        <label for="rxConsole">(Rx) Console [READONLY]</label>
        <textarea id="rxConsole" readonly></textarea>
      </div>
      <div>
        <label class="form-label" for="txInput">Transmit (Tx) Bar</label>
        <input class="form-input" type="text" id="txInput" />
      </div>
      <div>
        <select class="btn w-full" name="mode" id="mode">
          <option value="A">Auto Mode</option>
          <option value="C">Control Mode</option>
          <option value="S">Sleep Mode</option>
        </select>
        <button class="btn w-full" id="pumpButton">Toggle Pump</button>
        <button class="btn w-full" id="valveButton">Toggle Valve</button>
      </div>
      <form class="form" id="dataForm">
        <div class="form-control">
          <label class="form-label" for="led">Led</label>
          <input class="form-input" name="led" id="led" />
        </div>
        <div>
          <label class="form-label" for="command">Command</label>
          <input class="form-input" name="command" id="command" />
        </div>
        <button class="btn" type="submit">Send</button>
      </form>
    </main>

    <script>
      const pumpButton = document.getElementById('pumpButton');
      const valveButton = document.getElementById('valveButton');
      const mode = document.getElementById('mode');
      const txInput = document.getElementById('txInput');
      const rxConsole = document.getElementById('rxConsole');
      const dataForm = document.getElementById('dataForm');

      var Socket;

      const init = () => {
        Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
        Socket.onmessage = transmitText;
      };

      const togglePump = () => {
        Socket.send('3');
      };

      const toggleValve = () => {
        Socket.send('4');
      };

      const transmitText = (event) => {
        if (event.keyCode == 13) {
          const data = txInput.value;
          Socket.send(data);
          txInput.value = '';
        }
      };

      const onSocketMessage = (event) => {
        rxConsole.value += event.data;
      };

      const onFormSubmit = (event) => {
        event.preventDefault();

        const formData = new FormData(event.target);
        const jsonData = JSON.stringify(Object.fromEntries(formData));

        Socket.send(jsonData);
      };

      const onModeSelect = (event) => {
        Socket.send(mode.value);
      };

      pumpButton.addEventListener('click', togglePump);
      valveButton.addEventListener('click', toggleValve);
      txInput.addEventListener('keydown', transmitText);
      dataForm.addEventListener('submit', onFormSubmit);
      mode.addEventListener('change', onModeSelect);
    </script>
  </body>
</html>
)=====";

void showLed(int ledToOn)
{
  for (int i = 0; i < 3; i++)
  {
    if (leds[i] == ledToOn)
    {
      Serial.print("Showing LED...");
      Serial.println(ledToOn);
      digitalWrite(leds[i], HIGH);
    }
    else
    {
      digitalWrite(leds[i], LOW);
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

void parseJsonData(uint8_t *payload)
{

  DynamicJsonDocument doc(200);
  DeserializationError error = deserializeJson(doc, payload);

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  String led = doc["led"];
  String command = doc["command"];

  Serial.println(led);
  Serial.println(command);

  server.send(204, "");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type != WStype_TEXT)
  {
    return;
  }

  char actionCode = payload[0];

  Serial.print("Action Code: ");
  Serial.println(actionCode);

  if (actionCode == AUTO_MODE || actionCode == CONTROL_MODE || actionCode == SLEEP_MODE)
  {
    tankMode = actionCode;
    return;
  }

  if (tankMode == CONTROL_MODE)
  {
    if (actionCode == '3')
    {
      togglePump();
      return;
    }

    if (actionCode == '4')
    {
      toggleValve();
      return;
    }
  }

  Serial.print("Data: ");
  for (int i = 0; i < (int)length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  parseJsonData(payload);
}

void setPump(int status)
{
  digitalWrite(pump, status);
}

void setValve(int status)
{
  digitalWrite(valve, status);
}

void handleAutoMode()
{
  if (distance < 100)
  {
    showLed(ledRed);
    setPump(HIGH);
    setValve(LOW);
  }
  else if (distance >= 100 && distance <= 200)
  {
    showLed(ledGreen);
  }
  else
  {
    setPump(LOW);
    showLed(ledBlue);
    setValve(HIGH);
  }
}

void ultrasonic()
{
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  duration = pulseIn(echo, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.println(distance);

  Serial.print("Mode: ");
  Serial.println(tankMode);

  if (tankMode == AUTO_MODE)
  {
    handleAutoMode();
  }

  delay(500);
}

void setup()
{
  WiFi.begin(ssid, password);
  Serial.begin(115200);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(pump, OUTPUT);
  pinMode(valve, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);

  server.on("/", []()
            { server.send_P(200, "text/html", webpage); });
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  ultrasonic();

  webSocket.loop();
  server.handleClient();

  if (Serial.available() > 0)
  {
    char c[] = {(char)Serial.read()};
    webSocket.broadcastTXT(c, sizeof(c));
  }
}
