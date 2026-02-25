# Mecanum Robot - Working Pin Map

## Status: FL, FR, BL working. BR needs L298N #3.

## Dead Pins on Mega
- Pin 3: Dead (crashes Mega)
- Pin 5: Dead (crashes Mega via L298N #2 Ch B)
- Pin 14/15: Dead (Serial3 not working)
- Pin 24, 25, 26, 27: Dead or stuck

## Motor 1 - Front Left (FL) — L298N #1 Channel A
| Function | Mega Pin | L298N #1 |
|----------|----------|----------|
| ENA      | Pin 8    | ENA      |
| IN1      | Pin 40   | IN1      |
| IN2      | Pin 41   | IN2      |
| Motor    |          | OUT1/OUT2|

## Motor 2 - Front Right (FR) — L298N #1 Channel B
| Function | Mega Pin | L298N #1 |
|----------|----------|----------|
| ENB      | Pin 2    | ENB      |
| IN3      | Pin 22   | IN3      |
| IN4      | Pin 23   | IN4      |
| Motor    |          | OUT3/OUT4|

## Motor 3 - Back Left (BL) — L298N #2 Channel A
| Function | Mega Pin | L298N #2 |
|----------|----------|----------|
| ENA      | Pin 4    | ENA      |
| IN1      | Pin 42   | IN1      |
| IN2      | Pin 36   | IN2      |
| Motor    |          | OUT1/OUT2|

## Motor 4 - Back Right (BR) — L298N #3 Channel A (NEW BOARD)
| Function | Mega Pin | L298N #3 |
|----------|----------|----------|
| ENA      | Pin 10   | ENA      |
| IN1      | Pin 44   | IN1      |
| IN2      | Pin 45   | IN2      |
| Motor    |          | OUT1/OUT2|

## HC-05 Bluetooth — Serial2
| Function | Mega Pin |
|----------|----------|
| TXD      | Pin 17 (RX2) |
| RXD      | Pin 16 (TX2) |
| VCC      | 5V       |
| GND      | GND      |
| Baud     | 9600     |

## Power
- 10V supply → L298N #1, #2, #3 (12V input)
- USB/RPi → Mega power
- All GNDs connected together
- Remove ALL ENA/ENB jumper caps

## L298N #2 Channel B — DEAD (do not use)
