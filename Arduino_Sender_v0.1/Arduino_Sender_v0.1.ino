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
int y_dir = 1;
int x;
bool sudo = false;
String prev_transmit;
long int joy_still_x;
long int joy_still_y;

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
  if (!sudo) {
    joyX = analogRead(JOY_X);
    joyY = analogRead(JOY_Y);
    // reduce measurements so that the motor turns within the limits of the hardware
    // the multiplying by 100 is done so that I don't have to deal with float to int conversions
    joyX = joyX - 512;
    joyX = floor(joyX / 3.2);
    joyY = joyY - 512;
    joyY =  -1 * joyY;
  }

  if (abs(prevY - joyY) > 40 && !sudo) {
    // check if the y-value of the joystick has been changed significantly
    prevY = joyY;
    if (joyY > 250 && y_dir != 2) {
      y_dir = 2;
    } else if (joyY < -250 && y_dir != 0) {
      y_dir = 0;
    } else if (abs(joyY) <= 250 && y_dir != 1) {
      y_dir = 1;
    }
    joy_still_y = millis();
  }
  if (abs(prevX - joyX) > 40 && !sudo) {
    // check if the x-value of the joystick has been changed significantly
    prevX = joyX;
    x = joyX;
    joy_still_x = millis();
  }
  to_transmit = String(y_dir) + String(x);
  if (to_transmit != prev_transmit && (millis() - joy_still_x) >= 20 && (millis() - joy_still_y) >= 20) {
    // if x and/or y has changed significantly, send intructions to the car
    BTSerial.println(to_transmit);
    // Serial.println("^ " + to_transmit);
    joy_still_x = false;
    joy_still_y = false;
    prev_transmit = to_transmit;
    digitalWrite(LED, HIGH);
    delay(30);
    digitalWrite(LED, LOW);
    delay(20);
  } // send instructions for x and y in one string
  if (Serial.available()) {
    // if user sends input through the serial monitor...
    received = Serial.readString();
    received.trim(); // remove newline characters
    Serial.println("^ " + received);
    if (received == "91" || received == "sudo") {
      // toggle sudo mode
      if (sudo) {
        sudo = false;
        Serial.println("Sudo disabled");
      } else {
        sudo = true;
        Serial.println("Sudo enabled");
      }
    } else {
      // forward Serial input to car
      BTSerial.print(received);
    }
  }
  if (BTSerial.available()) {
    // check if car has responded
    received = BTSerial.readString();
    if (received.substring(0,1) == "x" && !sudo) {
      // confirm receiver has received the correct instructions
      int car_y = received.substring(1,2).toInt();
      int car_x = received.substring(2).toInt();
      Serial.println("x: " + String(x) + ", y: " + String(y_dir));
      if (car_y != y_dir || car_x != x) {
        BTSerial.println(String(y_dir) + String(x));
        Serial.println(String(y_dir) + String(y_dir));
      }
    }
    Serial.print("v " + received);
  }
  if ((digitalRead(BT_STATE) == 1) && !bt_connected) {
    // check bluetooth connected and notify user
    bt_connected = true;
    Serial.println("Bluetooth connected");
  } else if ((digitalRead(BT_STATE) == 0) && bt_connected) {
    bt_connected = false;
    Serial.println("Bluetooth disconnected");
  }
}
