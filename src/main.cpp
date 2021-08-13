#include <Arduino.h>
#include <whclient.h>
using namespace whclient;

String ssid = "HomeAP";
String pass = "routerkiller";
String key = "2AoARJLzvXeoU8FaQMY5orZ0371AjzNp5g6s3-Sgqgk";
String id = "z97gl5b3";
source sources[3];
int LED = D5;
int LDR = A0;
int buzzer = D4;
double lightCallback(double value)
{
  // Serial.println("This is a callback function");
  Serial.println(value);
  analogWrite(LED, value);
  return 0;
}
double buzzCallback(double value)
{
  // Serial.println("This is a callback function");
  // Serial.println(value);
  digitalWrite(buzzer, value);
  return 0;
}

double tempCallback(double val)
{
  // return analogRead(LDR);
  return millis() / 100000;
}

void setup()
{
  //you dont need to setup serial Monitor
  pinMode(LED, OUTPUT);
  pinMode(LDR, INPUT);
  pinMode(buzzer, OUTPUT);
  wifiSetup(ssid, pass);
  sources[0].set("Light", OUTPUT, lightCallback);
  sources[1].set("light2", INPUT, tempCallback);
  sources[2].set("fan", OUTPUT, buzzCallback);
  begin(key, id, sources);
}

void loop()
{
  run();
}