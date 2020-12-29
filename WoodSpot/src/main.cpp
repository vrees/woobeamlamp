#include "Arduino.h"
#include "WiFi.h"
#include "ESPmDNS.h"
#include "WiFiUdp.h"
#include "ArduinoOTA.h"
#include "esp_system.h"
#include "PubSubClient.h"
#include "vrees_neopixel.h"
#include "rotary_encoder.h"
#include "main.h"

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_LIGHT 1
#define LEDC_CHANNEL_DECO 0

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT 13

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 5000

#define LED_PIN_BELEUCHTUNG 19 // Esstischlampe, Beleuchtung nach unten
#define LED_PIN_DECORATION 33  // Esstischlampe, Decoration strahlt nach oben

#define MAX_BRIGHTNESS 1024
#define INIT_BRIGHTNESS_LIGHT 400
#define INIT_BRIGHTNESS_DECO 300

operation_mode_t operation_mode;
int brightnessLight = INIT_BRIGHTNESS_LIGHT; // how bright the LED is
int brightnessDeco = INIT_BRIGHTNESS_DECO;

// const char *ssid = "HausRees-Draytek";
// const char *password = "6422048768813046";

char ssid[] = "HausRees-Draytek";     //  your network SSID (name)
char password[] = "6422048768813046"; // your network password

const char *mqttServer = "192.168.178.84";
const int mqttPort = 1883;
const char *mqttUser = "pi-mqtt";
const char *mqttPassword = "gs5lzvy8";
const char *topicLight = "wohnung/wohnen/woodspot/brightness";
const char *topicDecoration = "wohnung/wohnen/woodspot/decoration";
const char *topicColor = "wohnung/wohnen/woodspot/color";

WiFiClient espClient;
PubSubClient pubSubClient;
hw_timer_t *timer = NULL;
uint32_t itColor = 0;
volatile boolean publishBrightnessInNextLoop = false;

void nextOperationMode()
{
  switch (operation_mode)
  {
  case LIGHT:
    operation_mode = DECORATION;
    brightnessLight = 0;
    brightnessDeco = INIT_BRIGHTNESS_DECO;
    break;
  case DECORATION:
    operation_mode = BOTH;
    brightnessLight = INIT_BRIGHTNESS_LIGHT;
    brightnessDeco = INIT_BRIGHTNESS_DECO;
    break;
  default:
    operation_mode = LIGHT;
    brightnessLight = INIT_BRIGHTNESS_LIGHT;
    brightnessDeco = 0;
    break;
  }

  delay(5); // verhindert prellen
  Serial.print("new operation_mode: ");
  Serial.println(operation_mode);

  changeBrightness(0, false);
}

uint32_t min(uint32_t valueA, uint32_t valueB)
{
  if (valueA < valueB)
    return valueA;
  else
    return valueB;
}

// value has to be between 0 and valueMax
void writePwmBrightness(uint32_t value, uint8_t channel)
{
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8192 / MAX_BRIGHTNESS) * min(value, MAX_BRIGHTNESS);

  Serial.print("Changing brightness output to ");
  Serial.print(value);
  Serial.print(", duty=");
  Serial.print(duty);
  Serial.print(", channel=");
  Serial.println(channel);

  ledcWrite(channel, duty);
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();

  Serial.print("setup_wifi: Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(10);
}

void setup_ota()
{
  ArduinoOTA.setHostname("woodspot");

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
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

  if (String(topic) == topicLight)
  {
    brightnessLight = atoi(messageTemp.c_str());
    writePwmBrightness(brightnessLight, LEDC_CHANNEL_LIGHT);
  }
  else if (String(topic) == topicDecoration)
  {
    brightnessDeco = atoi(messageTemp.c_str());
    writePwmBrightness(brightnessDeco, LEDC_CHANNEL_DECO);
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
  pubSubClient.subscribe(topicLight);
  pubSubClient.subscribe(topicDecoration);
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

void IRAM_ATTR publishBrightnessIsr()
{
  publishBrightnessInNextLoop = true;
}

void startDelayTimer(int msec)
{
  Serial.println("Start delay timer for ISR publishBrightness");

  timer = timerBegin(0, 80, true);                          //timer 0, div 80
  timerAttachInterrupt(timer, &publishBrightnessIsr, true); //attach callback
  timerAlarmWrite(timer, msec * 1000, false);               //set time in us
  timerAlarmEnable(timer);
}

void publishBrightness()
{
  pubSubClient.publish(topicLight, String(brightnessLight).c_str(), true);
  pubSubClient.publish(topicDecoration, String(brightnessDeco).c_str(), true);
}

void writeBrightness(int brightness, uint8_t channel)
{
  writePwmBrightness(brightness, channel);
}

int ensureBrightnessRange(int brightness, int delta)
{
  int effectiveDelta = delta;

  if (brightness <= 100)
    effectiveDelta = delta / 2;
  else if (brightness <= 20)
    effectiveDelta = delta / 5;

  int newBrightness = brightness + effectiveDelta;

  if (newBrightness >= MAX_BRIGHTNESS)
  {
    newBrightness = MAX_BRIGHTNESS;
  }
  if (newBrightness < 0)
  {
    newBrightness = 0;
  }

  return newBrightness;
}

void changeBrightness(int delta, boolean delayMqttPublish = false)
{
  switch (operation_mode)
  {
  case LIGHT:
    brightnessLight = ensureBrightnessRange(brightnessLight, delta);
    writeBrightness(brightnessLight, LEDC_CHANNEL_LIGHT);
    writeBrightness(brightnessDeco, LEDC_CHANNEL_DECO);
    break;
  case DECORATION:
    writeBrightness(brightnessLight, LEDC_CHANNEL_LIGHT);
    brightnessDeco = ensureBrightnessRange(brightnessDeco, delta);
    writeBrightness(brightnessDeco, LEDC_CHANNEL_DECO);
    break;
  case BOTH:
    brightnessLight = ensureBrightnessRange(brightnessLight, delta);
    writeBrightness(brightnessLight, LEDC_CHANNEL_LIGHT);
    brightnessDeco = ensureBrightnessRange(brightnessDeco, delta);
    writeBrightness(brightnessDeco, LEDC_CHANNEL_DECO);
    break;
  }

  if (delayMqttPublish)
  {
    startDelayTimer(1000);
  }
  else
    publishBrightness();
}

void setup_mqtt()
{
  Serial.println("\nsetup_mqtt: ...");
  pubSubClient.setClient(espClient);
  pubSubClient.setServer(mqttServer, mqttPort);
  pubSubClient.setCallback(callback);

  boolean is_connected = mqtt_connect();
  pubSubClient.publish(topicLight, String(brightnessLight).c_str(), true);
  pubSubClient.publish(topicDecoration, String(brightnessDeco).c_str(), true);
  pubSubClient.publish(topicColor, String("000000").c_str(), true);

  delay(2000);
  Serial.print("setup_mqtt: Published default Brightness and color. Connected=");
  Serial.println(is_connected);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");

  // Setup timer and attach timer to a led pin
  ledcSetup(LEDC_CHANNEL_LIGHT, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN_BELEUCHTUNG, LEDC_CHANNEL_LIGHT);

  ledcSetup(LEDC_CHANNEL_DECO, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN_DECORATION, LEDC_CHANNEL_DECO);

  Serial.print("operation_mode: ");
  Serial.println(operation_mode);

  nextOperationMode(); // defaults to LIGHT

  setup_wifi();
  setup_ota();
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
  // Serial.println(itColor);
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
    publishBrightness();
  }

  rotary_encoder_loop();
  color_loop();
  ArduinoOTA.handle();
  delay(1);
}