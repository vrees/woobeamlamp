#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"
#include "main.h"

/*
connecting Rotary encoder
CLK (A pin) - to any microcontroler intput pin with interrupt -> in this example pin 32
DT (B pin) - to any microcontroler intput pin with interrupt -> in this example pin 21
SW (button pin) - to any microcontroler intput pin -> in this example pin 25
VCC - to microcontroler VCC (then set ROTARY_ENCODER_VCC_PIN -1) or in this example pin 25
GND - to microcontroler GND
*/
#define ROTARY_ENCODER_A_PIN 27
#define ROTARY_ENCODER_B_PIN 14
#define ROTARY_ENCODER_BUTTON_PIN 12
#define ROTARY_ENCODER_VCC_PIN -1 /*put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */

#define DEFAULT_STEP_SIZE 20
#define FINE_STEP_SIZE 1

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN);

int steps = DEFAULT_STEP_SIZE;

void rotary_onButtonClick()
{
    nextOperationMode();
}

void rotary_encoder_loop()
{
    //first lets handle rotary encoder button click
    if (rotaryEncoder.currentButtonState() == BUT_RELEASED)
    {
        //we can process it here or call separate function like:
        rotary_onButtonClick();
    }

    // lets see if anything changed
    int16_t encoderDelta = rotaryEncoder.encoderChanged();

    //optionally we can ignore whenever there is no change
    if (encoderDelta == 0)
        return;

    Serial.print("encoderDelta=");
    Serial.println(encoderDelta);

    int delta = 0;
    if (encoderDelta > 0)
        delta = steps;
    if (encoderDelta < 0)
        delta = -steps;

    if (encoderDelta != 0)
    {
        changeBrightness(delta, true);
    }
}

void setup_rotary_encoder()
{
    Serial.println("\nsetup_rotary_encoder ...");
    //we must initialize rorary encoder
    rotaryEncoder.begin();
    rotaryEncoder.setup([] { rotaryEncoder.readEncoder_ISR(); });
    //optionally we can set boundaries and if values should cycle or not
    rotaryEncoder.setBoundaries(0, 1000, true); //minValue, maxValue, cycle values (when max go to min and vice versa)

    rotaryEncoder.enable();

    Serial.println("done!");
}
