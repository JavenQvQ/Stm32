#ifndef __ESP8266_H
#define __ESP8266_H
#include "stm32f10x.h"
#include "Delay.h"
#include "Serial.h"



#define USERNAME   "mqtt_stm32&k1pc8KDkdWX" //用户名
#define PASSWORD   "f3f17f624a5f4d78f8bae2dc2ed1e02388edf9da8f7387900dfb9f15bb335a63" //密码
#define CLIENTID   "k1pc8KDkdWX.mqtt_stm32|securemode=2\\,signmethod=hmacsha256\\,timestamp=1725448975997|" //设备名称
#define PRODUCTID  "k1pc8KDkdWX" //产品ID
#define DOMAINNAME PRODUCTID"iot-06z00dt2zbxttjh.mqtt.iothub.aliyuncs.com" //域名
#define DEVICENAME "mqtt_stm32"

#endif