#include <Servo.h>
#include <TimeAlarms.h>
#include <Time.h>

struct FeedServo {
  static Servo servo;
  static const int pin = 7; // digital pin controlling the servo
  static const int servoNeutral = 89; // 90 == servo is neutral (not rotating)
  static const int servoForward = 70; // 0 == fastest reverse
  static const int servoReverse = 115; // 180 == fastest forward
  static const int waitServoReverse = 5; // Time until servo goes backward
  static const int waitLapse = 7; // Time until servo stops (should be > waitServoReverse)
  static const int servoRepeats = 2; // How many times the servo will repeat its forward-backward motion

  static boolean feedingNow; // Servo control during feeding
  static boolean cancelled; // Is the next feed cancelled?

  static void initialize() {
    servo.attach(pin);
    neutral();
  }

  static void neutral() { servo.write(servoNeutral); }
  static void forward() { servo.write(servoForward); }
  static void reverse() { servo.write(servoReverse); }
  static void feeding_stopped() { feedingNow = false; }

  static void feed_trigger() {
    if (cancelled) {
      cancelled = false;
      return;
    }

    feed_now();
  }

  static void feed_now() {
    if (feedingNow)
      return;

    feedingNow = true;

    Serial.println("Feeding now");
    for (int i = 0; i < servoRepeats; i++) {
      Alarm.timerOnce(i*waitLapse + 1, forward);
      Alarm.timerOnce(i*waitLapse + 1 + waitServoReverse, reverse);
      Serial.println("servo forward in: " + String(i*waitLapse + 1));
      Serial.println("servo reverse in: " + String(i*waitLapse + 1 + waitServoReverse));
    }

    Serial.println("servo stop in: " + String(servoRepeats*waitLapse + 1));
    Alarm.timerOnce(servoRepeats*waitLapse + 1, neutral);
    Alarm.timerOnce(servoRepeats*waitLapse + 1, feeding_stopped);
  }
};
boolean FeedServo::feedingNow = false;
boolean FeedServo::cancelled = false;
Servo FeedServo::servo = Servo();
