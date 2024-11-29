#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include <Adafruit_FT6206.h>
#include <IRremote.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
// Defining the WiFi channel speeds up the connection:
#define WIFI_CHANNEL 6

#define PIN_RECEIVER 1 // Signal Pin of IR receiver

IRrecv receiver(PIN_RECEIVER);

WebServer server(80);

const int LED1 = 17;
const int LED2 = 18;

bool led1State = false;
bool led2State = false;

Adafruit_FT6206 ctp = Adafruit_FT6206();

#define TFT_DC 2
#define TFT_CS 15
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define BOXSIZE 40
#define PENRADIUS 3
int oldcolor, currentcolor;

Adafruit_GFX_Button webServerButton;
Adafruit_GFX_Button irReceiverButton;
bool isWebServerRunning = false;
bool isIRReceiverRunning = false;
int lastIRCodeY = 10;

void sendHtml()
{
  String response = R"(
    <!DOCTYPE html><html>
      <head>
        <title>ESP32 Web Server Demo</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: sans-serif; text-align: center; }
          body { display: inline-flex; flex-direction: column; }
          h1 { margin-bottom: 1.2em; } 
          h2 { margin: 0; }
          div { display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: auto auto; grid-auto-flow: column; grid-gap: 1em; }
          .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
          .btn.OFF { background-color: #333; }
        </style>
      </head>
            
      <body>
        <h1>ESP32 Web Server</h1>

        <div>
          <h2>LED 1</h2>
          <a href="/toggle/1" class="btn LED1_TEXT">LED1_TEXT</a>
          <h2>LED 2</h2>
          <a href="/toggle/2" class="btn LED2_TEXT">LED2_TEXT</a>
        </div>
      </body>
    </html>
  )";
  response.replace("LED1_TEXT", led1State ? "ON" : "OFF");
  response.replace("LED2_TEXT", led2State ? "ON" : "OFF");
  server.send(200, "text/html", response);
}

void setup(void)
{
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  startLCDGUI();
}

void startHTTPServer()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", sendHtml);

  server.on(UriBraces("/toggle/{}"), []()
            {
    String led = server.pathArg(0);
    Serial.print("Toggle LED #");
    Serial.println(led);

    switch (led.toInt()) {
      case 1:
        led1State = !led1State;
        digitalWrite(LED1, led1State);
        break;
      case 2:
        led2State = !led2State;
        digitalWrite(LED2, led2State);
        break;
    }

    sendHtml(); });

  server.begin();
  isWebServerRunning = true;
  Serial.println("HTTP server started (http://localhost:8180)");
}

void handleIR()
{
  if (lastIRCodeY > 220)
  {
    tft.fillRect(200, 0, 120, 240, ILI9341_BLACK);
    lastIRCodeY = 10;
  }
  tft.setCursor(200, lastIRCodeY);
  lastIRCodeY += 20;
  // Takes command based on IR code received
  switch (receiver.decodedIRData.command)
  {
  case 162:
    tft.println("POWER");
    break;
  case 226:
    tft.println("MENU");
    break;
  case 34:
    tft.println("TEST");
    break;
  case 2:
    tft.println("PLUS");
    break;
  case 194:
    tft.println("BACK");
    break;
  case 224:
    tft.println("PREV.");
    break;
  case 168:
    tft.println("PLAY");
    break;
  case 144:
    tft.println("NEXT");
    break;
  case 104:
    tft.println("num: 0");
    break;
  case 152:
    tft.println("MINUS");
    break;
  case 176:
    tft.println("key: C");
    break;
  case 48:
    tft.println("num: 1");
    break;
  case 24:
    tft.println("num: 2");
    break;
  case 122:
    tft.println("num: 3");
    break;
  case 16:
    tft.println("num: 4");
    break;
  case 56:
    tft.println("num: 5");
    break;
  case 90:
    tft.println("num: 6");
    break;
  case 66:
    tft.println("num: 7");
    break;
  case 74:
    tft.println("num: 8");
    break;
  case 82:
    tft.println("num: 9");
    break;
  default:
    tft.println("Unknown");
  }
}

void startLCDGUI()
{
  Wire.setPins(10, 8); // redefine first I2C port to be on pins 10/8
  tft.begin();
  tft.setRotation(1);

  if (!ctp.begin(40))
  { // pass in 'sensitivity' coefficient
    Serial.println("Couldn't start FT6206 touchscreen controller");
    while (1)
      ;
  }

  Serial.println("Capacitive touchscreen started");

  tft.fillScreen(ILI9341_BLACK);

  tft.setTextSize(2);
  tft.setCursor(10, 25);
  tft.println("Web Server");

  webServerButton.initButton(&tft, 60, 80, 100, 50, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "Start", 2);
  webServerButton.drawButton(false);

  tft.setCursor(10, 145);
  tft.println("IR Receiver");
  irReceiverButton.initButton(&tft, 60, 200, 100, 50, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "Start", 2);
  irReceiverButton.drawButton(false);
}

bool httpLastPressed = false; // Tracks the previous button state
bool irLastPressed = false;   // Tracks the previous button state

void loop(void)
{
  server.handleClient();
  delay(10);
  if (ctp.touched())
  {
    TS_Point p = ctp.getPoint();

    int16_t x = p.y;
    int16_t y = 240 - p.x;

    if (webServerButton.contains(x, y))
    {
      if (!httpLastPressed) // Trigger only on the first press
      {
        httpLastPressed = true; // Mark as pressed
        if (!isWebServerRunning)
        {
          webServerButton.initButton(&tft, 60, 80, 100, 50, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "Stop", 2);
          webServerButton.drawButton(true); // Highlight button
          startHTTPServer();                // Trigger action for Button Web Server
        }
        else
        {
          webServerButton.initButton(&tft, 60, 80, 100, 50, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "Start", 2);
          webServerButton.drawButton(false); // Redraw in normal state
          server.stop();
          Serial.println("HTTP server stopped");
          isWebServerRunning = false;
        }
      }
    }
    else if (irReceiverButton.contains(x, y))
    {
      if (!irLastPressed) // Trigger only on the first press
      {
        irLastPressed = true; // Mark as pressed
        if (!isIRReceiverRunning)
        {
          irReceiverButton.initButton(&tft, 60, 200, 100, 50, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "Stop", 2);
          irReceiverButton.drawButton(true);
          receiver.enableIRIn(); // Start the receiver
          isIRReceiverRunning = true;
          Serial.println("IR receiver started");
        }
        else
        {
          irReceiverButton.initButton(&tft, 60, 200, 100, 50, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "Start", 2);
          irReceiverButton.drawButton(false); // Redraw in normal state
          receiver.disableIRIn();             // Stop the receiver
          isIRReceiverRunning = false;
          Serial.println("IR receiver stopped");
        }
      }
    }
  }
  else
  {
    httpLastPressed = false;
    irLastPressed = false;
  }

  if (receiver.decode())
  {
    handleIR();
    receiver.resume(); // Receive the next value
  }
}
