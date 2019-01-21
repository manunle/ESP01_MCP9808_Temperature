#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_MCP9808.h"
#include "ESPBASE.h"

#define MAXTEMPCOUNT 50

WiFiClient espClient;
const byte mqttDebug = 1;
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
String verstr = "ESPTemperatureMon ver 1.2";
bool delaywake = false;
float lastValidTemp = 0.0;
unsigned long lasttemp = 0;
unsigned int tempcount = 0;
float validtemps[MAXTEMPCOUNT];

ESPBASE Esp;

void setup() 
{
  Serial.begin(115200);
  Esp.initialize();
    if (!tempsensor.begin()) 
    {
      Serial.println("Couldn't find MCP9808!");
    }
}

void loop() {
  Esp.loop();
  if(Esp.WIFI_connected)
  {
    if(tempsensor.sensoravailable)
    {
      if(temptime >= config.TempSeconds and config.TempSeconds != 0)
      {
        float tottemp = 0;
        for(int i=0;i<tempcount;i++)
        {
          tottemp = tottemp+validtemps[i];
        }
        float avgtemp = tottemp/tempcount;
        Esp.mqttSend("Temperature/"+config.DeviceName+"/value","Temperature:"+String(avgtemp),"");
//        Esp.mqttSend("Temperature/"+config.DeviceName+"/test","Test:"+String(validtemps[0]),"");
        tempcount = 0;
        temptime = 0;
      }
      if(lasttemp<millis()-2000)
      {
        lasttemp = millis();
//        unsigned long starttemp = millis();
        tempsensor.shutdown_wake(0);   // Don't remove this line! required before reading temp
//        delay(1);
        float c = tempsensor.readTempC();
        float f = c * 9.0 / 5.0 + 32;
//        Esp.mqttSend("Temperature/"+config.DeviceName+"/value","Temperature:"+String(f),":lastValidTemp:"+String(lastValidTemp));
        if(lastValidTemp == 0.0)
        {
          lastValidTemp = f;
        }
        else
        {
          if(f > lastValidTemp + 40.0 || f < lastValidTemp - 40.0)
          {
            Esp.mqttSend("Temperature/"+config.DeviceName+"/status","Throwing out Temperature:"+String(f),"");
            f = lastValidTemp;
          }
        }
        lastValidTemp = f;
        if(tempcount < MAXTEMPCOUNT-1)
        {
          validtemps[tempcount] = lastValidTemp;
          tempcount++;
          f = lastValidTemp;
        }
        Serial.print("Temp: "); Serial.print(c); Serial.print("*C\t"); 
        Serial.print(f); Serial.println("*F");
//        Esp.mqttSend("Temperature/"+config.DeviceName+"/value","Temperature:"+String(f),"");
        if((int) f > config.HighTempAlarm)
        {
          Esp.mqttSend("Temperature/"+config.DeviceName+"/status","High Temperature!! ",String(f));
        }
        if((int) f < config.LowTempAlarm)
        {
          Esp.mqttSend("Temperature/"+config.DeviceName,"Low Temperature!! ",String(f));
        }
        delaywake = true;
//        unsigned long stoptime = millis();
//        Esp.mqttSend("Temperature/"+config.DeviceName+"/value","It took ",String(stoptime-starttemp));
      }
//      if(delaywake)
//      {
//        delaywake = false;
//        Serial.println("Shutdown MCP9808.... ");
//        tempsensor.shutdown_wake(1); // shutdown MSP9808 - power consumption ~0.1 mikro Ampere
//      }    
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';
  
  String s_topic = String(topic);
  String s_payload = String(c_payload);
  
  if (mqttDebug) {
    Serial.print("MQTT in: ");
    Serial.print(s_topic);
    Serial.print(" = ");
    Serial.print(s_payload);
  }

  if (s_topic == "TestTopic") {
    Serial.println("Got set: ");
    Serial.println(s_payload);
    char dname[20];
    String DeviceName;
    char *p = c_payload;
    char *str;
    str = strtok_r(p,";",&p);
    strncpy(dname,str,20);
    Serial.print("device name: ");
    DeviceName = String(dname);
    Serial.println(DeviceName);
    if(DeviceName == String(config.DeviceName))
    {
      Serial.println("This is for me");
    }
  }
  else {
    if (mqttDebug) { Serial.println(" [unknown message]"); }
  }
}

void mqttSubscribe()
{
    if (Esp.mqttClient->connected()) 
    {
      if (Esp.mqttClient->subscribe("TestTopic")) 
      {
        Esp.mqttSend(String("TestChip"),config.DeviceName,String(": subscribed"));
        Esp.mqttSend(HeartbeatTopic,verstr,","+Esp.MyIP() + " start");
      }
    }
}
