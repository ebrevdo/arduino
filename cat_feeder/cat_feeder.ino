#include <Servo.h>
#include <TimeAlarms.h>
#include <Time.h>
#include <config_rest.h>
#include <rest_server.h>
#include <SPI.h>
#include <Ethernet.h>
#include <limits.h>

#include "feedservo.h"
#include "ntp.h"

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
    for (int i = 0; i < NUM_FEEDS; i++) {
        Alarm.free(feedAlarm[i]);
        Serial.println("New feed alarm: " + String(feedAlarm[i]));
        feedAlarm[i] = Alarm.alarmRepeat(feedTime + i*feedDelta, FeedServo::feed_trigger);
    }
}

struct ResourceInteraction {
    virtual int read() = 0;
    virtual void write(int v) = 0;
    //resource_description_t desc;
};

struct FeedNowInteraction : ResourceInteraction {
    //FeedNowInteraction() { desc = (resource_description_t){"feed_now", true, {0, 1}}; }
    virtual int read() { return 0; }
    virtual void write(int v) { FeedServo::feed_now(); }
};

struct ServoNeutralInteraction : ResourceInteraction {
    //ServoNeutralInteraction() { desc = (resource_description_t){"servo_neutral", true, {0, 180}}; }
    virtual int read() { return FeedServo::servoNeutral; }
    virtual void write(int v) {
        FeedServo::servoNeutral = v;
        FeedServo::neutral();
    }
};

struct FeedAMMinInteraction : ResourceInteraction {
    // 12AM-12PM = 720 minutes
    //FeedAMMinInteraction() { desc = (resource_description_t){"feed_am_min", true, {0, 720-1}}; }
    virtual int read() { return feedTime; }
    virtual void write(int v) {
        feedTime = v;
        set_feed_timer();
    }
};

#define RESOURCE_COUNT 3

ResourceInteraction *resource_interactions[RESOURCE_COUNT] = {
    new FeedNowInteraction(),
    new ServoNeutralInteraction(),
    new FeedAMMinInteraction()
};

void setup_server() {
    server.begin();

    resource_description_t resources[RESOURCE_COUNT] = {
        {"feed_now", true, {0, 1}},
        {"servo_neutral", true, {0, 180}},
        {"feed_am_min", true, {0, 720-1}} // 12AM-12PM = 720 minutes
    };
    restServer.register_resources(resources, RESOURCE_COUNT);
    restServer.set_post_with_get(true);
}

void setup_alarm() {
    set_feed_timer();
}

void setup_ethernet() {
    Ethernet.begin(ENET_MAC, ENET_IP);
}

void setup()
{
    // Serial port
    Serial.begin(9600);
    while (!Serial)
        ;

    // Ethernet
    Serial.println("Setting up ethernet");
    setup_ethernet();

    // NTP
    Serial.println("Setting up NTP");
    NTP::setup(Serial);

    // Feeder servo
    Serial.println("Setting up FeedServo");
    FeedServo::setup();
    //feedAlarm = Alarm.timerRepeat(feedDelta, FeedServo::feed_trigger);

    // Setup the alarm
    setup_alarm();

    // REST server
    setup_server();
}

void handle_resources(RestServer &serv) {
    for (int i = 0; i < RESOURCE_COUNT; i++) {
        if (serv.resource_requested(i)) {
            Serial.println("Resource requested: " + String(i));
            serv.resource_set_state(i, resource_interactions[i]->read());
        }
        if (serv.resource_updated(i)) {
            Serial.println("Resource written: " + String(i));
            resource_interactions[i]->write(serv.resource_get_state(i));
        }
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
        delay(1);
        client.stop();
    }
}

void loop() {
    handle_server();
}
