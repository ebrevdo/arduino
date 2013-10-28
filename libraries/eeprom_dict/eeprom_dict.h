#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/crc16.h>
#include <limits.h>
#include <float.h>

#ifndef __EEPROM_DICTIONARY__
#define __EEPROM_DICTIONARY__

#define EEP_MAX_ADDR E2END
#define EEP_MAGIC 0xAC

// Maximum number of variables that can be stored
#define EEP_MAX_COUNT 16 // This number should fit in 8 bits

#define EEP_MAGIC_IX 0
#define EEP_COUNT_IX 1 // Magic is 1 byte
#define EEP_INDEX_IX 2 // Magic is 1 byte, count is 1 byte
#define EEP_NOT_FOUND -1


class EEPD {
 protected:
    struct EEPDVar {
        uint16_t hash;
        uint8_t size;
        byte *loc;
        uint16_t addr;
        void read() { EEPD::read(addr, size, loc); }
        void write() { EEPD::write(addr, size, loc); }
    };

    static void write(unsigned int addr, uint8_t size, byte *var) {
        if (addr + size > EEP_MAX_ADDR) {
            Serial.println(F("Cannot write variable: region outside EEPROM"));
            return;
        }
        for (int i=0; i<size; i++) {
            eeprom_write_byte((unsigned char *) (addr + i), (uint8_t) var[i]);
        }
    }

    static void read(unsigned int addr, uint8_t size, byte *var) {
        if (addr + size > EEP_MAX_ADDR) {
            Serial.println(F("Cannot read variable: region outside EEPROM"));
            for (int i = 0; i < size; i++)
                var[i] = (byte)0;
            return;
        }
        for (int i=0; i<size; i++) {
            var[i] = (byte)eeprom_read_byte((unsigned char *) (addr + i));
        }
    }

    static uint16_t hash(const String& s) {
        // Use a CRC-16 implementation
        uint16_t h = 0xffff;
        for (int i = 0; i < s.length(); i++)
            h = _crc16_update(h, (uint8_t) *(s.c_str() + i));
        return h;
    }

    static int check_magic(byte magic) {
        byte get_magic;
        read(EEP_MAGIC_IX, sizeof(byte), &get_magic);
        Serial.println(magic);
        Serial.println(get_magic);
        return (magic == get_magic);
    }

    void initialize_eeprom(byte magic) {
        write(EEP_MAGIC_IX, sizeof(byte), &magic);
        write(EEP_COUNT_IX, sizeof(uint8_t), &count);
    }

    void write_index() {
        for (int i = 0; i < count; i++) {
            write(EEP_INDEX_IX + i*3, sizeof(uint16_t), (byte *) &(index[i].hash));
            write(EEP_INDEX_IX + i*3 + 2, sizeof(uint8_t), (byte *) &(index[i].size));
        }
    }

    void write_values() {
        for (int i = 0; i < count; i++)
            index[i].write();
    }

    void read_values() {
        for (int i = 0; i < count; i++)
            index[i].read();
    }

    void update_offsets() {
        uint16_t offset = 1;
        for (int i = 0; i < count; i++) {
            offset -= index[i].size;
            index[i].addr = EEP_MAX_ADDR + offset;
        }
    }

    int check_index() {
        uint16_t h;
        uint8_t s;

        uint8_t eep_count;
        read(EEP_COUNT_IX, sizeof(uint8_t), &eep_count);

        if (count != eep_count)
            return false;

        for (int i = 0; i < count; i++) {
            read(EEP_INDEX_IX + i*3, sizeof(uint16_t), (byte *)&h);
            read(EEP_INDEX_IX + i*3 + 2, sizeof(uint8_t), (byte *)&s);
            if (h != index[i].hash || s != index[i].size) {
                return false;
            }
        }
        return count;
    }

    int index_of(uint16_t h) {
        for (int i = 0; i < count; i++) {
            if (h == index[i].hash)
                return i;
        }
        return EEP_NOT_FOUND;
    }

 protected:
    int initialized;
    uint8_t count;
    EEPDVar index[EEP_MAX_COUNT];

 public:
    EEPD() : initialized(false), count(0) {}

    void initialize() {
        update_offsets();
        // Step 1: check magic number and match index.  Otherwise write headers
        if (check_magic(EEP_MAGIC) && check_index()) {
            Serial.println("reading!");
            read_values();
        } else {
            Serial.println("initializing!");
            initialize_eeprom(EEP_MAGIC);
            write_index();
            write_values();
        }

        initialized = true;
    }

    int map(const String& s, int size, byte *loc) {
        if (initialized)
            return false;
        if (count >= EEP_MAX_COUNT)
            return false;

        uint16_t h = hash(s);
        Serial.print(s);
        Serial.print(" -- hash: ");
        Serial.println(h);

        if (index_of(h) != EEP_NOT_FOUND)
            return false;

        index[count].hash = h;
        index[count].size = size;
        index[count].loc = loc;
        index[count].addr = 0xFFFF;

        count += 1;

        return true;
    }

    void map(const String& s, int *loc) { map(s, sizeof(int), (byte *)loc); }
    void map(const String& s, unsigned int *loc) { map(s, sizeof(unsigned int), (byte *)loc); }
    void map(const String& s, long *loc) { map(s, sizeof(long), (byte *)loc); }
    void map(const String& s, unsigned long *loc) { map(s, sizeof(unsigned long), (byte *)loc); }
    void map(const String& s, char *loc) { map(s, sizeof(char), (byte *)loc); }
    void map(const String& s, byte *loc) { map(s, sizeof(byte), (byte *)loc); }
    void map(const String& s, float *loc) { map(s, sizeof(float), (byte *)loc); }
    void map(const String& s, double *loc) { map(s, sizeof(double), (byte *)loc); }

    int index_of(const String& s) {
        return index_of(hash(s));
    }

    int write(const String& s) {
        if (!initialized)
            return false;
        int ix = index_of(s);
        if (ix == EEP_NOT_FOUND)
            return false;
        index[ix].write();
        return true;
    }

    int read(const String& s) {
        if (!initialized)
            return false;
        int ix = index_of(s);
        if (ix == EEP_NOT_FOUND)
            return false;
        index[ix].read();
        return true;
    }

    // Utility functions
    static void reset_eeprom() {
        byte EMPTY = 0xFF;
        for (int i = 0; i < EEP_MAX_COUNT; i++)
            write(i, sizeof(byte), &EMPTY);
    }
};

EEPD EEPROMDict;

void test_eeprom() {
    EEPD::reset_eeprom();

    int v1;

    EEPROMDict.map("hi", &v1);
    EEPROMDict.map("hey", &v1);
    EEPROMDict.map("hi!", &v1);
    EEPROMDict.map("", &v1);
    EEPROMDict.map("bleh", &v1);
    EEPROMDict.map("asljkdlsadfj", &v1);
    EEPROMDict.initialize();

    /*
    EEPROMDict.write("hi", ULONG_MAX);
    unsigned long got;
    EEPROMDict.read("hi", &got);
    Serial.print("got: ");
    Serial.print(got);
    Serial.print(" ");
    Serial.println(ULONG_MAX);
    */

    /*
      EEPROMDict.write("hey", DBL_MAX);
      double git;
      EEPROMDict.read("hi", &git);
      Serial.print("git: ");
      Serial.print(git);
      Serial.print(" ");
      Serial.println(DBL_MAX);
    */
}

#endif
