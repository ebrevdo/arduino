#include <Servo.h>
#include <TimeAlarms.h>
#include <Time.h>

namespace FeedServo {
    Servo servo = Servo();
    int servoNeutral = 85; // 90 == servo is neutral (not rotating)
    const int pin = 7; // digital pin controlling the servo
    const int servoForward = 70; // 0 == fastest reverse
    const int servoReverse = 115; // 180 == fastest forward
    const int waitServoReverse = 2; // Time until servo goes backward
    const int waitLapse = 5; // Time until servo stops (should be > waitServoReverse)
    const int servoRepeats = 4; // How many times the servo will repeat its forward-backward motion

    boolean feedingNow = false; // Servo control during feeding
    boolean cancelled = false; // Is the next feed cancelled?

    void neutral() { servo.write(servoNeutral); }
    void forward() { servo.write(servoForward); }
    void reverse() { servo.write(servoReverse); }
    void feeding_stopped() {
        Serial.println(F("Feeding stopped."));
        feedingNow = false;
    }

    void feed_now() {
        if (feedingNow)
            return;

        feedingNow = true;

        Serial.println(F("Feeding now"));
        for (int i = 0; i < servoRepeats; i++) {
            Alarm.timerOnce(i*waitLapse + 1, forward);
            Alarm.timerOnce(i*waitLapse + 1 + waitServoReverse, reverse);
        }
        Alarm.timerOnce(servoRepeats*waitLapse + 1, neutral);
        Alarm.timerOnce(servoRepeats*waitLapse + 1, feeding_stopped);
    }

    void feed_trigger() {
        if (cancelled) {
            cancelled = false;
            return;
        }

        feed_now();
    }

    void setup() {
        servo.attach(pin);
        neutral();
    }
};

