//
// Created by Hessian on 2023/3/29.
//

#ifndef ESP_DOOR_CONTROLLER_UTILS_H
#define ESP_DOOR_CONTROLLER_UTILS_H

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <Wire.h>

int i2cScan(TwoWire *twoWire, char* result);

#endif //ESP_DOOR_CONTROLLER_UTILS_H
