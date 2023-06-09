//
// Created by Hessian on 2023/6/5.
//
#include "webconfig.h"
#include "Arduino.h"
#include <EEPROM.h>


//---------------修改此处""内的信息--------------------
//如开启WEB配网则可不用设置这里的参数，前一个为wifi ssid，后一个为密码
MqttConfig mqtt_config;
static WifiConfig wificonf = {{""}, {""}};
static int mqtt_addr = 0; //被写入数据的EEPROM地址编号
static int wifi_addr = sizeof mqtt_config; //被写入数据的EEPROM地址编号
static WiFiManager wm;

//wifi ssid，psw保存到eeprom
void savewificonfig()
{
    //开始写入
    uint8_t *p = (uint8_t*)(&wificonf);
    for (uint i = 0; i < sizeof(wificonf); i++)
    {
        EEPROM.write(i + wifi_addr, *(p + i)); //在闪存内模拟写入
    }
    delay(10);
    EEPROM.commit();//执行写入ROM
    delay(10);
}

//删除原有eeprom中的信息
void deletewificonfig()
{
    WifiConfig deletewifi = {{""}, {""}};
    uint8_t *p = (uint8_t*)(&deletewifi);
    for (uint i = 0; i < sizeof(deletewifi); i++)
    {
        EEPROM.write(i + wifi_addr, *(p + i)); //在闪存内模拟写入
    }
    delay(10);
    EEPROM.commit();//执行写入ROM
    delay(10);
}

//从eeprom读取WiFi信息ssid，psw
void readwificonfig()
{
    uint8_t *p = (uint8_t*)(&wificonf);
    for (uint i = 0; i < sizeof(wificonf); i++)
    {
        *(p + i) = EEPROM.read(i + wifi_addr);
    }
    // EEPROM.commit();
    // ssid = wificonf.stassid;
    // pass = wificonf.stapsw;
    Serial.printf("Read WiFi Config.....\r\n");
    Serial.printf("SSID:%s\r\n", wificonf.stassid);
    Serial.printf("PSW:%s\r\n", wificonf.stapsw);
    Serial.printf("Connecting.....\r\n");
}

//从eeprom读取mqtt信息
void readMqttConfig()
{
    uint8_t *p = (uint8_t*)(&mqtt_config);
    for (uint i = 0; i < sizeof(mqtt_config); i++)
    {
        *(p + i) = EEPROM.read(i + mqtt_addr);
    }
}

String getParam(String name) {
    //read parameter from server, for customhmtl input
    String value;
    if (wm.server->hasArg(name)) {
        value = wm.server->arg(name);
    }
    return value;
}

void saveParamCallback() {
    Serial.println("[CALLBACK] saveParamCallback fired");

    getParam("MqttHost").toCharArray(mqtt_config.host, 64);
    getParam("MqttUsername").toCharArray(mqtt_config.username, 32);
    getParam("MqttPassword").toCharArray(mqtt_config.password, 64);
    mqtt_config.port = getParam("MqttPort").toInt();


    //开始写入
    uint8_t *p = (uint8_t*)(&mqtt_config);
    for (uint i = 0; i < sizeof(mqtt_config); i++)
    {
        EEPROM.write(i + mqtt_addr, *(p + i)); //在闪存内模拟写入
    }
    delay(10);
    EEPROM.commit();//执行写入ROM
    delay(10);
    Serial.println("mqtt_config saved");
}


//WEB配网函数
void Webconfig(String clientId)
{
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

    delay(3000);
    wm.resetSettings(); // wipe settings

    WiFiManagerParameter header("<h1>MQTT 设置</h1>");
    WiFiManagerParameter clientIdParam(clientId.c_str());
    WiFiManagerParameter param_mqtt_host("MqttHost", "服务器", "w7afeb1b.ala.cn-hangzhou.emqxsl.cn", 64);
    WiFiManagerParameter param_mqtt_port("MqttPort", "端口", "8883", 5);
    WiFiManagerParameter param_mqtt_username("MqttUsername", "用户名", "hessian", 32);
    WiFiManagerParameter param_mqtt_password("MqttPassword", "密码", "", 64);
    WiFiManagerParameter p_lineBreak_notext("<p></p>");


    wm.addParameter(&p_lineBreak_notext);
    wm.addParameter(&header);
    wm.addParameter(&clientIdParam);
    wm.addParameter(&p_lineBreak_notext);
    wm.addParameter(&param_mqtt_host);
    wm.addParameter(&param_mqtt_port);
    wm.addParameter(&param_mqtt_username);
    wm.addParameter(&param_mqtt_password);
    wm.addParameter(&p_lineBreak_notext);
    wm.addParameter(&p_lineBreak_notext);
    wm.addParameter(&p_lineBreak_notext);
    wm.setSaveParamsCallback(saveParamCallback);

    // custom menu via array or vector
    //
    // menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
    // const char* menu[] = {"wifi","info","param","sep","restart","exit"};
    // wm.setMenu(menu,6);
    std::vector<const char *> menu = {"wifi", "restart"};
    wm.setMenu(menu);

    // set dark theme
    wm.setClass("invert");

    //set static ip
    // wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
    // wm.setShowStaticFields(true); // force show static ip fields
    // wm.setShowDnsFields(true);    // force show dns field always

    // wm.setConnectTimeout(20); // how long to try to connect for before continuing
    //  wm.setConfigPortalTimeout(30); // auto close configportal after n seconds
    // wm.setCaptivePortalEnable(false); // disable captive portal redirection
    // wm.setAPClientCheck(true); // avoid timeout if client connected to softap

    // wifi scan settings
    // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
    wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
    // wm.setShowInfoErase(false);      // do not show erase button on info page
    // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons

    // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    res = wm.autoConnect("WitgineIOT"); // anonymous ap
    //  res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

    while (!res);
}

void TryWebconfig(String clientId) {
    Serial.print("正在连接WIFI...\n");

    readwificonfig();

    size_t ssidLen = strlen(wificonf.stassid);

    if (ssidLen > 0) {
        Serial.printf("SSID: %s len: %d\n", wificonf.stassid, ssidLen);
        Serial.printf("PWD: %s len: %d\n", wificonf.stapsw, strlen(wificonf.stapsw));

        WiFi.begin(wificonf.stassid, wificonf.stapsw);
    } else {
        Serial.println("No saved wifi config, start webconfig");
    }

    int tryNum = 0;
    while (WiFi.status() != WL_CONNECTED) {
        if ((tryNum == 0 && ssidLen == 0) || tryNum++ > 240) {
            Webconfig(clientId);
            Serial.print("SSID:");
            Serial.println(WiFi.SSID().c_str());
            Serial.print("PSW:");
            Serial.println(WiFi.psk().c_str());
            strcpy(wificonf.stassid, WiFi.SSID().c_str()); //名称复制
            strcpy(wificonf.stapsw, WiFi.psk().c_str()); //密码复制
            savewificonfig();
            break;
        }
        Serial.print(".");
        delay(500);
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
}

void WiFiResetSettings() {
    wm.resetSettings();
}