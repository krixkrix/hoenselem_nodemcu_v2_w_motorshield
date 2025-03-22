#ifndef IFTTT_DATAMAKER_H
#define IFTTT_DATAMAKER_H

#include <ESP8266WiFi.h>
#include "WifiUtil.h"

const char* hostname = "script.google.com";
String webAppPath = "/macros/s/AKfycbwLtmuWB670MzO2I73ylLMlNiuXO5tbu13_FY6MD99QYL1Bk72zODJYBVTODsBZuUE4/exec";
const int port = 443;


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
    // WiFi client for HTTPS
    WiFiClientSecure client;
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
  checkWifi();
  int r = 20;  // retries

  // Set insecure mode (skips SSL certificate validation)
  client.setInsecure(); 
  
  while ((!client.connect(hostname, port)) && (r > 0)){
      delay(100);
      Serial.print("*");
      r--;
  }
  return r>0;
}

void DataToMaker::post(const char* event)
{
  checkWifi();
  compileData();

  // Send HTTP POST request
  client.println("POST " + webAppPath + " HTTP/1.1");
  client.println("Host: script.google.com");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(postData.length());
  client.println();
  client.println(postData);

  // Wait for response
  Serial.println("Data sent, waiting for response...");

  // Read the HTTP response
  int httpResponseCode = -1;
  String response = "";
  while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line.startsWith("HTTP/1.1")) {
          httpResponseCode = line.substring(9, 12).toInt();  // Extract status code
      }
      if (line == "\r") {
          break;  // End of headers
      }
  }

  // Read the response body
  while (client.available()) {
      response += client.readString();
  }

  Serial.print("HTTP Response Code: ");
  Serial.print(httpResponseCode);
  Serial.print(" ");

  if (httpResponseCode == 200) {
    Serial.println("OK");
  }
  else if (httpResponseCode == 302) {
    Serial.println("Redirected (OK)");
  }
  else {
    Serial.println(" FAILURE");
    Serial.println("Response Body: ");
    Serial.println(response);
    Serial.println("");
  }
  
  Serial.println("Closing connection.");
  client.stop();
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
