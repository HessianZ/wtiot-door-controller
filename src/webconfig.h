//
// Created by Hessian on 2023/6/5.
//

#ifndef ESP_DOOR_CONTROLLER_WEBCONFIG_H
#define ESP_DOOR_CONTROLLER_WEBCONFIG_H

#include <WiFiManager.h>

/* *****************************************************************
    参数设置
 * *****************************************************************/

struct MqttConfig
{
    char host[64];//定义配网得到的WIFI名长度(最大32字节)
    uint16 port;//定义配网得到的WIFI名长度(最大32字节)
    char username[32];//定义配网得到的WIFI密码长度(最大64字节)
    char password[64];//定义配网得到的WIFI密码长度(最大64字节)
};

struct WifiConfig
{
    char stassid[32];//定义配网得到的WIFI名长度(最大32字节)
    char stapsw[64];//定义配网得到的WIFI密码长度(最大64字节)
};


void savewificonfig();
void deletewificonfig();
void readwificonfig();
void readMqttConfig();
void Webconfig(String clientId);
void TryWebconfig(String clientId);
void WiFiResetSettings();
String getParam(String name);

#endif //ESP_DOOR_CONTROLLER_WEBCONFIG_H
