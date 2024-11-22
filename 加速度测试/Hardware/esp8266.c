#include "esp8266.h"


void esp8266_init(void)
{
    Serial_SendString("AT+RST\r\n"); // 复位
    Delay_s(1);
    Serial_SendString("ATE0\r\n"); // 关闭回显
    Delay_s(1);
    Serial_SendString("AT+CWMODE=3\r\n"); // 设置双模式
    Delay_s(1);
    Serial_SendString("AT+CWJAP=\"Xiaomi 14\",\"123456789\"\r\n"); // 设置WIFI密码和账号
    Delay_s(5);
    char mqttUserCfg[100];
    snprintf(mqttUserCfg, sizeof(mqttUserCfg), "AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"\r\n", USERNAME, PASSWORD);
    Serial_SendString(mqttUserCfg); // 设置MQTT的username和password
    Delay_s(2);
    char mqttClientId[50];
    snprintf(mqttClientId, sizeof(mqttClientId), "AT+MQTTCLIENTID=0,\"%s\"\r\n", CLIENTID);
    Serial_SendString(mqttClientId); // 设置CLIENTID
    Delay_s(3);
    Serial_SendString("AT+MQTTCONN=0,\"iot-06z00dt2zbxttjh.mqtt.iothub.aliyuncs.com\",1883,1\r\n"); // 设置域名
    printf("AT+MQTTSUB=0,\"/%s/%s/user/get\",1\r\n",PRODUCTID,DEVICENAME);//订阅    
}