
#ifndef MAIN_H
#define MAIN_H

typedef enum
{
    LIGHT = 1,      // Beleuchtung unten
    DECORATION = 2, // Decobeleuchtung nach oben
    BOTH = 3,       // Alle Leds an. Oben und unten
} operation_mode_t;

extern operation_mode_t operation_mode;

void changeBrightness(int delta, boolean delayMqttPublish);
void nextOperationMode();

#endif /* MAIN_H */
