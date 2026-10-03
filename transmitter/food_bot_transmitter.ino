/*
  RC Car Transmitter - NodeMCU (ESP8266)
  ------------------------------------------------
  Reads a single joystick (X = steering, Y = throttle) using the
  proven diode-based channel switching + calibration method,
  and sends the values wirelessly via ESP-NOW to the car's receiver.

  No WiFi network connection, no web server, no OTA - pure ESP-NOW only,
  so it stays on the same default WiFi channel as the receiver.

  IMPORTANT: receiverMAC[] below must match your receiver's MAC address.

  WIRING (same as your working joystick test):
    Pot X (steering): VCC -> D1 | GND -> GND | Wiper -> Diode Anode -> Common Node
    Pot Y (throttle):  VCC -> D2 | GND -> GND | Wiper -> Diode Anode -> Common Node
    Common Node -> A0
    Common Node -> 10k resistor -> GND
    (Diode cathode / silver band side faces the Common Node)
*/

#include <ESP8266WiFi.h>
#include <espnow.h>

// ---------- Receiver's MAC address ----------
uint8_t receiverMAC[] = {0x24, 0x4C, 0xAB, 0x6C, 0x5B, 0xD9};

// ---------- Pin setup ----------
#define POT_X_PWR D1  // steering
#define POT_Y_PWR D2  // throttle
#define ANALOG_PIN A0

int xValue = 512;
int yValue = 512;

// ---------- Calibration ----------
int xCenter = 512;
int yCenter = 512;
const int DEADZONE = 15;      // ignore tiny jitter around center
const int OUT_MIN = -100;     // final mapped output range
const int OUT_MAX = 100;

typedef struct {
  int throttle; // Y axis, -100 to 100
  int steering; // X axis, -100 to 100
} JoyData;

JoyData data;

// Read one channel: power it on, read A0, power it off
int readChannel(int pwrPin) {
  digitalWrite(pwrPin, HIGH);
  delayMicroseconds(300);   // let voltage settle through diode
  int val = analogRead(ANALOG_PIN);
  digitalWrite(pwrPin, LOW);
  return val;
}

// Take several readings at boot and average them to find resting center
void calibrateCenter() {
  long xSum = 0, ySum = 0;
  const int samples = 30;

  Serial.println("Calibrating center... don't touch the joystick.");
  for (int i = 0; i < samples; i++) {
    xSum += readChannel(POT_X_PWR);
    ySum += readChannel(POT_Y_PWR);
    delay(20);
  }
  xCenter = xSum / samples;
  yCenter = ySum / samples;

  Serial.printf("Calibration done. X center: %d  Y center: %d\n", xCenter, yCenter);
}

// Convert raw reading -> centered, deadzoned, mapped value
int applyCalibration(int raw, int center) {
  int diff = raw - center;

  if (abs(diff) < DEADZONE) {
    return 0; // inside deadzone, treat as centered
  }

  if (diff > 0) {
    return map(diff, DEADZONE, 1023 - center, 0, OUT_MAX);
  } else {
    return map(diff, -DEADZONE, -center, 0, OUT_MIN);
  }
}

void onDataSent(uint8_t *mac_addr, uint8_t status) {
  // Optional: uncomment to debug send status
  // Serial.println(status == 0 ? "Send OK" : "Send FAIL");
}

void setup() {
  Serial.begin(115200);

  pinMode(POT_X_PWR, OUTPUT);
  pinMode(POT_Y_PWR, OUTPUT);
  digitalWrite(POT_X_PWR, LOW);
  digitalWrite(POT_Y_PWR, LOW);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(receiverMAC, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_register_send_cb(onDataSent);

  // ---- Calibrate resting center (don't touch joystick during this) ----
  calibrateCenter();

  Serial.println("Transmitter ready.");
}

void loop() {
  xValue = readChannel(POT_X_PWR);
  yValue = readChannel(POT_Y_PWR);

  data.steering = applyCalibration(xValue, xCenter);
  data.throttle = applyCalibration(yValue, yCenter);

  esp_now_send(receiverMAC, (uint8_t *)&data, sizeof(data));

  Serial.printf("Raw X:%d Y:%d | Center X:%d Y:%d | Throttle:%d Steering:%d\n",
    xValue, yValue, xCenter, yCenter, data.throttle, data.steering);

  delay(30); // ~30 updates/sec
}
