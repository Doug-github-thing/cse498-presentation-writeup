#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <functional>
#include <stdio.h>
#include <stdint.h>
#include <cstdint>
#include "BitFileIO.h"

using namespace std;

struct DataPoint {
    uint32_t timestamp;
    float temperature;
    float humidity;
    DataPoint (uint32_t timestamp=0, float temperature=0, float humidity=0)
        : timestamp(timestamp), temperature(temperature), humidity(humidity) {}
    bool operator== (const DataPoint& other) const {
        return (this->timestamp  == other.timestamp 
            && this->temperature == other.temperature
            && this->humidity    == other.humidity);
    }
};

void write_naive(string fname, uint32_t timestamp, float temp, float humidity) {
    ofstream fout(fname, std::ios::app);
    fout << timestamp << ' ' << temp << ' ' << humidity << '\n';
    fout.close();
}

vector<DataPoint> read_naive(string fname, int num_datapoints) {
    vector<DataPoint> read_data(num_datapoints);

    ifstream fin(fname);
    for (int i=0; i<num_datapoints; ++i) {
        fin >> read_data[i].timestamp >> read_data[i].temperature >> read_data[i].humidity;
    }
    fin.close();
    return read_data;
}

struct DataPointSmall {
    // Unix timestamp 1767225600 is Jan 01 2026 00:00:00 GMT+0000
    // That means we can subtract that from each of our data points
    // if we build our system after at date.
    //
    // In practice, these loggers are cheap and easily replaceable.
    // From the datasheet of the AHT20 sensor: 
    // "The humidity sensitivity level (MSL) is 1, according
    // to IPC/JEDEC J-STD-020 standard. Therefore, it is
    // recommended to use it within one year after delivery"
    //
    // This means we do not have to store data more than a few years after
    // the manufacture date of the board since it is not intended for 
    // long term use.
    //
    // Unix timestamp 2082758400 is Tue Jan 01 2036 00:00:00 GMT+0000
    // Supposing a 10 year lifespan of the module:
    // 2082758400 - 1767225600 = 1767225600 <- 28 bits
    //
    // Furthermore, this system logs once every 5 minutes, so each data
    // point represented in memory will be a multiple of 5 minutes (300sec)
    // so we can store number 300x smaller
    // 1767225600 / 300 = 5890752 <- 22 bits; 10 bit savings!
    unsigned int timestamp : 22;  // 22 bits for 10 year range, 5 minute increments
    unsigned int temperature: 10; // 10 bits for range 0 to 102.3 fahrenheit
    unsigned int humidity : 10;   // 10 bits for range 0 to 102.3 % RH
};

void write_struct(string fname, uint32_t timestamp, float temp, float humidity) {

    // Convert to compressed structure
    DataPointSmall point;
    point.timestamp   = (timestamp - 1767225600) / 300;
    point.temperature = (unsigned int)(temp * 10);
    point.humidity    = (unsigned int)(humidity * 10);

    uint64_t packed = ((uint64_t)point.timestamp   << 20) |
                      ((uint64_t)point.temperature << 10) |
                      ((uint64_t)point.humidity);

    // Write the 64 bit data as raw bytes
    ofstream fout(fname, std::ios::binary | std::ios::app);
    fout.write(reinterpret_cast<const char*>(&packed), sizeof(packed));
    fout.close();
}

vector<DataPoint> read_struct(string fname, int num_datapoints) {
    vector<DataPoint> read_data(num_datapoints);
    ifstream fin(fname, std::ios::binary);

    for (int i = 0; i < num_datapoints; ++i) {
        uint64_t packed;
        fin.read(reinterpret_cast<char*>(&packed), sizeof(packed));

        // Unpack bits
        uint32_t timestamp   = (packed >> 20) & ((1ULL << 22) - 1);
        uint32_t temperature = (packed >> 10) & ((1ULL << 10) - 1);
        uint32_t humidity    =  packed        & ((1ULL << 10) - 1);

        // Convert back to original units
        read_data[i].timestamp   = timestamp * 300 + 1767225600;
        read_data[i].temperature = temperature / 10.0f;
        read_data[i].humidity    = humidity / 10.0f;
    }

    return read_data;
}

void write_u16_at_start(const std::string& fname, uint16_t value) {
    std::fstream file(fname, std::ios::binary | std::ios::in | std::ios::out);

    // Swap to big endian for consistency with storage
    uint8_t bytes[2];
    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;

    file.seekp(0); // go to beginning
    file.write(reinterpret_cast<char*>(bytes), 2);
}

// Storage scheme:
// Don't store timestamps anymore. Store the first one once, and every
// subsquent one is assumed to be 5 minutes after the one before it.
//
// Avoid wasted padding bits between data primitives. 20 bits does not fit
// nicely into a data primitive. Use unsigned int primitives by bitshifting
// into them densely.
//
// First 16 bits are the address of the last bit written to
// Next 32 bits are the timestamp of the first data point
// All subsequent writes are dense pairs of 10 bit ints representing temp/humidity
void write_compressed(string fname, uint32_t timestamp, float temp, float humidity) {
    
    uint16_t start_offset = 16 + 32;

    // Read header
    std::ifstream fin(fname, std::ios::binary);
    uint8_t bytes[2] = {0,0};
    fin.read(reinterpret_cast<char*>(bytes), 2);
    fin.close();

    uint16_t last_bit_written = (bytes[0] << 8) | bytes[1];

    if (last_bit_written == 0) {
        // initialize file
        write_at_bit(fname, 0, start_offset, 16);
        write_at_bit(fname, 16, timestamp, 32);
        last_bit_written = start_offset;
    }

    uint32_t packed20 =
        ((uint32_t)(temp * 10) << 10) |
        ((uint32_t)(humidity * 10));

    // Append real data to bit location pointed to in the file
    write_at_bit(fname, last_bit_written, packed20, 20);
    // Update header    
    write_at_bit(fname, 0, last_bit_written+20, 16);
}

vector<DataPoint> read_compressed(string fname, int num_datapoints) {
    vector<DataPoint> out(num_datapoints);

    uint16_t total_bits = read_at_bit(fname, 0, 16);
    uint32_t initial_timestamp = read_at_bit(fname, 16, 32);

    for (int i = 0; i < num_datapoints; ++i) {
        uint32_t packed20 = read_at_bit(fname, 48 + i * 20, 20);

        uint16_t temp = (packed20 >> 10) & 0x3FF;
        uint16_t hum  = packed20 & 0x3FF;

        // Convert back to original units
        out[i].timestamp = initial_timestamp + i * 300;
        out[i].temperature = temp / 10.0f;
        out[i].humidity = hum / 10.0f;
    }

    return out;
}
