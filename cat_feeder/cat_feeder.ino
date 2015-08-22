#include <Servo.h>
#include <TimeAlarms.h>
#include <Time.h>
//#include <eeprom_dict.h>
#include <config_rest.h>
#include <rest_server.h>
#include <SPI.h>
#include <Ethernet.h>
#include <limits.h>

#include "feedservo.h"
#include "ntp.h"
#include "soft_reset.h"

byte ENET_MAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ENET_IP[] = { 192, 168, 2, 2 };

#define FEED_INTERVAL_HOURS 12
#define NUM_FEEDS 2
time_t feedTime = AlarmHMS(6, 0, 0); // Feed at 6am, 6pm
time_t feedDelta = AlarmHMS(FEED_INTERVAL_HOURS, 0, 0);
AlarmId feedAlarm[NUM_FEEDS];

EthernetServer server(80);
RestServer restServer = RestServer(Serial);

void set_feed_timer() {
    for (int i = 0; i < NUM_FEEDS; i++)
        Alarm.free(feedAlarm[i]);

    for (int i = 0; i < NUM_FEEDS; i++) {
        feedAlarm[i] = Alarm.alarmRepeat((time_t)(feedTime + i*feedDelta), FeedServo::feed_trigger);
        Serial.print(F("New feed alarm: "));
        Serial.print(String(i));
        Serial.print(F(" / "));
        Serial.println((int)feedAlarm[i]);
    }
}

struct Resource {
    virtual int read() { return 0; };
    virtual void write(int v) {};
    //resource_description_t desc;
};

struct FeedNow : Resource { virtual void write(int v) { if (v) { FeedServo::feed_now(); } } };

struct ServoNeutral : Resource {
    virtual int read() { return FeedServo::servoNeutral; }
    virtual void write(int v) {
        if (abs(v - FeedServo::servoNeutral) < 20) {
            FeedServo::servoNeutral = v;
            FeedServo::neutral();
        }
    }
};

struct FeedHour : Resource {
    virtual int read() {
        int h = numberOfHours(feedTime);
        return (h == 0) ? 12 : h;
    }
    virtual void write(int v) {
        int m = numberOfMinutes(feedTime);
        int h = (v == 12) ? 0 : v;
        feedTime = AlarmHMS(h, m, 0);
        set_feed_timer();
    }
};

struct FeedMinute : Resource {
    virtual int read() {
        return numberOfMinutes(feedTime);
    }
    virtual void write(int v) {
        int h = numberOfHours(feedTime);
        feedTime = AlarmHMS(h, v, 0);
        set_feed_timer();
    }
};

struct SoftReset : Resource { virtual void write(int v) { if (v) soft_reset(); } };

struct TimeHour : Resource { virtual int read() { return hour(); } };
struct TimeMinute: Resource { virtual int read() { return minute(); } };
struct TimeSecond : Resource { virtual int read() { return second(); } };


#define RESOURCE_COUNT 8

Resource *resource_interactions[RESOURCE_COUNT] = {
    new FeedNow(),
    new ServoNeutral(),
    new FeedHour(),
    new FeedMinute(),
    new SoftReset(),
    new TimeHour(),
    new TimeMinute(),
    new TimeSecond()
};

void setup_server() {
    server.begin();

    Serial.println(F("Started server"));

    resource_description_t resources[RESOURCE_COUNT] = {
        {"feed_now", true, {0, 1}},
        {"servo_neutral", true, {0, 180}},
        {"feed_hour", true, {1, 12}}, // 1am/pm, 2am/pm, ..., 12am/pm
        {"feed_minute", true, {0, 59}}, // minutes
        {"soft_reset", true, {0, 1}},
        {"t_hour", false, {0, 24}},
        {"t_min", false, {0, 60}},
        {"t_sec", false, {0, 60}}
    };
    restServer.register_resources(resources, RESOURCE_COUNT);
    // restServer.set_post_with_get(true);

    Serial.print(F("Registered with server: "));
    Serial.print(String(restServer.get_server_state()));
}

void setup_alarm() {
    set_feed_timer();
}

void setup_ethernet() {
    // Sprinkle some magic pixie dust. (disable SD SPI to fix bugs)
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);

    Ethernet.begin(ENET_MAC, ENET_IP);
    // Disable w5100 SPI
    digitalWrite(10, HIGH);
}

void setup()
{
    // Soft reset
    setup_soft_reset();

    // Serial port
    Serial.begin(9600);
    while (!Serial)
        ;

    // Ethernet
    Serial.println(F("Setting up ethernet"));
    setup_ethernet();

    // NTP
    Serial.println(F("Setting up NTP"));
    NTP::setup(Serial);

    // REST server
    setup_server();

    // Feeder servo
    Serial.println(F("Setting up FeedServo"));
    FeedServo::setup();

    // Setup the alarm
    setup_alarm();

    // test_eeprom()
}

void handle_resources(RestServer &serv) {
    for (int i = 0; i < RESOURCE_COUNT; i++) {
        if (serv.resource_updated(i))
            resource_interactions[i]->write(serv.resource_get_state(i));
        if (serv.resource_requested(i))
            serv.resource_set_state(i, resource_interactions[i]->read());
    }
}

void handle_server() {
    EthernetClient client = server.available();
    if (client) {
        while (client.connected()) {
            if (restServer.handle_requests(client)) {
                handle_resources(restServer);
                restServer.respond();
            }
            if (restServer.handle_response(client))
                break;
        }
        Alarm.delay(1);
        client.stop();
    }
}

void loop() {
    handle_server();
    Alarm.delay(0); // Service any alarms
}
