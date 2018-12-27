#include "WiFi.h"
#include <PubSubClient.h>
#include "vrees_neopixel.h"

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0 0

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT 13

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 5000

#define LED_PIN 19

// used in this example to print variables every 10 seconds
unsigned long printEntry;
unsigned long fadeMillis;
int brightness = 0; // how bright the LED is
int fadeAmount = 5; // how many points to fade the LED by

// const char *ssid = "HausRees-Draytek";
// const char *password = "6422048768813046";

char ssid[] = "HausRees-Draytek";     //  your network SSID (name)
char password[] = "6422048768813046"; // your network password

const char *mqttServer = "192.168.178.84";
const int mqttPort = 1883;
const char *mqttUser = "pi-mqtt";
const char *mqttPassword = "gs5lzvy8";
const char *topicBrightNess = "wohnung/wohnen/woodspot/brightness";

WiFiClient espClient;
PubSubClient client(espClient);

uint32_t min(uint32_t valueA, uint32_t valueB)
{
  if (valueA < valueB)
    return valueA;
  else
    return valueB;
}

// Arduino like analogWrite
// value has to be between 0 and valueMax
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255)
{
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);
  Serial.print("Duty=");
  Serial.println(duty);
  ledcWrite(channel, duty);
}

void autoFade()
{
  if (millis() - fadeMillis > 30)
  { // Serial.print the example variables every 10 seconds
    // set the brightness on LEDC channel 0

    Serial.print("working5 - brightness=");
    Serial.println(brightness);

    ledcAnalogWrite(LEDC_CHANNEL_0, brightness);

    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;

    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 0 || brightness >= 255)
    {
      fadeAmount = -fadeAmount;
    }

    fadeMillis = millis();
  }
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == topicBrightNess)
  {
    int prozent = atoi(messageTemp.c_str());
    Serial.print("Changing output to ");
    Serial.print(prozent);
    Serial.println("\%");
    ledcAnalogWrite(LEDC_CHANNEL_0, prozent, 1000);
  }
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client", mqttUser, mqttPassword))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe(topicBrightNess);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void setup()
{
  Serial.begin(115200);
  setup_wifi();

  // Setup timer and attach timer to a led pin
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL_0);

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  neopixel_setup();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  neopixel_loop();
}