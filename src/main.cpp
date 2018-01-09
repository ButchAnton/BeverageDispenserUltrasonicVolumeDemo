#include <Arduino.h>
#include <stdio.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// WiFi Connection Manger

#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>          //https://github.com/esp8266/Arduino
#endif

//needed for library
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager


// N.B.: MUST INCREASE STACK SIZE!!!!  I USED 16384!!!!!
// /Users/butch/.platformio/packages/framework-arduinoespressif32/cores/esp32/main.cpp

// Sensor Configuration Information

#define TIME_OFFSET (0) // Zulu time
// Palo Alto
#if 1
String sensorID = F("EU24229486132943999");     // Palo Alto
String longitude = F("37.398776");              // Palo Alto
String latitude = F(" -122.146571");            // Palo Alto
#endif // Palo Alto
// Chicago
#if 0
String sensorID = F("EU24228998962899896");     // Chicago
String longitude = F("41.8533117");             // Chicago
String latitude = F("-87.6224929");             // Chicago
#endif // Chicago

const int triggerPin = 18;
const int echoPin = 19;

const double speedOfSound = 2.93866995797702; // mm per microsecond
const double heightOfColumn = 573.388650;  // height of the beverage dispenser in mm

// Wi-Fi network configuration

// const char* ssid     = "Butch_ASUS";     // your network SSID (name of wifi network)
// const char* password = "4085297774";     // your network password

// CNG server information

String dataServer = F("https://sap-connected-goods-ingestion-api-qa.cfapps.eu10.hana.ondemand.com/ConnectedGoods/v1/DeviceData");
String authServer = F("https://cng-qa1.authentication.eu10.hana.ondemand.com/oauth/token");

// Root CA certificate obtained from the server

const char* caCert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFSTCCBDGgAwIBAgIQaYeUGdnjYnB0nbvlncZoXjANBgkqhkiG9w0BAQsFADCB\n" \
"vTELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n" \
"ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwOCBWZXJp\n" \
"U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MTgwNgYDVQQDEy9W\n" \
"ZXJpU2lnbiBVbml2ZXJzYWwgUm9vdCBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTAe\n" \
"Fw0xMzA0MDkwMDAwMDBaFw0yMzA0MDgyMzU5NTlaMIGEMQswCQYDVQQGEwJVUzEd\n" \
"MBsGA1UEChMUU3ltYW50ZWMgQ29ycG9yYXRpb24xHzAdBgNVBAsTFlN5bWFudGVj\n" \
"IFRydXN0IE5ldHdvcmsxNTAzBgNVBAMTLFN5bWFudGVjIENsYXNzIDMgU2VjdXJl\n" \
"IFNlcnZlciBTSEEyNTYgU1NMIENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n" \
"CgKCAQEAvjgWUYuA2+oOTezoP1zEfKJd7TuvpdaeEDUs48XlqN6Mhhcm5t4LUUos\n" \
"0PvRFFpy98nduIMcxkaMMSWRDlkXo9ATjJLBr4FUTrxiAp6qpxpX2MqmmXpwVk+Y\n" \
"By5LltBMOVO5YS87dnyOBZ6ZRNEDVHcpK1YqqmHkhC8SFTy914roCR5W8bUUrIqE\n" \
"zq54omAKU34TTBpAcA5SWf9aaC5MRhM7OQmCeAI1SSAIgrOxbIkPbh41JbAsJIPj\n" \
"xVAsukaQRYcNcv9dETjFkXbFLPsFKoKVoVlj49AmWM1nVjq633zS0jvY3hp6d+QM\n" \
"jAvrK8IisL1Vutm5VdEiesYCTj/DNQIDAQABo4IBejCCAXYwEgYDVR0TAQH/BAgw\n" \
"BgEB/wIBADA+BgNVHR8ENzA1MDOgMaAvhi1odHRwOi8vY3JsLndzLnN5bWFudGVj\n" \
"LmNvbS91bml2ZXJzYWwtcm9vdC5jcmwwDgYDVR0PAQH/BAQDAgEGMDcGCCsGAQUF\n" \
"BwEBBCswKTAnBggrBgEFBQcwAYYbaHR0cDovL29jc3Aud3Muc3ltYW50ZWMuY29t\n" \
"MGsGA1UdIARkMGIwYAYKYIZIAYb4RQEHNjBSMCYGCCsGAQUFBwIBFhpodHRwOi8v\n" \
"d3d3LnN5bWF1dGguY29tL2NwczAoBggrBgEFBQcCAjAcGhpodHRwOi8vd3d3LnN5\n" \
"bWF1dGguY29tL3JwYTAqBgNVHREEIzAhpB8wHTEbMBkGA1UEAxMSVmVyaVNpZ25N\n" \
"UEtJLTItMzczMB0GA1UdDgQWBBTbYiD7fQKJfNI7b8fkMmwFUh2tsTAfBgNVHSME\n" \
"GDAWgBS2d/ppSEefUxLVwuoHMnYH0ZcHGTANBgkqhkiG9w0BAQsFAAOCAQEAGcyV\n" \
"4i97SdBIkFP0B7EgRDVwFNVENzHv73DRLUzpLbBTkQFMVOd9m9o6/7fLFK0wD2ka\n" \
"KvC8zTXrSNy5h/3PsVr2Bdo8ZOYr5txzXprYDJvSl7Po+oeVU+GZrYjo+rwJTaLE\n" \
"ahsoOy3DIRXuFPqdmBDrnz7mJCRfehwFu5oxI1h5TOxtGBlNUR8IYb2RBQxanCb8\n" \
"C6UgJb9qGyv3AglyaYMyFMNgW379mjL6tJUOGvk7CaRUR5oMzjKv0SHMf9IG72AO\n" \
"Ym9vgRoXncjLKMziX24serTLR3x0aHtIcQKcIwnzWq5fQi5fK1ktUojljQuzqGH5\n" \
"S5tV1tqxkju/w5v5LA==\n" \
"-----END CERTIFICATE-----\n";

String oauthToken;

// NTP configuration Information

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// Post the level of the beverage dispenser to CNG as a percentage of full.
// Send along some approximate, pre-configured values for longitude and latitude.
// Fake the values for flavo(u)r and temperature.

void postLevelPercentage(float levelPercentage) {
  HTTPClient http;

  String response = "";
  int httpResponseCode = -9999;
  int returnCode = 0;

  // Get the time in this format: 2017-10-24T21:54:22.261Z

  time_t time_now;
  struct tm time_now_struct;
  char formatted_time[80];

  timeClient.update();
  time_now = timeClient.getEpochTime();
  time_now_struct = *localtime(&time_now);
  // Serial.printf("time_now (epoch time) is %ld\n", time_now);
  strftime(formatted_time, sizeof(formatted_time), "%Y-%m-%dT%H:%M:%SZ", &time_now_struct);
  // Serial.printf("The time is %s\n", formatted_time);


  http.begin(dataServer, caCert);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", oauthToken);
  http.addHeader("cng_devicetype", "CONNECTED_CONTAINER");
  http.addHeader("cng_messagetype", "Container_MessageData");

  String request = "{\"messages\":[{\"cng_deviceId\":\"" + sensorID + "\",\"timestamp\":\"" + formatted_time + "\",\"flavour\":1,\"fillLevel\":" + levelPercentage + ",\"longitude\":" + longitude + ",\"latitude\":" + latitude + ",\"temperature\":18.1}]}";

  // log_d("Request: %s\n", request.c_str());
  Serial.printf("Request: %s\n", request.c_str());
  returnCode = http.POST(request);
  if (returnCode == HTTP_CODE_OK || returnCode == HTTP_CODE_ACCEPTED) {
    response = http.getString();
    Serial.printf("Response: %s\n", response.c_str());
  } else {
    Serial.printf("Error posting sensor data, returnCode = %d\n", returnCode);
  }
}

// Retrieve an OAuth token from SAP CNG for use with POST requests.

void getAuthToken() {

  String response = "";
  int returnCode = 0;
  HTTPClient http;

  http.begin(authServer, caCert);

  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String request = F("client_id=sb-sap-connected-goods-qa!t5&client_secret=Eu7gZ7suJG4VRahheB2i3F2E85E=&grant_type=client_credentials&token_format=jwt&response_type=token");

  // Serial.printf("Before calling POST\n");
  returnCode = http.POST(request);
  // Serial.printf("After calling POST\n");
  // returnCode = HTTP_CODE_OK;

  if (returnCode == HTTP_CODE_OK) {
    // Serial.printf("Setting response\n");
    response = http.getString();
    // response = "{\"access_token\":\"abcde\"}";
    // Serial.printf("allocating jsonBuffer<10>\n");
    StaticJsonBuffer<3100> jsonBuffer;
    // StaticJsonBuffer<100> jsonBuffer;
    // Serial.printf("calling parseObject\n");
    JsonObject& root = jsonBuffer.parseObject(response);
    // Serial.printf("retrieving access_token\n");
    const char *token = root["access_token"];
    // Serial.printf("token = %s\n", token);
    // Serial.printf("Creating oauthToken\n");
    oauthToken = "Bearer " + String(token);
    Serial.printf("oauthToken = %s\n", oauthToken.c_str());
  } else {
    Serial.printf("Error getting auth token, returnCode = %d\n", returnCode);
  }
}

// Read the distance sensor, determine the distance to the fluid, compute
// the percentage full of the dispenser, and return it.

float getFillPercentage() {

  long duration;
  double distance;
  float percentage;

  // Drop the trigger pin low for 2 microseconds

  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  // Send a 10 microsecond burst (high) on the trigger pin

  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // Read the echo pin, which is the round trip time in microseconds.

  duration = pulseIn(echoPin, HIGH);

  // Calculate the distance in mm.  This is half of the duration
  // divided by the speed of sound in microseconds.

  distance = (duration / 2) / speedOfSound;
  percentage = (100.00 - ((distance / heightOfColumn) * 100.0));

  Serial.printf("Distance: %f mm, %f%%\n", distance, percentage);

  return(percentage);
}

String getCNGFormattedTime() {

}

void setup() {

  Serial.begin(115200);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Connect Wi-Fi

  delay(10000);
  Serial.printf("Entering Setup ...\n\n\n");

  WiFiManager wifiManager;
  wifiManager.autoConnect("SAPSensor", "SAPSensor");

  Serial.printf(" Connected to %s\n", wifiManager.getSSID());

  timeClient.begin();
  timeClient.setTimeOffset(TIME_OFFSET);
  timeClient.update();
}

void loop() {

  getAuthToken();
  Serial.printf("oauthToken: %s\n", oauthToken.c_str());

  postLevelPercentage(getFillPercentage());
  // postLevelPercentage(33.48);
  delay(10000); // wait 10 seconds
}
