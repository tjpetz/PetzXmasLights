/**
 * @file PetzXmasLights.cpp
 * @author Thomas J. Petz, Jr. (tom@tjpetz.com)
 * @brief Christmas tree light effects for WS1218B LEDS using FastLED with BLE configuration.
 * @version 0.1
 * @date 2024-11-27
 * 
 * @copyright Copyright (c) 2024
 * 
 * @note While it might be nice to use the flash storage on the NINA module the
 * ArduinoBLE module and WiFiNINA are difficult to use together.  Using the WiFiNINA to
 * access the filesystem disables the BLE functionality.
 * 
 */

#include <Arduino.h>
#include <WiFiNINA.h>
#include <ArduinoMDNS.h>

#include <FastLED.h>

#include "secrets.h"

/** Global defaults */
#define CANDY_STRIPE_WIDTH 5
#define TRAIN_CAR_LENGTH 5
#define HOSTNAME "Library_XmasLights"
#define NUMBER_OF_LIGHTS 150
#define SECONDS_BETWEEN_EFFECTS 5
#define DATA_PIN 3
#define LED_BRIGHTNESS 64
#define MAX_POWER_mW 5000

#define MAX_DEBUG_BUFF 256
#define LOG(...) \
{ \
  char _buff[MAX_DEBUG_BUFF]; \
  snprintf(_buff, MAX_DEBUG_BUFF, __VA_ARGS__); \
  Serial.print(_buff); \
}


CRGBArray<NUMBER_OF_LIGHTS> leds;

int currentEffectNbr = 0;
const int nbrOfEffects = 7;

/** Web server including mDNS support */
WiFiUDP udp;
MDNS mdns(udp);
WiFiServer server(80);                        // Our WiFi server listens on port 80


/** Comet */
void comet(unsigned int nbrOfLEDS, HSVHue cometHue = HUE_RED) {

  const int cometSize = 10;
  static int iDirection = 1;
  static int iPos = 0;
  const int fadeAmt = 64;

  iPos += iDirection;

  if (iPos == (nbrOfLEDS - cometSize) || iPos == 0)
    iDirection *= -1;

  for (int i = 0; i < cometSize; i++)
    leds[iPos + 1].setHue(cometHue);

  for (int j = 0; j < nbrOfLEDS; j++)
    if (random(2) == 1)
      leds[j] = leds[j].fadeToBlackBy(fadeAmt);

}

/** Sparkle  */
void sparkle(unsigned int nbrOfLEDS) {

  static const unsigned int nbrOfColors = 6;

  static const CRGB sparkleColors [nbrOfColors] =
  {
    CRGB::Red,
    CRGB::Blue,
    CRGB::Purple,
    CRGB::Black,
    CRGB::Green,
    CRGB::Orange
  };

  EVERY_N_MILLISECONDS(750) {
    for (int i = 0; i < nbrOfLEDS; i++) 
      leds[i] = sparkleColors[random(nbrOfColors)];
  }
}

/** Twinkle stars */
void twinkleStar(unsigned int nbrOfLEDS) {

  static const unsigned int nbrOfColors = 5;

  static const CRGB twinkleColors [nbrOfColors] =
  {
    CRGB::Red,
    CRGB::Blue,
    CRGB::Purple,
    CRGB::Green,
    CRGB::Orange
  };

  static unsigned int passCount = 0;

  EVERY_N_MILLISECONDS(200) {
    passCount++;

    if (passCount == nbrOfLEDS / 4) 
    {
      passCount = 0;
      FastLED.clear(false);
    }
    
    leds[random(nbrOfLEDS)] = twinkleColors[random(nbrOfColors)];
  }

}

/** Green and Red Train */
void train(unsigned int nbrLEDS, unsigned int trainLength = 10) {

  static int offset = 0;      // train position, advanced each call

  EVERY_N_MILLISECONDS(100) {
    FastLED.clear();
    for (int j = 0; j < trainLength; j++) {
      if ((j + offset) < nbrLEDS) {
        leds[j + offset] = CRGB::DarkRed;
      }
      if ((j + trainLength + offset) < nbrLEDS) {
        leds[j + trainLength + offset] = CRGB::DarkGreen;
      }
    }
    
    offset = (offset + 1) % nbrLEDS;
  }
}

/** Rotating candy cane - move the candy cane one step everytime we're called 
 * 
 * @note While safe to call with any width strip, to avoid artifacts the
 * nbrOfLEDS should be divisible by 2 * stripWidth.
*/
void candyCane(unsigned int nbrLEDS, unsigned int stripeWidth = CANDY_STRIPE_WIDTH, CRGB stripeColor = CRGB::Red) {

  static int offset = 0;

  EVERY_N_MILLISECONDS(500) {
    // Fill all with background color - white
    fill_solid(leds, nbrLEDS, CRGB::White);
    
    // Draw the red stripes - compute the number of stripes
    for (int i = 0; i < (nbrLEDS / stripeWidth); i += 2) {
      // Draw each strip 2 strip widths apart.
      for (int j = i * stripeWidth; j < min(((i * stripeWidth) + stripeWidth), nbrLEDS); j++) {
        // Serial.print(j); Serial.print(",");
        leds[(j + offset) % nbrLEDS] = stripeColor;
      }
    }

    offset = (offset + 1) % nbrLEDS;
  }
}

/** Rotating American Flag */
void redWhiteBlue(unsigned int nbrLEDS, unsigned int stripeWidth = 5) {

  static int offset = 0;

  EVERY_N_MILLISECONDS(500) {
    
    FastLED.clear(false);

    for (int i = 0; i < nbrLEDS; i++) {
      switch ((i % (3 * stripeWidth)) / stripeWidth) {
        case 0:
          leds[(i + offset) % nbrLEDS] = CRGB::DarkBlue;
          break;
        case 1:
          leds[(i + offset) % nbrLEDS] = CRGB::White;
          break;
        case 2:
          leds[(i + offset) % nbrLEDS] = CRGB::DarkRed;
          break;
      }
    }
    
    offset = (offset + 1) % nbrLEDS;
  }
}

/** random green and red */
void randomGreenAndRed(unsigned int nbrLEDS) {

  EVERY_N_MILLISECONDS(500) {
    for (int i = 0; i < nbrLEDS; i++) {
      leds[i] = random(10) > 5 ? CRGB::DarkRed : CRGB::DarkGreen;
    }
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

/** Respond to can web client connects and refresh the web status page. */
void processAnyWebRequests() {

  WiFiClient client = server.available();
  if (client) {
    // an HTTP request ends with a blank line
    bool current_line_is_blank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if we've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so we can send a reply
        if (c == '\n' && current_line_is_blank) {
          // send a standard HTTP response header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();

          client.println(F("<!DOCTYPE HTML>"));
          client.println(F("<html>"));
          client.print(F("<h1>"));
          client.print(HOSTNAME);
          client.println(F("</h1>"));
          client.println("<h2>LED Status</h2>");
          client.print(F("Power Draw =  "));
          client.print(calculate_unscaled_power_mW(leds, NUMBER_OF_LIGHTS) * ((float) LED_BRIGHTNESS / 255.0));
          client.println(F(" mW"));
          client.println(F("<br />"));
          client.print(F("FPS = "));
          client.println(FastLED.getFPS());
          client.println(F("<br />"));
          client.print(F("Effect Number = "));
          client.println(currentEffectNbr);
          client.println(F("</html>"));

          break;
        }
        if (c == '\n') {
          // we're starting a new line
          current_line_is_blank = true;
        } else if (c != '\r') {
          // we've gotten a character on the current line
          current_line_is_blank = false;
        }
      }
    }
    
    // give the web browser time to receive the data
    delay(1);
    client.stop();
    }
}


void setup() {

  Serial.begin(115200);
  delay(3000);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.setHostname(HOSTNAME);             // Use this host name in the DHCP registration

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PWD);
    delay(10000); 
  }

  delay(5000);

  printWifiStatus();

  // start the web server
  server.begin();

  // Register our services via mDNS
  mdns.begin(WiFi.localIP(), HOSTNAME);
  mdns.addServiceRecord("XmasLights_controller._http", 80, MDNSServiceTCP);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUMBER_OF_LIGHTS);
  FastLED.setMaxPowerInMilliWatts(MAX_POWER_mW);
  set_max_power_indicator_LED(LED_BUILTIN);

  FastLED.setBrightness(LED_BRIGHTNESS);
}

void loop() {

  mdns.run();     // allow any mDNS pending processing

  switch(currentEffectNbr) {
    case 0:
      candyCane(NUMBER_OF_LIGHTS);
      break;
    case 1:
      twinkleStar(NUMBER_OF_LIGHTS);
      break;
    case 2:
      comet(NUMBER_OF_LIGHTS, HUE_ORANGE);
      break;
    case 3:
      train(NUMBER_OF_LIGHTS);
      break;
    case 4:
      sparkle(NUMBER_OF_LIGHTS);
      break;
    case 5: 
      redWhiteBlue(NUMBER_OF_LIGHTS);
      break;
    case 6:
      randomGreenAndRed(NUMBER_OF_LIGHTS);
      break;
  }

  FastLED.show();

  processAnyWebRequests();            // Check if we have any requests and handle them.

  EVERY_N_SECONDS(SECONDS_BETWEEN_EFFECTS) {
    currentEffectNbr = (currentEffectNbr + 1) % nbrOfEffects;
    FastLED.clear(false);
  }

  delay(50);

}
