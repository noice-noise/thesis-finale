
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

ESP8266WebServer server;
WebSocketsServer webSocket = WebSocketsServer(81);

const char *ssid = "ZTE_2.4G_dtgg7C";
const char *password = "hSLdzYah";

#define toggle_led 4
uint8_t pin_led = 16;

int brightness;

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
        <button class="btn w-full" id="indicatorLed">Toggle LED</button>
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
      const indicatorLed = document.getElementById('indicatorLed');
      const txInput = document.getElementById('txInput');
      const rxConsole = document.getElementById('rxConsole');
      const dataForm = document.getElementById('dataForm');

      var Socket;

      const init = () => {
        Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
        Socket.onmessage = transmitText();
      };

      const toggleSwitch = () => {
        Socket.send('$');
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

      indicatorLed.addEventListener('click', toggleSwitch);
      txInput.addEventListener('keydown', transmitText);
      dataForm.addEventListener('submit', onFormSubmit);
    </script>
  </body>
</html>
)=====";

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
  if (type == WStype_TEXT)
  {
    if (payload[0] == '#')
    {
      uint16_t brightness = (uint16_t)strtol((const char *)&payload[1], NULL, 10);
      brightness = 1024 - brightness;
      analogWrite(pin_led, brightness);
      Serial.print("brightness= ");
      Serial.println(brightness);
    }
    else if (payload[0] == '$')
    {
      if (digitalRead(toggle_led) == HIGH)
      {
        Serial.println("toggled LED to LOW");
        digitalWrite(toggle_led, LOW);
      }
      else
      {
        Serial.println("toggled LED to HIGH");
        digitalWrite(toggle_led, HIGH);
      }
    }
    else
    {
      for (int i = 0; i < (int)length; i++)
      {
        Serial.print("Data: ");
        Serial.print((char)payload[i]);
      }
      Serial.println();

      parseJsonData(payload);
    }
  }
}

void toggleLED()
{
  digitalWrite(toggle_led, !digitalRead(toggle_led));
  server.send(204, "");
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

  pinMode(toggle_led, OUTPUT);

  server.on("/", []()
            { server.send_P(200, "text/html", webpage); });

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  webSocket.loop();
  server.handleClient();
  if (Serial.available() > 0)
  {
    char c[] = {(char)Serial.read()};
    webSocket.broadcastTXT(c, sizeof(c));
  }
}