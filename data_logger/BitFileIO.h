#include <fstream>
#include <cstdint>

class BitWriter {
    std::ofstream out;
    uint8_t buffer = 0;
    int bit_pos = 0; // number of bits currently in buffer (0–7)

public:
    BitWriter(const std::string& fname)
        : out(fname, std::ios::binary | std::ios::app) {}

    // Write lowest `bits` bits of value
    void write_bits(uint32_t value, int bits) {
        for (int i = bits - 1; i >= 0; --i) {
            uint8_t bit = (value >> i) & 1;

            buffer = (buffer << 1) | bit;
            bit_pos++;

            if (bit_pos == 8) {
                flush_byte();
            }
        }
    }

    void flush_byte() {
        out.put(buffer);
        buffer = 0;
        bit_pos = 0;
    }

    // Call this when you're done writing
    void flush() {
        if (bit_pos > 0) {
            buffer <<= (8 - bit_pos); // pad remaining bits with 0
            flush_byte();
        }
    }

    ~BitWriter() {
        flush();
    }
};


class BitReader {
    std::ifstream in;
    uint8_t buffer = 0;
    int bit_pos = 8; // force initial refill

public:
    BitReader(const std::string& fname)
        : in(fname, std::ios::binary) {}

    uint32_t read_bits(int bits) {
        uint32_t value = 0;

        for (int i = 0; i < bits; ++i) {
            if (bit_pos == 8) {
                buffer = in.get();
                bit_pos = 0;
            }

            value = (value << 1) | ((buffer >> (7 - bit_pos)) & 1);
            bit_pos++;
        }

        return value;
    }
};
void write_at_bit(const std::string& fname, uint64_t bit_offset, uint32_t value, int bits) {
    // Ensure file exists
    {
        std::ofstream create(fname, std::ios::binary | std::ios::app);
    }

    std::fstream file(fname, std::ios::binary | std::ios::in | std::ios::out);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    for (int i = 0; i < bits; ++i) {
        uint64_t current_bit = bit_offset + i;

        uint64_t byte_index = current_bit / 8;
        int bit_index = 7 - (current_bit % 8);

        // --- READ BYTE (or treat as 0 if past EOF) ---
        uint8_t byte = 0;

        file.seekg(0, std::ios::end);
        uint64_t file_size = file.tellg();

        if (byte_index < file_size) {
            file.seekg(byte_index);
            file.read(reinterpret_cast<char*>(&byte), 1);
        }

        // --- MODIFY BIT ---
        uint8_t bit = (value >> (bits - 1 - i)) & 1;

        byte &= ~(1 << bit_index);
        byte |= (bit << bit_index);

        // --- WRITE BACK ---
        file.seekp(byte_index);
        file.write(reinterpret_cast<char*>(&byte), 1);
    }

    file.flush();
}

uint32_t read_at_bit(const std::string& fname, uint64_t bit_offset, int bits) {
    std::ifstream file(fname, std::ios::binary);

    uint32_t value = 0;

    for (int i = 0; i < bits; ++i) {
        uint64_t current_bit = bit_offset + i;

        uint64_t byte_index = current_bit / 8;
        int bit_index = 7 - (current_bit % 8);

        file.seekg(byte_index);
        uint8_t byte = file.get();

        uint8_t bit = (byte >> bit_index) & 1;
        value = (value << 1) | bit;
    }

    return value;
}