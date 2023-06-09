//
// Created by Hessian on 2023/3/27.
//

#include "Arduino.h"
#include "main.h"
#include "webconfig.h"
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include "utils.h"

// MQTT Broker
const char *mqtt_topic = "menjin/"; // define topic
extern MqttConfig mqtt_config;

#define I2C_MASTER_SDA 4
#define I2C_MASTER_SCL 5

// init wifi client
WiFiClientSecure net;
PubSubClient mqttClient(net);

// 3. fingerprint of EMQX Cloud Serverless. Host: *.emqxsl.cn
const char* fingerprint = "7E:52:D3:84:48:3C:5A:9F:A4:39:9A:8B:27:01:B1:F8:C6:AD:D4:47";

static uint8_t I2C_SLAVE_ADDR = 0x50;

static TwoWire wireMaster;

static String clientId;

typedef enum {
    CMD_NONE = 0,
    CMD_OPEN = 1,
    CMD_SEQ = 2,
    // 免提
    CMD_HAND_FREE = 3,
    // 开锁
    CMD_UNLOCK = 4,
} I2C_CMD;

typedef enum {
    STATE_OK,
    STATE_ERROR,
} MQTT_STATE;

static uint8_t i2cReceiveBuffer[16];
static volatile int i2cReceiveByteCount = 0;
static volatile I2C_CMD cmdSignal = CMD_NONE;
static MQTT_STATE mqttState = STATE_OK;
static unsigned long mqttLastConnectTime = 0;

// define two Tasks for DigitalRead & AnalogRead
static void onMqttMessage(char* topic, byte* payload, unsigned int length);
void mqttConnect();
IRAM_ATTR void onResetPressed();
IRAM_ATTR void onOpenPressed();
IRAM_ATTR void onUnlockPressed();
IRAM_ATTR void onHandFreePressed();

void setup() {
    // setup i2c wireMaster as master
    wireMaster.begin(I2C_MASTER_SDA, I2C_MASTER_SCL); // SDA, SCL
    wireMaster.setClock(5000);

    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);

    while (!Serial) {
        delay(10);
    }

    EEPROM.begin(1024);

    if (digitalRead(2) == LOW) {
        Serial.println("onResetPressed");
        deletewificonfig();
        ESP.restart();
        return;
    }

    // 初始化LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, 0);
    pinMode(2, INPUT_PULLUP);
    pinMode(12, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(2), onResetPressed, CHANGE);
    attachInterrupt(digitalPinToInterrupt(12), onOpenPressed, CHANGE);
    attachInterrupt(digitalPinToInterrupt(13), onHandFreePressed, CHANGE);
    attachInterrupt(digitalPinToInterrupt(14), onUnlockPressed, CHANGE);

#ifdef MQTT_CLIENT_ID
    clientId = MQTT_CLIENT_ID;
#else
    clientId = String("wtdc-") + String(EspClass::getChipId());
#endif

    // 初始化WiFi
    TryWebconfig(clientId);

    // 读取MQTT配置
    readMqttConfig();

    // 初始化MQTT
    Serial.printf("WiFi connected, IP address: %s\nStart MQTT connect...\n", WiFi.localIP().toString().c_str());
    // MQTT brokers usually use port 8883 for secure connections.

    net.setFingerprint(fingerprint);
    Serial.printf("MQTT Server: %s:%d\n", mqtt_config.host, mqtt_config.port);
    mqttClient.setServer(mqtt_config.host, mqtt_config.port);
    mqttClient.setCallback(onMqttMessage);

    mqttConnect();
}

void loop() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    mqttClient.loop();

    if (!mqttClient.connected()) {
        digitalWrite(LED_BUILTIN, 1);
        mqttConnect();
    }

/*
    // 有错误就报三长两端
    if (mqttState == STATE_ERROR) {
        light(500);
        light(500);
        light(500);
        light(150);
        light(150);
    }*/

    char buf[128];

    // i2c ISR收到数据后，会触发i2cOnReceive，然后在这里处理
    if (i2cReceiveByteCount > 0) {
        digitalWrite(LED_BUILTIN, 1);
        sprintf(buf, "i2c prepare to write %d bytes: ", i2cReceiveByteCount);

        // 输出键码
        wireMaster.beginTransmission(I2C_SLAVE_ADDR);
        for (int i = 0; i < i2cReceiveByteCount; ++i) {
            sprintf(buf, "%d ", i2cReceiveBuffer[i]);
            wireMaster.write((uint8_t)i2cReceiveBuffer[i]);
        }
        Serial.println();
        int ret = wireMaster.endTransmission(true);

        sprintf(buf, "wireMaster.endTransmission ret: %d \n", ret);
        // 重置接收状态
        i2cReceiveByteCount = 0;
    } else if (cmdSignal != CMD_NONE) {
        digitalWrite(LED_BUILTIN, 1);
        Serial.printf("receive signal %d\n", cmdSignal);
        if (cmdSignal == CMD_OPEN) {
            int ret;
            // 接听
            wireMaster.beginTransmission(I2C_SLAVE_ADDR);
            wireMaster.write(97u);
            ret = wireMaster.endTransmission(true);
            Serial.printf("send 97 ret %d\n", ret);

            delay(2000);

            // 开锁
            wireMaster.beginTransmission(I2C_SLAVE_ADDR);
            wireMaster.write(99u);
            ret = wireMaster.endTransmission(true);
            Serial.printf("send 99 ret %d\n", ret);

            delay(2000);

            // 挂断
            wireMaster.beginTransmission(I2C_SLAVE_ADDR);
            wireMaster.write(97u);
            ret = wireMaster.endTransmission(true);

            Serial.printf("send 97 ret %d\n", ret);
        }
        else if (cmdSignal == CMD_SEQ) {
            int ret;

            for (int i = 0; i < i2cReceiveByteCount; i ++) {
                uint8_t val = i2cReceiveBuffer[i];
                if (i % 2) {
                    Serial.printf("delay %ds\n", val);
                    delay(val * 1000);
                } else {
                    // 接听
                    wireMaster.beginTransmission(I2C_SLAVE_ADDR);
                    wireMaster.write(val);
                    ret = wireMaster.endTransmission(true);
                    Serial.printf("send %d ret %d\n", val, ret);
                }
            }
            i2cReceiveByteCount = 0;
        }
        else if (cmdSignal == CMD_HAND_FREE) {
            // 接听
            wireMaster.beginTransmission(I2C_SLAVE_ADDR);
            wireMaster.write(97u);
            int ret = wireMaster.endTransmission(true);
            Serial.printf("send 97 ret %d\n", ret);
        }
        else if (cmdSignal == CMD_UNLOCK) {
            // 接听
            wireMaster.beginTransmission(I2C_SLAVE_ADDR);
            wireMaster.write(99u);
            int ret = wireMaster.endTransmission(true);
            Serial.printf("send 99 ret %d\n", ret);
        }

        cmdSignal = CMD_NONE;
    }
}


/*--------------------------------------------------*/
/*-------------------- Functions -------------------*/
/*--------------------------------------------------*/

void mqttConnect() {
    if (millis() - mqttLastConnectTime < 60 * 1000) {
        return;
    }

    mqttState = STATE_ERROR;
    Serial.print("checking wifi...");

    mqttLastConnectTime = millis();

    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        Serial.print(".");
        delay(1000);
    }
    digitalWrite(LED_BUILTIN, 1);

    while (!mqttClient.connected()) {
        Serial.print("\nMQTT connecting...\n");
        Serial.printf("ClientId: %s\n", clientId.c_str());
        Serial.printf("User: %s\n", mqtt_config.username);
        Serial.printf("Pass: %s\n", mqtt_config.password);
        if (mqttClient.connect(clientId.c_str(), mqtt_config.username, mqtt_config.password)) {
            Serial.println("Connected to MQTT broker.");
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" Retrying in 5 seconds.");
            delay(5000);
        }
    }
    digitalWrite(LED_BUILTIN, 0);

    mqttState = STATE_OK;

    String topic = String(mqtt_topic) + clientId;
    mqttClient.subscribe(topic.c_str());
//    mqttClient.publish(mqtt_topic, "im ready");
}

void onMqttMessage(char *topic, byte *p, unsigned int length) {
    p[length] = '\0';
    String payload((char*)p);
    if (length > 1024) {
        Serial.printf("ERROR - Message too long, [%s] : %s\n", topic, payload.c_str());
        return;
    }

    Serial.printf("Message arrived [%s] : %s len: %d\n", topic, payload.c_str(), length);
    char * buf = (char*)malloc(1024 * sizeof(char));

    if (payload.equals("scan")) {
        i2cScan(&wireMaster, buf);

        memset(buf, 0, 2048 * sizeof(char));
    } else if (payload.equals("on") || payload.equals("open")) {
        cmdSignal = CMD_OPEN;
    } else if (payload.equals("restart")) {
       ESP.restart();
    } else if (payload.equals("reset")) {
        WiFiResetSettings();
        ESP.restart();
    } else if (payload.startsWith("sendto")) {
        String sendTo = payload.substring(7, 9);
        String sendContent = payload.substring(10);
        i2cReceiveBuffer[0] = (char)sendContent.toInt();
        i2cReceiveByteCount = 1;
        I2C_SLAVE_ADDR = (int)sendTo.toInt();
        Serial.printf("sendTo: %d, sendContent: %d\n", I2C_SLAVE_ADDR, (uint8_t)sendContent.toInt());
    } else if (payload.startsWith("seq ")) {
        const char *str = payload.substring(4).c_str();
        // strtok() will modify the original string, so we need to make a copy of it

        char *pch = strcpy(buf, str);
        pch = strtok(pch, " ");
        int i = 0;
        while (pch != NULL)
        {
            pch = strtok (NULL, " ");
            i2cReceiveBuffer[i++] = (uint8_t)atoi(pch);
            Serial.printf("%d\n", i2cReceiveBuffer[i-1]);
        }
        i2cReceiveByteCount = i;
    } else {
        wireMaster.beginTransmission(I2C_SLAVE_ADDR);
        wireMaster.write((uint8_t)payload.toInt());
        int ret = wireMaster.endTransmission();
        sprintf(buf, "onMqttMessage wireMaster.endTransmission ret: %d \n", ret);
    }

    free(buf);
}

IRAM_ATTR void onResetPressed(){
    if (digitalRead(2) == LOW) {
        Serial.println("onResetPressed");
        deletewificonfig();
        ESP.restart();
    }
}
IRAM_ATTR void onOpenPressed() {
    if (digitalRead(12) == LOW) {
        Serial.println("onOpenPressed");
        cmdSignal = CMD_OPEN;
    }
}
IRAM_ATTR void onHandFreePressed(){
    if (digitalRead(13) == LOW) {
        Serial.println("onHandFreePressed");
        cmdSignal = CMD_HAND_FREE;
    }
}
IRAM_ATTR void onUnlockPressed(){
    if (digitalRead(14) == LOW) {
        Serial.println("onUnlockPressed");
        cmdSignal = CMD_UNLOCK;
    }
}