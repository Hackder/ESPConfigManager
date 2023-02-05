#include <Arduino.h>
#include <ESP8266WiFi.h>

#define CONFIG      \
  S(name)           \
  X(int, rotations) \
  X(bool, enabled)  \
  X(int, test)      \
  S(test2)          \
  X(bool, zimnyCas)
#include <ESPConfigManager.h>

const char *ssid = "ssid";
const char *pass = "pass";

const Config defaultConfig = {
    .name = "Slavo",
    .rotations = 10,
    .enabled = true,
    .test = 10,
    .test2 = "test2",
    .zimnyCas = true,
    };
ESPConfigManager configManager(defaultConfig, "admin", "admin", 1024);

void setup()
{
  Serial.begin(9600);
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  configManager.start();
}

long long last = 0;

void loop()
{
  configManager.handle();

  if (millis() - last > 5000)
  {
    Serial.println(configManager.getConfig().name);
    Serial.println(configManager.getConfig().rotations);
    Serial.println(configManager.getConfig().enabled);
    Serial.println(configManager.getConfig().test);
    Serial.println(configManager.getConfig().test2);
    Serial.println("---------------------");
    last = millis();
  }
}
