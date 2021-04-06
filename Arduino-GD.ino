#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#define FIREBASE_HOST "https://gasdetect-fe60e-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "XKKqf7QjLzIDa5rzSwjNbpG5wjufnxbbtFtoM5dS"
#define WIFI_SSID "KARIN."
#define WIFI_PASSWORD "89428942"
FirebaseData firebaseData;
int n = 0;
int sensorValue = analogRead(A0);
String firesensor1 = String(sensorValue);
void setup()
{
  Serial.begin(9600);
  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  int n = 0;
}
void loop()
{
  Serial.println(sensorValue);
  Firebase.setString(firebaseData, "Sensor1/data", firesensor1);
}
