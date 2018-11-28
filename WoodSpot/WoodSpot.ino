#include <dummy.h>

/*
	WoodSpot V1.0.0
*/

#define COMPDATE __DATE__ __TIME__
#define MODEBUTTON 0                                        // Button pin on the esp for selecting modes. D3 for the Wemos!


#include <IOTAppStory.h>                                    // IotAppStory.com library
IOTAppStory IAS(COMPDATE, MODEBUTTON);                      // Initialize IotAppStory



// ================================================ EXAMPLE VARS =========================================
// used in this example to print variables every 10 seconds
unsigned long printEntry;
unsigned long fadeMillis;

// We want to be able to edit these example variables below from the wifi config manager
// Currently only char arrays are supported.
// Use functions like atoi() and atof() to transform the char array to integers or floats
// Use IAS.dPinConv() to convert Dpin numbers to integers (D6 > 14)

char* lbl         = "WoodSpot";
char* exampleURL  = "http://vrees-url.com/getdata.php?userid=1234&key=7890abc";
char* nrOf        = "6";

char* doSomething = "1";
char* chosen      = "0";

char* updInt      = "300";
char* ledPin      = "15";
char* timeZone    = "0.0";
char* version     = "1.0.4";


// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0     0

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT  13

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000

#define LED_PIN            15

int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by

boolean min(uint32_t valueA, uint32_t valueB) {
  if (valueA<valueB) 
    return valueA;
  else
    return valueB;
}

// Arduino like analogWrite
// value has to be between 0 and valueMax
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}



// ================================================ SETUP ================================================
void setup() {
  /* TIP! delete lines below when not used */
  IAS.preSetDeviceName("woodspot");                       // preset deviceName this is also your MDNS responder: http://virginsoil.local
  //IAS.preSetAutoUpdate(false);                            // automaticUpdate (true, false)
  //IAS.preSetAutoConfig(false);                            // automaticConfig (true, false)
  IAS.preSetWifi("San-Nazzaro","ConnectMe2theWorld");                      // preset Wifi	

  IAS.addField(lbl, "textLine", 16);                        // These fields are added to the "App Settings" page in config mode and saved to eeprom. Updated values are returned to the original variable.
  IAS.addField(exampleURL, "Textarea", 80, 'T');            // reference to org variable | field label value | max char return | Optional "special field" char
  IAS.addField(nrOf, "Number", 8, 'N');                     // Find out more about the optional "special fields" at https://iotappstory.com/wiki
  
  IAS.addField(doSomething, "Checkbox:Check me", 1, 'C');
  IAS.addField(chosen, "Selectbox:Red,Green,Blue", 1, 'S');

  IAS.addField(updInt, "Interval", 8, 'I');
  IAS.addField(ledPin, "ledPin", 2, 'P');
  IAS.addField(timeZone, "Timezone", 4, 'Z');
  IAS.addField(version, "Version", 8, 'Z');


  // You can configure callback functions that can give feedback to the app user about the current state of the application.
  // In this example we use serial print to demonstrate the call backs. But you could use leds etc.

  IAS.onModeButtonShortPress([]() {
    Serial.println(F(" If mode button is released, I will enter in firmware update mode."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.onModeButtonLongPress([]() {
    Serial.println(F(" If mode button is released, I will enter in configuration mode."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.onModeButtonVeryLongPress([]() {
    Serial.println(F(" If mode button is released, I won't do anything unless you program me to."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
    /* TIP! You can use this callback to put your app on it's own configuration mode */
  });
  

  IAS.onModeButtonNoPress([]() {
    Serial.println(F(" Mode Button is not pressed."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });
  
  IAS.onFirstBoot([]() {                              
    Serial.println(F(" Run or display something on the first time this app boots"));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });


  IAS.onFirmwareUpdateCheck([]() {
    Serial.println(F(" Checking if there is a firmware update available."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.onFirmwareUpdateDownload([]() {
    Serial.println(F(" Downloading and Installing firmware update."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.onFirmwareUpdateError([]() {
    Serial.println(F(" Update failed...Check your logs"));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.onConfigMode([]() {
    Serial.println(F(" Starting configuration mode. Search for my WiFi and connect to 192.168.4.1."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });
 

  IAS.begin('P');                                     // Optional parameter: What to do with EEPROM on First boot of the app? 'F' Fully erase | 'P' Partial erase(default) | 'L' Leave intact
  IAS.setCallHome(true);                              // Set to true to enable calling home frequently (disabled by default)
  IAS.setCallHomeInterval(atoi(updInt));                        // Call home interval in seconds, use 60s only for development. Please change it to at least 2 hours in production

  //-------- Your Setup starts from here ---------------
  // Setup timer and attach timer to a led pin
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL_0);
}



// ================================================ LOOP =================================================
void loop() {
  IAS.loop();                                   // this routine handles the calling home functionality and reaction of the MODEBUTTON pin. If short press (<4 sec): update of sketch, long press (>7 sec): Configuration


  if (millis() - fadeMillis > 30) {                // Serial.print the example variables every 10 seconds   
    // set the brightness on LEDC channel 0
    ledcAnalogWrite(LEDC_CHANNEL_0, brightness);
  
    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;
  
    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 0 || brightness >= 255) {
      fadeAmount = -fadeAmount;
    }
    
    fadeMillis = millis();
  }

/*
  if (millis() - printEntry > 120000) {                // Serial.print the example variables every 10 seconds
    printStatistic();
    printEntry = millis();
  }
*/
}

void printStatistic() {
    Serial.println(F(" LABEL\t\t| VAR\t\t| VALUE"));
    Serial.print(F(" textLine\t| lbl\t\t| "));
    Serial.println(lbl);
    Serial.print(F(" Textarea\t| exampleURL\t| "));
    Serial.println(exampleURL);
    Serial.print(F(" Number\t\t| nrOf\t\t| "));
    Serial.println(nrOf);
    Serial.print(F(" Checkbox\t| doSomething\t| "));
    Serial.println(doSomething);
    Serial.print(F(" Selectbox\t| chosenColor\t| "));
    Serial.println(chosen);
    Serial.print(F(" Interval\t| updInt\t| "));
    Serial.println(updInt);
    Serial.print(F(" Led pin\t| ledPin\t| "));
    Serial.println(atoi(ledPin));  
    Serial.print(F(" Timezone\t| timeZone\t| "));
    Serial.println(atof(timeZone));
    Serial.println(F("*-------------------------------------------------------------------------*"));
}
