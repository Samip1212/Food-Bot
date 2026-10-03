/*
  RC Car Receiver - NodeMCU (ESP8266)
  ------------------------------------------------
  Receives throttle (Y) and steering (X) values via ESP-NOW
  from the transmitter, mixes them arcade-style, and drives
  a 2-channel L298N motor driver (4 motors, 2 in parallel per side).

  Pure ESP-NOW only - no WiFi network connection, no web dashboard.

  WIRING:
    L298N IN1 -> D1
    L298N IN2 -> D2
    L298N IN3 -> D3
    L298N IN4 -> D4
    L298N ENA -> D5  (PWM - left side speed)
    L298N ENB -> D6  (PWM - right side speed)
    L298N GND -> NodeMCU GND (must share common ground)
    L298N +12V -> motor battery pack
*/

#include <ESP8266WiFi.h>
#include <espnow.h>

// ---------- Motor driver pins ----------
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4
#define ENA D5   // Left side speed (PWM)
#define ENB D6   // Right side speed (PWM)

// ---------- Trim (compensate for motor speed mismatch) ----------
// If the car curves one way when going straight, reduce the trim value
// on the FASTER side (e.g. 0.85 = 85% power) until it drives straight.
// Start both at 1.0, then adjust only the side that's too fast.
const float LEFT_TRIM  = 1.0;
const float RIGHT_TRIM = 1.0;
typedef struct {
  int throttle; // -100 to 100 (Y axis)
  int steering; // -100 to 100 (X axis)
} JoyData;

JoyData incoming;

unsigned long lastReceiveTime = 0;
const unsigned long FAILSAFE_TIMEOUT = 500; // ms - stop car if signal lost

void setMotor(int side_IN_A, int side_IN_B, int enPin, int speedVal) {
  // speedVal: -255 to 255 (negative = reverse, positive = forward)
  speedVal = constrain(speedVal, -255, 255);

  if (speedVal == 0) {
    // True stop: both direction pins LOW, not just relying on PWM=0.
    // Important if your L298N's ENA/ENB has a jumper permanently tying
    // it HIGH - in that case direction pins alone control the motor.
    digitalWrite(side_IN_A, LOW);
    digitalWrite(side_IN_B, LOW);
    analogWrite(enPin, 0);
    return;
  }

  if (speedVal > 0) {
    digitalWrite(side_IN_A, HIGH);
    digitalWrite(side_IN_B, LOW);
  } else {
    digitalWrite(side_IN_A, LOW);
    digitalWrite(side_IN_B, HIGH);
    speedVal = -speedVal;
  }
  analogWrite(enPin, speedVal);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ESP-NOW receive callback
void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&incoming, incomingData, sizeof(incoming));
  lastReceiveTime = millis();

  // ---- Arcade mixing ----
  // throttle: -100 (full reverse) to 100 (full forward)
  // steering: -100 (full left) to 100 (full right)
  int left  = incoming.throttle + incoming.steering;
  int right = incoming.throttle - incoming.steering;

  // Map from joystick range (-100..100, but mixed can exceed) to PWM (-255..255)
  left  = map(constrain(left, -100, 100), -100, 100, -255, 255);
  right = map(constrain(right, -100, 100), -100, 100, -255, 255);

  // Apply trim to compensate for motor speed mismatch
  left  = (int)(left * LEFT_TRIM);
  right = (int)(right * RIGHT_TRIM);

  Serial.printf("Recv Throttle:%d Steering:%d | Left PWM:%d Right PWM:%d\n",
    incoming.throttle, incoming.steering, left, right);

  setMotor(IN1, IN2, ENA, left);   // left side pair
  setMotor(IN3, IN4, ENB, right);  // right side pair
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  stopMotors();

  WiFi.mode(WIFI_STA);
  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Receiver ready.");
}

void loop() {
  // Failsafe: if no data received recently, stop the car
  if (millis() - lastReceiveTime > FAILSAFE_TIMEOUT) {
    stopMotors();
  }
}
