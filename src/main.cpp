#include <Arduino.h>
#include <newklioEsp8266.h>

using namespace newklioEsp8266;

String ssid = "WIFI_SSID";   //router SSID
String pass = "WIFI_PASSWD"; // router password

//set authentication key find it in API section on Dashboard
String key = "API_KEY";

//set deviceID find it in devices section on dashboard
String id = "DEVICE_ID";

//declare sources array (important Note: array should be a global variable)
source Sources[1];

//set callback function for fetching values from cloud.
//this function will be called automatically with the help of callback functionality.
double lightcallback(double value)
{ //return type should be int and pass an int parameter.
    Serial.println("this is light Callback");
    Serial.println(value);
    //your code goes here.

    return 0; // if you are planning to upload data to server return the value that you want.
    // send 0 by default, this function will push data only if you use push function or set a callback interval.
}

void setup()
{

    //no need to call serial.begin(); istead use
    // wifiSetup(ssid, pass, 9600); by default baud rate is 9600

    wifiSetup(ssid, pass); //connect to AP

    //set source name, mode and callback function to your sources list.
    //if you set mode to input timer interval start autmatically. default=1000ms
    Sources[0].set("light1", OUTPUT, lightcallback); //ignore warning messages.

    //connect to broker
    begin(key, id, Sources); // specify auth key, device ID and sources array.
    // if you have your own fingerprint and url you can specify like this
    // begin(key, id, Sources, "fingerprint_goes_here", "wss://example.com/")
}

void loop()
{
    //avoid delays here, it can mess up with server communication, use interval callback function instead
    //start polling and interval function
    run();
}