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

// We want to be able to edit these example variables below from the wifi config manager
// Currently only char arrays are supported.
// Use functions like atoi() and atof() to transform the char array to integers or floats
// Use IAS.dPinConv() to convert Dpin numbers to integers (D6 > 14)

char* lbl         = "WoodSpot";
char* exampleURL  = "http://vrees-url.com/getdata.php?userid=1234&key=7890abc";
char* nrOf        = "6";

char* doSomething = "1";
char* chosen      = "0";

char* updInt      = "120";
char* ledPin      = "2";
char* timeZone    = "0.0";
char* version     = "1.0.2";




// ================================================ SETUP ================================================
void setup() {
  /* TIP! delete lines below when not used */
  IAS.preSetDeviceName("woodspot");                       // preset deviceName this is also your MDNS responder: http://virginsoil.local
  //IAS.preSetAutoUpdate(false);                            // automaticUpdate (true, false)
  //IAS.preSetAutoConfig(false);                            // automaticConfig (true, false)
  //IAS.preSetWifi("ssid","password");                      // preset Wifi
	

  IAS.addField(lbl, "textLine", 16);                        // These fields are added to the "App Settings" page in config mode and saved to eeprom. Updated values are returned to the original variable.
  IAS.addField(exampleURL, "Textarea", 80, 'T');            // reference to org variable | field label value | max char return | Optional "special field" char
  IAS.addField(nrOf, "Number", 8, 'N');                     // Find out more about the optional "special fields" at https://iotappstory.com/wiki
  
  IAS.addField(doSomething, "Checkbox:Check me", 1, 'C');
  IAS.addField(chosen, "Selectbox:Red,Green,Blue", 1, 'S');

  IAS.addField(updInt, "Interval", 8, 'I');
  IAS.addField(ledPin, "ledPin", 2, 'P');
  IAS.addField(timeZone, "Timezone", 4, 'Z');
  IAS.addField(version, "Version", 10, 'T'); 


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
 
	/* TIP! delete the lines above when not used */
 
  //IAS.begin();
  IAS.begin('P');                                     // Optional parameter: What to do with EEPROM on First boot of the app? 'F' Fully erase | 'P' Partial erase(default) | 'L' Leave intact

  IAS.setCallHome(true);                              // Set to true to enable calling home frequently (disabled by default)
  IAS.setCallHomeInterval(atoi(updInt));                        // Call home interval in seconds, use 60s only for development. Please change it to at least 2 hours in production


  //-------- Your Setup starts from here ---------------

}



// ================================================ LOOP =================================================
void loop() {
  IAS.loop();                                   // this routine handles the calling home functionality and reaction of the MODEBUTTON pin. If short press (<4 sec): update of sketch, long press (>7 sec): Configuration


  //-------- Your Sketch starts from here ---------------

  if (millis() - printEntry > 10000) {                // Serial.print the example variables every 10 seconds

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

    Serial.print(F(" Version\t| versione\t| "));
    Serial.println(atof(version));

    Serial.println(F("*-------------------------------------------------------------------------*"));
    printEntry = millis();
  }

}
