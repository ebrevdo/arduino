#include <LiquidCrystal.h>
#include <Servo.h>
#include <TimeAlarms.h>
#include <Time.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
const int buttonPin = A0; // analogue pin receiving button presses
time_t feedDelta = AlarmHMS(12, 0, 0); // 12 hours = 12*60*60 seconds
int menuResetDelta = 5;
AlarmId feedAlarm;
AlarmId menuResetAlarm;

#include "feedservo.h"
#include "buttons.h"
#include "menuitem.h"
#include "menu.h"

struct DHM {
  int hours;
  int mins;
  int secs;
};

void timeToDHM(time_t d, DHM &dhm) {
  dhm.hours = d / SECS_PER_HOUR;
  d -= dhm.hours * SECS_PER_HOUR;
  dhm.mins = d / SECS_PER_MIN;
  d -= dhm.mins * SECS_PER_MIN;
  dhm.secs = d;
}

void reset_feed_timer() {
  Alarm.free(feedAlarm);
  feedAlarm = Alarm.timerRepeat(feedDelta, FeedServo::feed_trigger);
  Serial.println("new feed alarm: " + String(feedAlarm));
}

class NextFeedMenuItem : public MenuItem {
 public:
  NextFeedMenuItem() : MenuItem("Next Feed In:") {}
  virtual String info() {
    time_t tnow = now();
    time_t tnext = Alarm.getAlarm(feedAlarm)->nextTrigger;
    time_t delta = tnext - tnow;
    DHM d;
    timeToDHM(delta, d);
    char buf[16];
    sprintf(buf, "%02dh %02dm %02ds",
            d.hours, d.mins, d.secs);
    return buf;
  }
};

class FeedNowMenuItem : public MenuItem {
 public:
  FeedNowMenuItem() : MenuItem("Feed Now") {}
  virtual void select() {
    FeedServo::feed_now();
  }
};

class ResetFeedMenuItem : public MenuItem {
 public:
  ResetFeedMenuItem() : MenuItem("Reset Timer") {}
  virtual void select() {
    reset_feed_timer();
  }
};

class CancelNextFeedMenuItem : public MenuItem {
 public:
  CancelNextFeedMenuItem() : MenuItem("Cancel Next Feed") {}
  virtual void left() {
    FeedServo::cancelled = ! FeedServo::cancelled;
  }
  virtual void right() {
    FeedServo::cancelled = ! FeedServo::cancelled;
  }
  virtual String info() {
    if (FeedServo::cancelled)
      return "[Cancelled]";
    else
      return "[Not Cancelled]";
  }
};

class ChangeFeedDelta : public MenuItem {
 public:
  ChangeFeedDelta() : MenuItem("Feed Delta") {}
  virtual void left() {
    // Can't set the timer for less than 30 min
    if (feedDelta <= 30*SECS_PER_MIN)
      return;

    feedDelta -= 30*SECS_PER_MIN;
    reset_feed_timer();
  }
  virtual void right() {
    // Can't set the timer for more than 1 day
    if (feedDelta >= 1*SECS_PER_DAY)
      return;

    feedDelta += 30*SECS_PER_MIN;
    reset_feed_timer();
  }
  virtual String info() {
    DHM d;
    timeToDHM(feedDelta, d);
    char buf[16];
    sprintf(buf, "%02dh %02dm %02ds",
            d.hours, d.mins, d.secs);
    return buf;
  }
};

Buttons b;
MenuItem* mitems[5] = {
  new NextFeedMenuItem(),
  new FeedNowMenuItem(),
  new ResetFeedMenuItem(),
  new CancelNextFeedMenuItem(),
  new ChangeFeedDelta()
};

Menu menu(mitems, 5);

void menu_reset() { menu.set(0); }
void restart_menu_reset_timer() {
  Alarm.free(menuResetAlarm);
  menuResetAlarm = Alarm.timerRepeat(menuResetDelta, menu_reset);
}

void setup()
{
  Serial.begin(9600);
  FeedServo::initialize();
  lcd.begin(16, 2);
  feedAlarm = Alarm.timerRepeat(feedDelta, FeedServo::feed_trigger);
  //menuResetAlarm = Alarm.timerRepeat(menuResetDelta, menu_reset);

  menu.showCurrent();
}

void loop() {
  int buttonRead = analogRead(buttonPin);

  switch (b.type(buttonRead)) {
    case Buttons::UP:
      menu.dec();
      break;
    case Buttons::DOWN:
      menu.inc();
      break;
    case Buttons::SELECT:
      menu.callSelect();
      break;
    case Buttons::LEFT:
      menu.callLeft();
      break;
    case Buttons::RIGHT:
      menu.callRight();
      break;
    default:
      break;
  }

  if (b.type(buttonRead) != Buttons::NONE) {
    Serial.println("bnum: " + String(buttonRead) + " button type: " + b.toString(b.type(buttonRead)));
    //restart_menu_reset_timer();
    Alarm.delay(300);
  } else {
    Alarm.delay(0);
  }
}
