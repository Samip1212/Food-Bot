/*
  Upload this to your RECEIVER NodeMCU first.
  Open Serial Monitor (115200 baud) to see its MAC address.
  Copy that MAC address - you'll need it in the transmitter sketch.
*/

#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  WiFi.mode(WIFI_STA);
  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {}
