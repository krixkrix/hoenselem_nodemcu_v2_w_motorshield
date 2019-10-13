#ifndef IFTTT_DATAMAKER_H
#define IFTTT_DATAMAKER_H

/**
 * Class to build and send a HTTP post request to ifttt webhook
 * copied from
 * https://github.com/mylob/ESP-To-IFTTT/blob/master/ESP8266_To_IFTTT/DataToMaker.h
 */


#include <ESP8266WiFi.h>

class DataToMaker
{
  public:
    DataToMaker(const char*); // constructor
    bool connect();
    bool setValue(int, String);
    void sendToMaker();
    void post(const char*);

  protected: // it is protected because the subclass needs access
    //to max distance!

  private:
    void compileData();
    WiFiClient client;
    const char* privateKey;
    String value1, value2, value3 = "";
    bool dataAvailable;
    String postData;
};

DataToMaker::DataToMaker(const char* _privateKey)
{
  privateKey = _privateKey;
}

bool DataToMaker::connect()
{
  int r = 20;  // retries
  while ((!client.connect("maker.ifttt.com", 80)) && (r > 0)){
      delay(100);
      Serial.print("*");
      r--;
  }
  return r>0;
}

void DataToMaker::post(const char* event)
{
  compileData();
  client.print(F("POST /trigger/"));
  client.print(event);
  client.print(F("/with/key/"));
  client.print(privateKey);
  client.println(F(" HTTP/1.1"));

  client.println(F("Host: maker.ifttt.com"));
  client.println(F("User-Agent: Arduino/1.0"));
  client.println(F("Connection: close"));
  if (dataAvailable)
  { // append json values if available
    client.println(F("Content-Type: application/json"));
    client.print(F("Content-Length: "));
    client.println(postData.length());
    client.println();
    client.println(postData);
  }
  else
    client.println();
}

bool DataToMaker::setValue(int valueToSet, String value)
{
  switch (valueToSet)
  {
    case 1:
      value1 = value;
      break;
    case 2:
      value2 = value;
      break;
    case 3:
      value3 = value;
      break;
    default:
      return false;
      break;
  }
  return true;
}

void DataToMaker::compileData()
{
  if (value1 != "" || value2 != "" || value3 != "")
  {
    dataAvailable = true;
    bool valueEntered = false;
    postData = "{";
    if (value1 != "")
    {
      postData.concat(F("\"value1\":\""));
      postData.concat(value1);
      valueEntered = true;
    }
    if (value2 != "")
    {
      if (valueEntered)
      {
        postData.concat(F("\","));
      }
      postData.concat(F("\"value2\":\""));
      postData.concat(value2);
      valueEntered = true;
    }
    if (value3 != "")
    {
      if (valueEntered)
      {
        postData.concat(F("\","));
      }
      postData.concat(F("\"value3\":\""));
      postData.concat(value3);
    }
    postData.concat(F("\"}"));
  }
  else dataAvailable = false;
}

#endif
