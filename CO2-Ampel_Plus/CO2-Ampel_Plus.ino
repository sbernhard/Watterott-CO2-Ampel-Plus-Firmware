/*
  CO2-Ampel (v2.1)

  Testmodus:
  1. Den Switch-Taster beim Einschalten gedrueckt halten.
  2. Buzzer-Test
  3. LED-Test: rot, gruen, blau
  4. Sensor-Test: LED 1 = Licht, LED 2 = CO2, LED 3 = Temperatur, LED 4 =
  Luftfeuchtigkeit

  Kalibrierung:
  1. Die Ampel bei Frischluft mind. 1 Minute betreiben (im Freien oder am
  offenen Fenster, aber windgeschützt).
  2. Den Testmodus starten.
  3. Nach dem LED-Test (blaue LEDs) den Switch-Taster waehrend des Sensor-Tests
  kurz drücken (Buzzer ertoent).
  4. Die Kalibrierung wird nach dem Sensor-Test ausgeführt und dauert mindestens
  2 Minuten. Die LEDs zeigen dabei den aktuellen CO2-Wert an: gruen bis 499ppm,
  gelb bis 599ppm, rot ab 600ppm
  5. Nach erfolgreicher Kalibrierung leuchten die LEDs kurz blau und der Buzzer
  ertoent.
*/
#include <Arduino.h>
#include <JC_Button.h>
#include <WiFi101.h>
#include "Buzzer.h"
#include "Config.h"
#include "DeviceConfig.h"
#include "Led.h"
#include "NetworkManager.h"
#include "Sensor.h"

byte wifi_state = WIFI_MODE_WPA_CONNECT;
const byte BUTTON_PIN(PIN_SWITCH);
const unsigned long LONG_PRESS(3000);

Button modeButton(BUTTON_PIN);

int wifi_reconnect_attemps = WIFI_RECONNECT_ATTEMPTS;

void setup() {
  // while (!SerialUSB);
  while (!Serial) {
     ; // wait for serial port to connect. Needed for native USB
  }
  Serial.begin(115200);
  Serial.println("------------------------");
  Serial.println("Starting setup...");
  Serial.println("Watterott CO2 Ampel PLUS");
  Serial.println("Firmware version v1.0.1");
#if DEBUG_LOG > 0
  Serial.println("Loglevel set to DEBUG!");
  Serial.println("--- !!! WARNING !!! ---");
  Serial.println("Debug log exposes sensitive");
  Serial.println("login informtation to the");
  Serial.println("serial console. Never use");
  Serial.println("in production environment!");
#endif
  led_init();
  led_set_color(LED_WHITE);
  led_update();

  modeButton.begin();
  modeButton.read();

  buzzer_init();
  buzzer_test();

  sensor_init();
  /**
   * Factory Reset when button is pressed while reset
   */
  if (!config_is_initialized() || modeButton.isPressed()) {
    Serial.println("Loading factory defaults");
    led_off();
    led_set_color(LED_RED);
    led_update();
    delay(50);
    config_set_factory_defaults();
    led_off();
  }
  Serial.println("Setup complete!");
  Serial.println("------------------------");
}

void loop() {
  /**
   * Start WiFi Access Point when Button is pressed for more than 3 seconds
   */
  modeButton.read();
  if (modeButton.pressedFor(3000)) {
    wifi_state = WIFI_MODE_AP_INIT;
  }

  switch (wifi_state) {
    case WIFI_MODE_AP_INIT:  // Create  an Access  Point
      Serial.println("Creating Access Point");
      wifi_ap_create();
      wifi_state = WIFI_MODE_AP_LISTEN;
      Serial.println("------------------------");
      break;

    case WIFI_MODE_WPA_CONNECT:  // Connect to WiFi

      device_config_t cfg = config_get_values();
      Serial.print("Connecting to SSID ");
      Serial.print(cfg.wifi_ssid);
      Serial.println(" Wifi");
      if (strlen(cfg.wifi_ssid) != 0) {
        if (wifi_wpa_connect() == WL_CONNECTED) {
          wifi_state = WIFI_MODE_WPA_LISTEN;
        } else {
          wifi_state = WIFI_MODE_WPA_CONNECT;
        }
      } else {
        Serial.println("No WiFi SSID Configured.");
        wifi_state = WIFI_MODE_NOT_CONECTED;
      }
      Serial.println("------------------------");
      Serial.println("Start measurement");
      Serial.println("");
      break;
  }

  wifi_handle_client();
  sensor_handler();
  sensor_handle_brightness();
}
