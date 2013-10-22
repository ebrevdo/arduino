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

time_t feedDelta = AlarmHMS(12, 0, 0); // 12 hours = 12*60*60 seconds
AlarmId feedAlarm;
AlarmId menuResetAlarm;

EthernetServer server(80);
RestServer restServer = RestServer(Serial);

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

void restart_menu_reset_timer() {
    Alarm.free(menuResetAlarm);
    //menuResetAlarm = Alarm.timerRepeat(menuResetDelta, menu_reset);
}

void setup_ethernet() {
    Ethernet.begin(ENET_MAC, ENET_IP);
}

void setup_server() {
    server.begin();

    resource_description_t resources[4] = {
        {"feed_now", true, {0, 1}},
        {"skip_feed", true, {0, 1}},
        {"servo_neutral", true, {0, 180}},
        {"feed_tod_sec", true, {INT_MIN, INT_MAX}}
    };
    restServer.register_resources(resources, 4);
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

    // REST server
    setup_server();
}

void handle_server() {
    EthernetClient client = server.available();
    if (client) {
        while (client.connected()) {
            if (restServer.handle_requests(client)) {
                // Handle stuff
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
