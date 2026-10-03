<img width="922" height="2048" alt="bot" src="https://github.com/user-attachments/assets/87fd750d-7b7d-412d-83d1-418f2020f31f" />
# Food Bot

A wireless, 2-board robot controlled by a joystick. A transmitter board reads a joystick and sends throttle/steering commands wirelessly (ESP-NOW) to a receiver board, which drives 4 DC motors through an L298N motor driver.


## Hardware

- 2x NodeMCU v3 (ESP8266)
- 1x joystick module (2x B502, 5k linear potentiometers  X and Y axes)
- 4x signal diodes (1N4148 or similar)
- 1x 10k resistor
- 1x L298N dual H-bridge motor driver
- 4x DC motors (wired as 2 parallel pairs  left side, right side)
- Motor battery pack (matched to motor voltage)

## How it works

- The **transmitter** reads the joystick's X (steering) and Y (throttle) potentiometers one at a time through a shared analog pin, using diodes to switch between them. It auto-calibrates the resting center at boot and sends calibrated values (-100 to 100) over **ESP-NOW**.
- The **receiver** gets those values, mixes them arcade-style (`left = throttle + steering`, `right = throttle - steering`), and drives the L298N accordingly. A failsafe stops the motors if signal is lost for more than 500ms.

## Wiring

### Transmitter  Joystick

| Joystick pin | NodeMCU pin |
|---|---|
| X pot VCC | D1 |
| X pot GND | GND |
| X pot Wiper | Diode anode |
| Y pot VCC | D2 |
| Y pot GND | GND |
| Y pot Wiper |  Diode anode |

All diode **cathodes** (silver band side) join at one common node:
- Common node  **A0**
- Common node 10k resistor  **GND**

### Receiver L298N Motor Driver

| L298N pin | NodeMCU pin |
|---|---|
| IN1 | D1 |
| IN2 | D2 |
| IN3 | D3 |
| IN4 | D4 |
| ENA (left speed, PWM) | D5 |
| ENB (right speed, PWM) | D6 |
| GND | GND (shared with NodeMCU) |
| +12V | Motor battery pack |

Left motor pair  L298N Motor A terminals. Right motor pair Motor B terminals.

**Remove any jumper caps on ENA/ENB** on the L298N board  they must be controlled by D5/D6 for speed control to work.

## Setup

1. Upload `utilities/get_mac_address.ino` to the **receiver** board. Open Serial Monitor (115200 baud), note the printed MAC address.
2. Open `transmitter/food_bot_transmitter.ino` and paste that MAC address into the `receiverMAC[]` array.
3. Upload `receiver/food_bot_receiver.ino` to the receiver board.
4. Upload `transmitter/food_bot_transmitter.ino` to the transmitter board.
5. Power both boards. Don't touch the joystick for the first second after power-up  that's the auto-calibration window.
6. Test with wheels off the ground first.
7. If the car curves instead of driving straight, adjust `LEFT_TRIM` / `RIGHT_TRIM`  in the receiver code â€” lower the value on whichever side is faster.

## Troubleshooting

- **Car moves without touching the joystick**  re-check calibration; make sure nothing touches the joystick during the first second of power-up.
- **Nothing responds at all** â†’ confirm the receiver's MAC address in the transmitter code is correct, and that both boards are powered.
- **One motor side doesn't move**  check L298N jumpers (ENA/ENB), wiring continuity, and try `utilities/test_left_motor.ino` to isolate a hardware fault from a code issue.
- **Car always drives forward at full power even with throttle at 0**  likely an ENA/ENB jumper cap still installed on the L298N, bypassing PWM speed control.
