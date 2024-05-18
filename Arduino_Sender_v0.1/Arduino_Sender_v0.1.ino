/* INDEX TABLE OF ARRAYS:
 * ----------------------------------------------------------------------------------------------------------------
 * First  | Corresponds | Second value  | 
 * value: | to:         | range:        | Other notes: 
 * -------|-------------|---------------|--------------------------------------------------------------------------
 *   1    | x-value     | -260 | 260    | Reduce input by 512. Use this in combination with x_steps variable
 *   2    | y-value     | -260 | 260    | Reduce input by 512. Use this for speed (possibly with increments)
 *   3    | TBD         |   |           | 1: forwards, 2: neutral, 3: backwards
 *   4    | TBD         |   |           | 
 *   5    | TBD         |   |           | 
 *   0    | Stop all    |    0          | Stop all. This will act as an emergency exit code that can be sent to
 *        |             |               | the vehical to stop all programs and shut it down remotely.
 */

/* PIN SETUP:
 * --------------------------------------------------------------------------------------------------------------------------
 *  Pin on    | Corresponds | Pin on  | Corresponds  | Pin on  | Corresponds | Pin on  | Corresponds | Pin on  | Corresponds 
 *  Arduino   | to:         | Arduino | to:          | Arduino | to:         | Arduino | to:         | Arduino | to:         
 * -----------|-------------|---------|--------------|---------|-------------|---------|-------------|---------|-------------
 *   +5V      | HC-05 VCC   |   A0    | Joystick X   |    2    | TBD         |    6    | TBD         |    10   | TBD      
 *   GND      | HC-05 GND   |   A1    | Joystick Y   |    3    | TBD         |    7    | BT state    |    11   | TBD      
 *   RX (0)   | HC-05 TXD   |   A2    | TBD          |    4    | TBD         |    8    | Stop btn    |    12   | TBD      
 *   TX (1)   | HC-05 RXD   |   A3    | TBD          |    5    | Jystck btn  |    9    | TBD         |    13   | TBD      
 */


#include <SoftwareSerial.h>
SoftwareSerial BTSerial(10, 11);


#define JOY_BTN 2
#define JOY_X A0
#define JOY_Y A1
#define BT_STATE 7
// #define STOP_BTN 8
#define LED 13

int joyX;
int joyY;
int prevX = 9;
int prevY = 0;
String received;
String to_transmit;
bool bt_connected;
int y_dir = 0;
bool y_changed = false;

void setup() {
  pinMode(JOY_BTN, INPUT);
  pinMode(BT_STATE, INPUT);
  pinMode(LED, OUTPUT);
  // pinMode(STOP_BTN, INPUT);
  Serial.begin(9600);
  BTSerial.begin(9600);
  Serial.println("Initiating...");
}

void loop() {
  joyX = analogRead(JOY_X);
  joyY = analogRead(JOY_Y);
  // reduce measurements so that the motor turns within the limits of the hardware
  // the multiplying by 100 is done so that I don't have to deal with float to int conversions
  joyX = joyX - 512;
  joyX = joyX / 2;
  joyY = joyY - 512;
  joyY =  -1 * joyY;

  if (abs(prevY - joyY) > 40) { 
    prevY = joyY;
    if (joyY > 250 && y_dir != 1) {
      to_transmit = "21";
      y_dir = 1;
      y_changed = true;
    } else if (joyY < -250 && y_dir != -1) {
      to_transmit = "2-1";
      y_dir = -1;
      y_changed = true;
    } else if (abs(joyY) <= 250 && y_dir != 0) {
      to_transmit = "20";
      y_dir = 0;
      y_changed = true;
    }
    if (y_changed) {
      // Serial.print(joyY);
      Serial.println("^ " + to_transmit);
      digitalWrite(LED, HIGH);
      BTSerial.println(to_transmit);
      delay(20);
      digitalWrite(LED, LOW);
      delay(500);
      y_changed = false;
    }
  }
  if (abs(prevX - joyX) > 30) {
    prevX = joyX;
    to_transmit = "1" + String(joyX);
    Serial.println("^ " + to_transmit);
    digitalWrite(LED, HIGH);
    BTSerial.println(to_transmit);
    delay(20);
    digitalWrite(LED, LOW);
    delay(500);
  }
  if (Serial.available()) {
    received = Serial.readString();
    BTSerial.print(received);
  }
  if (BTSerial.available()) {
    received = BTSerial.readString();
    if (received.substring(0,1) == "x") {
      // confirm receiver has received the correct instructions
      int car_y = received.substring(1,2).toInt() - 1;
      int car_x = received.substring(2).toInt();
      if (car_y != y_dir) {
        BTSerial.println("2" + String(y_dir));
        delay(50);
      }
      if (abs(car_x - joyX) > 30) {
        BTSerial.println("1" + String(joyX));
        delay(50);
      }
    }
    Serial.print("v " + received);
  }
  if ((digitalRead(BT_STATE) == 1) && !bt_connected) {
    bt_connected = true;
    Serial.println("Bluetooth connected");
  } else if ((digitalRead(BT_STATE) == 0) && bt_connected) {
    bt_connected = false;
    Serial.println("Bluetooth disconnected");
  }
}
