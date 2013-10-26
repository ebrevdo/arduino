#include <Time.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Stream.h>
#include <SPI.h>
#include <stdarg.h>

namespace NTP {

    // NTP Servers:
    IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
    // IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
    // IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov

    //const int timeZone = 1;     // Central European Time
    //const int timeZone = -5;  // Eastern Standard Time (USA)
    //const int timeZone = -4;  // Eastern Daylight Time (USA)
    const int timeZone = -8;  // Pacific Standard Time (USA)
    //const int timeZone = -7;  // Pacific Daylight Time (USA)

    EthernetUDP ntpUDP;

    /*-------- NTP code ----------*/

    const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message

    // send an NTP request to the time server at the given address
    void sendNTPpacket(IPAddress &address)
    {
        byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
        // set all bytes in the buffer to 0
        memset(packetBuffer, 0, NTP_PACKET_SIZE);
        // Initialize values needed to form NTP request
        // (see URL above for details on the packets)
        packetBuffer[0] = 0b11100011;   // LI, Version, Mode
        packetBuffer[1] = 0;     // Stratum, or type of clock
        packetBuffer[2] = 6;     // Polling Interval
        packetBuffer[3] = 0xEC;  // Peer Clock Precision
        // 8 bytes of zero for Root Delay & Root Dispersion
        packetBuffer[12]  = 49;
        packetBuffer[13]  = 0x4E;
        packetBuffer[14]  = 49;
        packetBuffer[15]  = 52;
        // all NTP fields have been given values, now
        // you can send a packet requesting a timestamp:
        ntpUDP.beginPacket(address, 123); //NTP requests are to port 123
        ntpUDP.write(packetBuffer, NTP_PACKET_SIZE);
        ntpUDP.endPacket();
    }

    time_t getNtpTime()
    {
        byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
        while (ntpUDP.parsePacket() > 0) ; // discard any previously received packets
        Serial.println("Transmit NTP Request");
        sendNTPpacket(timeServer);
        uint32_t beginWait = millis();
        while (millis() - beginWait < 1500) {
            int size = ntpUDP.parsePacket();
            if (size >= NTP_PACKET_SIZE) {
                ntpUDP.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
                unsigned long secsSince1900;
                // convert four bytes starting at location 40 to a long integer
                secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
                secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
                secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
                secsSince1900 |= (unsigned long)packetBuffer[43];
                return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
            }
        }
        return 0; // return 0 if unable to get the time
    }

    void p(Print& to, const char *fmt, ... ){
        char tmp[64]; // resulting string limited to 64 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(tmp, 64, fmt, args);
        va_end (args);
        to.print(tmp);
    }

    void setup(Print& serial) {
        // Assumes Ethernet has been set up with a MAC address already
        unsigned int localPort = 64234;  // local port to listen for UDP packets
        ntpUDP.begin(localPort);

        // Wait until time has been set
        serial.println(F("Waiting until NTP has synced"));
        while (timeStatus() != timeSet) {
            setSyncProvider(getNtpTime);
            delay(1000);
            serial.print(".");
        }
        time_t t_now = now();
        tmElements_t te;
        breakTime(t_now, te);
        p(serial, "Time set: %04d-%02d-%02d %02d:%02d:%02d",
            1970 + te.Year, te.Month, te.Day, te.Hour, te.Minute, te.Second);
        serial.println("");
    }
};
