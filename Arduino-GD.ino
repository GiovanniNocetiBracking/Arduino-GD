#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
const char *ssid = "KARIN.";
const char *password = "89428942";
const char *serverName = "http://192.168.0.105:3333/api/dashboard/arduinoSensor";
int sensorValue = 0;
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;
void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Netmask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
}
void loop()
{
  sensorValue = analogRead(A0);
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    String httpData = "{\"sensor\":\"" + String(sensorValue) + "\"}";
    int httpCode = http.POST(httpData);
    String payload = http.getString();
    Serial.print("Http code: ");
    Serial.println(httpCode);
    Serial.println(payload);
    http.end();
  }
  else
  {
    Serial.println("Error in WiFi connection");
  }
  delay(10000);
}