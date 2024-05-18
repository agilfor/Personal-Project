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
 * --------------------------------------------------------------------------------------------------
 *  Pin on    | Corresponds | Pin on  | Corresponds  | Pin on  | Corresponds | Pin on  | Corresponds 
 *  Arduino   | to:         | Arduino | to:          | Arduino | to:         | Arduino | to:         
 * -----------|-------------|---------|--------------|---------|-------------|---------|-------------
 *   +5V      | HC-05 VCC   |    2    | TBD          |    6    | TBD         |    10   | Stepper 1   
 *   GND      | HC-05 GND   |    3    | TBD          |    7    | TBD         |    11   | Stepper 2   
 *   RX (0)   | HC-05 TXD   |    4    | TBD          |    8    | TBD         |    12   | Stepper 3   
 *   TX (1)   | HC-05 RXD   |    5    | TBD          |    9    | TBD         |    13   | Stepper 4   
 */


/* PINS */
// Define DC motors transistor
#define MOTOR_LF 2 
#define MOTOR_LB 3
#define MOTOR_RF 4
#define MOTOR_RB 5

// Define pins for directional stepper motor
#define STEPPER_1 6
#define STEPPER_2 7
#define STEPPER_3 8
#define STEPPER_4 9

#define LED 13

/* VARIABLES */
String received; // store bluetooth received data
int key; // store first value of received array
int value; // store second value of received array
int x = 0; // direction of front wheels
int x_change = 0;
int y = 0;
int new_y = 0;
int motor_r = 0;
int motor_l = 0;
bool bt_connected = 0;
bool shut_down = false;

void make_step(bool a, bool b, bool c, bool d) {
  if (a) { digitalWrite(STEPPER_1, HIGH); } else { digitalWrite(STEPPER_1, LOW); }
  if (b) { digitalWrite(STEPPER_2, HIGH); } else { digitalWrite(STEPPER_2, LOW); }
  if (c) { digitalWrite(STEPPER_3, HIGH); } else { digitalWrite(STEPPER_3, LOW); }
  if (d) { digitalWrite(STEPPER_4, HIGH); } else { digitalWrite(STEPPER_4, LOW); }
}

void setup() {
  pinMode(MOTOR_LF, OUTPUT);
  pinMode(MOTOR_LB, OUTPUT);
  pinMode(MOTOR_RF, OUTPUT);
  pinMode(MOTOR_RB, OUTPUT);
  pinMode(STEPPER_1, OUTPUT);
  pinMode(STEPPER_2, OUTPUT);
  pinMode(STEPPER_3, OUTPUT);
  pinMode(STEPPER_4, OUTPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  make_step(false, false, false, false);
  // y_direction(0);

  // start communication
  Serial.begin(9600);
  Serial.println("Initiating...");
}

void loop() {
  if (Serial.available() && !shut_down) {
    digitalWrite(LED, HIGH);
    delay(20);
    digitalWrite(LED, LOW);
    received = Serial.readString();
    Serial.println(received);
    key = received.substring(0,1).toInt();
    value = received.substring(1).toInt();
    switch (key) {
      case 1:
        if (abs(x-value) > 2) {
          if (x_change != 0) {
            x = x - x_change;
          }
          x_change = x - value;
          x_change = x_change * -1;
          x = value;
        }
        break;

      case 2:
        new_y = value;
        break;

      case 0:
        if (value == 0) {
          Serial.println("Shutting down...");
          // y_direction(0);
          new_y = 0;
          digitalWrite(LED, HIGH);
          delay(1000);
          digitalWrite(LED, LOW);
          shut_down = true;
        }
        break;
    }
  }
  if (y != new_y) {
    // y_direction(new_y);
    if (new_y == 1) {
      digitalWrite(MOTOR_RF, HIGH);
      digitalWrite(MOTOR_LF, HIGH);
      digitalWrite(MOTOR_LB, LOW);
      digitalWrite(MOTOR_RB, LOW);
    } else if (new_y == -1) {
      digitalWrite(MOTOR_RF, LOW);
      digitalWrite(MOTOR_LF, LOW);
      digitalWrite(MOTOR_LB, HIGH);
      digitalWrite(MOTOR_RB, HIGH);
    } else {
      digitalWrite(MOTOR_RF, LOW);
      digitalWrite(MOTOR_LF, LOW);
      digitalWrite(MOTOR_LB, LOW);
      digitalWrite(MOTOR_RB, LOW);
    }
    y = new_y;
  }
  if (x_change != 0) {
    if (x_change > 0) {
      // Serial.println("Turning CW");
      make_step(true, false, false, false);
      delay(3);
      make_step(false, true, false, false);
      delay(3);
      make_step(false, false, true, false);
      delay(3);
      make_step(false, false, false, true);
      delay(3);
      x_change--;
    } else if (x_change < 0) {
      // Serial.println("Turning CCW");
      make_step(false, false, false, true);
      delay(3);
      make_step(false, false, true, false);
      delay(3);
      make_step(false, true, false, false);
      delay(3);
      make_step(true, false, false, false);
      delay(3);
      x_change++;
    }
  } else {
    make_step(false, false, false, false);
    if (shut_down) {
      // if shut down, return to starting position
      x_change = x * -1;
      x = 0;
    }
  }
}