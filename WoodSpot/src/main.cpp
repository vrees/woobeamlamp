#include "WiFi.h"
#include <PubSubClient.h>
#include "vrees_neopixel.h"
#include "rotary_encoder.h"

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0 0

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT 13

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 5000

#define LED_PIN 19

#define ESSTISCH_MAX_BRIGHTNESS 1024
#define ESSTISCH_INIT_BRIGHTNESS 500

int brightness = 500; // how bright the LED is

// const char *ssid = "HausRees-Draytek";
// const char *password = "6422048768813046";

char ssid[] = "HausRees-Draytek";     //  your network SSID (name)
char password[] = "6422048768813046"; // your network password

const char *mqttServer = "192.168.178.84";
const int mqttPort = 1883;
const char *mqttUser = "pi-mqtt";
const char *mqttPassword = "gs5lzvy8";
const char *topicBrightNess = "wohnung/wohnen/woodspot/brightness";
const char *topicColor = "wohnung/wohnen/woodspot/color";

WiFiClient espClient;
PubSubClient pubSubClient;

uint32_t min(uint32_t valueA, uint32_t valueB)
{
  if (valueA < valueB)
    return valueA;
  else
    return valueB;
}

// value has to be between 0 and valueMax
void writePwmBrightness(uint32_t value)
{
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8192 / ESSTISCH_MAX_BRIGHTNESS) * min(value, ESSTISCH_MAX_BRIGHTNESS);
  Serial.print("Duty=");
  Serial.println(duty);
  ledcWrite(LEDC_CHANNEL_0, duty);
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();

  Serial.print("setup_wifi: Connecting to ");
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
  delay(10);
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
    brightness = atoi(messageTemp.c_str());
    Serial.print("Changing brightness output to ");
    Serial.print(brightness);
    Serial.println(" \%\%");
    writePwmBrightness(brightness);
  }
  else if (String(topic) == topicColor)
  {
    Serial.print("Changing RGB Color to ");
    Serial.println(messageTemp);

    setColor(strtol(messageTemp.c_str(), NULL, 16));
  }
}

boolean mqtt_connect()
{
  boolean is_connected = true;

  if (!pubSubClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (pubSubClient.connect("ESP32Client", mqttUser, mqttPassword))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.println(pubSubClient.state());
      is_connected = false;
    }
  }
  return is_connected;
}

void subscribeTopics()
{
  pubSubClient.subscribe(topicBrightNess);
  pubSubClient.subscribe(topicColor);
}

void reconnect()
{
  while (!pubSubClient.connected())
  {
    if (mqtt_connect())
      subscribeTopics();
  }
}

void changeBrightness(int delta)
{
  brightness += delta;

  if (brightness >= ESSTISCH_MAX_BRIGHTNESS)
  {
    brightness = ESSTISCH_MAX_BRIGHTNESS;
  }
  if (brightness < 0)
  {
    brightness = 0;
  }

  writePwmBrightness(brightness);
  pubSubClient.publish(topicBrightNess, String(brightness).c_str());
}

void setup_mqtt()
{
  Serial.println("\nsetup_mqtt: ...");
  pubSubClient.setClient(espClient);
  pubSubClient.setServer(mqttServer, mqttPort);
  pubSubClient.setCallback(callback);

  boolean is_connected = mqtt_connect();
  pubSubClient.publish(topicBrightNess, String(brightness).c_str());
  Serial.print("setup_mqtt: Connected=");
  Serial.println(is_connected);
}

void setup()
{
  Serial.begin(115200);

  // Setup timer and attach timer to a led pin
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL_0);
  brightness = ESSTISCH_INIT_BRIGHTNESS;
  changeBrightness(0);

  setup_wifi();
  setup_mqtt();

  setup_neopixel();
  setup_rotary_encoder();
  subscribeTopics();
}

void loop()
{
  if (!pubSubClient.connected())
  {
    reconnect();
  }
  pubSubClient.loop();

  rotary_encoder_loop();
}