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
 *   +5V      | HC-05 VCC   |    2    | Motors F     |    6    | Stepper 1   |    10   | TBD
 *   GND      | HC-05 GND   |    3    | Motors B     |    7    | Stepper 2   |    11   | TBD
 *   RX (0)   | HC-05 TXD   |    4    | TrigPin F    |    8    | Stepper 3   |    12   | TBD
 *   TX (1)   | HC-05 RXD   |    5    | EchoPin F    |    9    | Stepper 4   |    13   | Signal LED
 */


/* PINS */
// Define DC motors
#define MOTORS_F 2 // green
#define MOTORS_B 3 // blue

// Define pins for directional stepper motor
#define STEPPER_1 6 // purple
#define STEPPER_2 7 // red
#define STEPPER_3 8 // orange
#define STEPPER_4 9 // yellow

#define LED 13

// define ultrasonic sensors
#define TRIG_F 4 // blue
#define ECHO_F 5 // brown

/* VARIABLES */
String received; // store bluetooth received data
int x = 0; // direction of front wheels
int x_change = 0; // temporary variable for change of x
int y = 0; // direction of car
bool shut_down = false; // emergency stop
int buffer[] = {0, 0}; // {y, x}
unsigned long lastSerialMillis = 0;  // Milliseconds since last serial check
const int serialCheckInterval = 10; // Check for data every 10 milliseconds
long front_distance;


void stop_stepper() {
  digitalWrite(STEPPER_1, LOW);
  digitalWrite(STEPPER_2, LOW);
  digitalWrite(STEPPER_3, LOW);
  digitalWrite(STEPPER_4, LOW);
  delay(3);
}

long front_dist() {
  // Trigger pulse
  digitalWrite(TRIG_F, HIGH);
  delayMicroseconds(2);
  digitalWrite(TRIG_F, LOW);

  // Measure echo pulse duration
  long echo_duration = pulseIn(ECHO_F, HIGH);

  // Calculate distance (speed of sound is 34300 cm/s)
  return echo_duration * 0.0343 / 2;
}

int rot(int a) {
  if (a == 8) { digitalWrite(STEPPER_1, HIGH); } else { digitalWrite(STEPPER_1, LOW); }
  if (a == 4) { digitalWrite(STEPPER_2, HIGH); } else { digitalWrite(STEPPER_2, LOW); }
  if (a == 2) { digitalWrite(STEPPER_3, HIGH); } else { digitalWrite(STEPPER_3, LOW); }
  if (a == 1) { digitalWrite(STEPPER_4, HIGH); } else { digitalWrite(STEPPER_4, LOW); }
  delay(3);
}

void setup() {
  // define  input/output pins
  pinMode(MOTORS_F, OUTPUT);
  pinMode(MOTORS_B, OUTPUT);
  pinMode(STEPPER_1, OUTPUT);
  pinMode(STEPPER_2, OUTPUT);
  pinMode(STEPPER_3, OUTPUT);
  pinMode(STEPPER_4, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(TRIG_F, OUTPUT);
  pinMode(ECHO_F, INPUT);

  // set initial state
  digitalWrite(LED, LOW);
  digitalWrite(MOTORS_F, LOW);
  digitalWrite(MOTORS_B, LOW);
  stop_stepper();

  // start communication
  Serial.begin(9600);
  // Serial.println("Initiating..."); // for testing
}


void writeToBuffer(String data) {
  // write received data to buffer
  buffer[0] = data.substring(0,1).toInt();
  buffer[1] = data.substring(1).toInt();
}


void loop() {
  if (front_distance < 13 && buffer[0] == 2) {
    buffer[0] = 1;
    Serial.println("Obstacle detected");
  }
  if (y != buffer[0]) {
    // check if forwards/backwards needs to be updated
     if (buffer[0] == 2) {
      digitalWrite(MOTORS_F, HIGH);
      digitalWrite(MOTORS_B, LOW);
      Serial.println("Forwards");
    } else if (buffer[0] == 0) {
      digitalWrite(MOTORS_F, LOW);
      digitalWrite(MOTORS_B, HIGH);
      Serial.println("Backwards");
    } else {
      digitalWrite(MOTORS_F, LOW);
      digitalWrite(MOTORS_B, LOW);
      if (front_distance >= 13) { Serial.println(" "); }
    }
    y = buffer[0];
  } else if (buffer[1] != x) {
    // check if direction needs to be updated
    // only happens if forwards/backwards motion has not been updated
    if (abs(x - buffer[1]) > 2) {
      if (x_change != 0) {
        x = x - x_change;
      }
      x_change = x - buffer[1];
      x_change = -1 * x_change;
      x = buffer[1];
    }
  }
  if (x_change != 0) {
    // update steering if required
    if (x_change > 0) {
      // Turning CW
      for (int i = 8; i >= 1; i = i >> 1) {
        rot(i);
      }
      x_change--;
    } else if (x_change < 0) {
      // Turning CCW
      for (int i = 1; i <= 8; i = i << 1) {
        rot(i);
      }
      x_change++;
    }
  } else {
    // turn off stepper motor if no steering adjustments are needed
    stop_stepper();
  }
  if (shut_down) {
    // if shut down, return to starting position and stop car
    digitalWrite(MOTORS_F, LOW);
    digitalWrite(MOTORS_B, LOW);
    x_change = x * -1;
    x = 0;
  }
  // check if there is input (happens at 10 millisecond intervals)
  unsigned long currentMillis = millis();
  if (currentMillis - lastSerialMillis >= serialCheckInterval) {
    front_distance = front_dist();
    if (Serial.available()) {
      received = Serial.readString(); // receive instructions
      if (received == "99") {
        // shutdown everything
        Serial.println("Shutting down...");
        digitalWrite(LED, HIGH);
        delay(1000);
        digitalWrite(LED, LOW);
        shut_down = true;
      } else if (received == "98" && shut_down) {
        // reboot car
        Serial.println("Rebooting...");
        digitalWrite(LED, HIGH);
        delay(1000);
        digitalWrite(LED, LOW);
        shut_down = false;
      }
      writeToBuffer(received); // write received instructions to buffer (to avoid overload)
      // send buffer to sender to check if instructions were received correctly
      Serial.println("x" + String(buffer[0]) + String(buffer[1]));
    }
    lastSerialMillis = currentMillis;
  }
// }
}
