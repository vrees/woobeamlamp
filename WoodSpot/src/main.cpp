#include "WiFi.h"
#include "esp_system.h"
#include "PubSubClient.h"
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
hw_timer_t *timer = NULL;
uint32_t itColor = 0;
volatile boolean publishBrightnessInNextLoop = false;

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

  Serial.print("Changing brightness output to ");
  Serial.print(value);
  Serial.print(" \%\%");
  Serial.print(", duty=");
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
  pubSubClient.subscribe(topicColor, 0);
}

void reconnect()
{
  while (!pubSubClient.connected())
  {
    if (mqtt_connect())
      subscribeTopics();
  }
}

void IRAM_ATTR publishBrightness()
{
  publishBrightnessInNextLoop = true;
}

void startDelayTimer(int msec)
{
  Serial.println("Start delay timer for ISR publishBrightness");

  timer = timerBegin(0, 80, true);                       //timer 0, div 80
  timerAttachInterrupt(timer, &publishBrightness, true); //attach callback
  timerAlarmWrite(timer, msec * 1000, false);            //set time in us
  timerAlarmEnable(timer);
}

void changeBrightness(int delta, boolean delayMqttPublish = false)
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

  if (delayMqttPublish)
  {
    startDelayTimer(1000);
  }
  else
    pubSubClient.publish(topicBrightNess, String(brightness).c_str(), true);
}

void setup_mqtt()
{
  Serial.println("\nsetup_mqtt: ...");
  pubSubClient.setClient(espClient);
  pubSubClient.setServer(mqttServer, mqttPort);
  pubSubClient.setCallback(callback);

  boolean is_connected = mqtt_connect();
  pubSubClient.publish(topicBrightNess, String(brightness).c_str(), true);
  pubSubClient.publish(topicColor, String("000000").c_str(), true);

  delay(2000);
  Serial.print("setup_mqtt: Published default Brightness and color. Connected=");
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

void color_loop()
{
  ++itColor;
  setColor(itColor);
  itColor = itColor & 0xFFFFFF;
  Serial.println(itColor);
}

void loop()
{
  if (!pubSubClient.connected())
  {
    reconnect();
  }
  pubSubClient.loop();

  if (publishBrightnessInNextLoop)
  {
    publishBrightnessInNextLoop = false;
    pubSubClient.publish(topicBrightNess, String(brightness).c_str(), true);
  }

  rotary_encoder_loop();
  color_loop();
  delay(1);
}