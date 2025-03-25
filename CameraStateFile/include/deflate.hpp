#ifndef DEFLATE_HPP
#define DEFLATE_HPP

#include <vector>
#include <array>
#include <cstdint>
#include <stdexcept>

namespace deflate {

/**
 * @brief Structure to hold base values and extra bits for length and distance codes.
 */
struct CodeInfo {
    uint16_t base_value; ///< Base value for the code
    uint8_t extra_bits;  ///< Number of extra bits following the code
};

/**
 * @brief Lookup table for length codes (257-285) as per RFC 1951.
 */
constexpr std::array<CodeInfo, 29> length_codes = {{
    {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0},    // 257-264
    {11, 1}, {13, 1}, {15, 1}, {17, 1},                                 // 265-268
    {19, 2}, {23, 2}, {27, 2}, {31, 2},                                 // 269-272
    {35, 3}, {43, 3}, {51, 3}, {59, 3},                                 // 273-276
    {67, 4}, {83, 4}, {99, 4}, {115, 4},                                // 277-280
    {131, 5}, {163, 5}, {195, 5}, {227, 5},                             // 281-284
    {258, 0}                                                    // 285
}};

/**
 * @brief Lookup table for distance codes (0-29) as per RFC 1951.
 */
constexpr std::array<CodeInfo, 30> distance_codes = {{
    {1, 0}, {2, 0}, {3, 0}, {4, 0},                                     // 0-3
    {5, 1}, {7, 1},                                             // 4-5
    {9, 2}, {13, 2},                                            // 6-7
    {17, 3}, {25, 3},                                           // 8-9
    {33, 4}, {49, 4},                                           // 10-11
    {65, 5}, {97, 5},                                           // 12-13
    {129, 6}, {193, 6},                                         // 14-15
    {257, 7}, {385, 7},                                         // 16-17
    {513, 8}, {769, 8},                                         // 18-19
    {1025, 9}, {1537, 9},                                       // 20-21
    {2049, 10}, {3073, 10},                                     // 22-23
    {4097, 11}, {6145, 11},                                     // 24-25
    {8193, 12}, {12289, 12},                                    // 26-27
    {16385, 13}, {24577, 13}                                    // 28-29
}};

/**
 * @brief Structure to hold a Huffman code and its bit length.
 */
struct HuffmanCode {
    uint32_t code;   ///< The Huffman code value
    uint8_t length;  ///< Number of bits in the code
};

/**
 * @brief Get the fixed Huffman code for a literal/length symbol (0-285).
 * @param symbol The symbol to encode (0-255 for literals, 256 for EOB, 257-285 for lengths).
 * @return HuffmanCode containing the code and its length.
 * @throws std::runtime_error if the symbol is invalid.
 */
inline HuffmanCode get_literal_length_code(uint16_t symbol) {
    if (symbol < 144) return {(uint32_t)48 + symbol, 8};                  // Literals 0-143: 8 bits
    else if (symbol < 256) return {(uint32_t)400 + (symbol - 144), 9};    // Literals 144-255: 9 bits
    else if (symbol == 256) return {0, 7};                      // End-of-block: 7 bits
    else if (symbol <= 279) return {symbol - (uint32_t)256, 7};           // Lengths 257-279: 7 bits
    else if (symbol <= 285) return {(uint32_t)192 + (symbol - (uint32_t)280), 8};   // Lengths 280-285: 8 bits
    else throw std::runtime_error("Invalid symbol");
}

/**
 * @brief Get the fixed Huffman code for a distance code (0-29).
 * @param distance_code The distance code to encode (0-29).
 * @return HuffmanCode containing the code and its length (always 5 bits).
 * @throws std::runtime_error if the distance code is invalid.
 */
inline HuffmanCode get_distance_code(uint16_t distance_code) {
    if (distance_code <= 29) return {distance_code, 5};
    else throw std::runtime_error("Invalid distance code");
}

/**
 * @brief Find the length code and extra bits for a given length.
 * @param length The length to encode (3-258).
 * @return Pair of {code, extra}, where code is 257-285 and extra is the extra bits value.
 * @throws std::runtime_error if the length is invalid.
 */
inline std::pair<uint16_t, uint16_t> find_length_code(uint16_t length) {
    if (length < 3 || length > 258) throw std::runtime_error("Invalid length");
    if (length == 258) return {285, 0};
    for (size_t i = 0; i < 28; ++i) {
        uint16_t base = length_codes[i].base_value;
        uint8_t extra_bits = length_codes[i].extra_bits;
        uint16_t max_length = base + (1 << extra_bits) - 1;
        if (length <= max_length) {
            uint16_t code = 257 + i;
            uint16_t extra = length - base;
            return {code, extra};
        }
    }
    throw std::runtime_error("Length code not found");
}

/**
 * @brief Find the distance code and extra bits for a given distance.
 * @param distance The distance to encode (1-32768).
 * @return Pair of {code, extra}, where code is 0-29 and extra is the extra bits value.
 * @throws std::runtime_error if the distance is invalid.
 */
inline std::pair<uint16_t, uint16_t> find_distance_code(uint32_t distance) {
    if (distance < 1 || distance > 32768) throw std::runtime_error("Invalid distance");
    for (size_t i = 0; i < 30; ++i) {
        uint16_t base = distance_codes[i].base_value;
        uint8_t extra_bits = distance_codes[i].extra_bits;
        uint32_t max_dist = base + (1 << extra_bits) - 1;
        if (distance <= max_dist) {
            uint16_t code = i;
            uint16_t extra = distance - base;
            return {code, extra};
        }
    }
    throw std::runtime_error("Distance code not found");
}

/**
 * @brief Class to write bits to a byte vector, LSB first.
 */
class BitWriter {
private:
    std::vector<uint8_t>& output; ///< Reference to the output byte vector
    uint32_t buffer = 0;          ///< Buffer to accumulate bits
    uint8_t bits_in_buffer = 0;   ///< Number of bits currently in the buffer

public:
    explicit BitWriter(std::vector<uint8_t>& out) : output(out) {}

    /**
     * @brief Write a single bit to the output.
     * @param bit The bit to write (0 or 1).
     */
    void write_bit(uint8_t bit) {
        buffer |= (bit & 1) << bits_in_buffer;
        bits_in_buffer++;
        if (bits_in_buffer == 8) {
            output.push_back(static_cast<uint8_t>(buffer));
            buffer = 0;
            bits_in_buffer = 0;
        }
    }

    /**
     * @brief Write multiple bits to the output, LSB first.
     * @param value The value to write.
     * @param count Number of bits to write from the value.
     */
    void write_bits(uint32_t value, uint8_t count) {
        for (uint8_t i = 0; i < count; ++i) {
            write_bit((value >> i) & 1);
        }
    }

    /**
     * @brief Flush any remaining bits to the output as a byte.
     */
    void flush() {
        if (bits_in_buffer > 0) {
            output.push_back(static_cast<uint8_t>(buffer));
            buffer = 0;
            bits_in_buffer = 0;
        }
    }
};

/**
 * @brief Class to read bits from a byte vector, LSB first.
 */
class BitReader {
private:
    const std::vector<uint8_t>& input; ///< Reference to the input byte vector
    size_t byte_pos = 0;               ///< Current byte position in the input
    uint8_t bit_pos = 0;               ///< Current bit position in the current byte
    uint8_t current_byte = 0;          ///< Current byte being read

public:
    explicit BitReader(const std::vector<uint8_t>& in) : input(in) {
        if (!input.empty()) current_byte = input[0];
    }

    /**
     * @brief Read a single bit from the input.
     * @return The bit read (0 or 1).
     * @throws std::runtime_error if reading past the end of input.
     */
    uint8_t read_bit() {
        if (byte_pos >= input.size()) throw std::runtime_error("Read past end");
        uint8_t bit = (current_byte >> bit_pos) & 1;
        bit_pos++;
        if (bit_pos == 8) {
            bit_pos = 0;
            byte_pos++;
            if (byte_pos < input.size()) current_byte = input[byte_pos];
        }
        return bit;
    }

    /**
     * @brief Read multiple bits from the input, LSB first.
     * @param count Number of bits to read.
     * @return The value read.
     * @throws std::runtime_error if reading past the end of input.
     */
    uint32_t read_bits(uint8_t count) {
        uint32_t value = 0;
        for (uint8_t i = 0; i < count; ++i) {
            value |= read_bit() << i;
        }
        return value;
    }
};

/**
 * @brief Read the next literal/length symbol from the bitstream using fixed Huffman codes.
 * @param reader The BitReader to read from.
 * @return The symbol (0-255 for literals, 256 for EOB, 257-285 for lengths).
 * @throws std::runtime_error if an invalid code is encountered.
 */
inline uint16_t read_literal_length_symbol(BitReader& reader) {
    uint32_t code = 0;
    uint8_t len = 0;
    while (true) {
        if (len >= 9) throw std::runtime_error("Invalid code");
        code |= reader.read_bit() << len;
        len++;
        if (len == 7) {
            if (code == 0) return 256;                  // End-of-block
            if (code >= 1 && code <= 23) return 257 + (code - 1); // Lengths 257-279
        } else if (len == 8) {
            if (code >= 48 && code <= 191) return code - 48;      // Literals 0-143
            if (code >= 192 && code <= 199) return 280 + (code - 192); // Lengths 280-285
        } else if (len == 9) {
            if (code >= 400 && code <= 511) return 144 + (code - 400); // Literals 144-255
        }
    }
}

/**
 * @brief Compress data into a raw deflate stream using fixed Huffman codes.
 * @param input The data to compress.
 * @return The compressed data as a byte vector.
 */
inline std::vector<uint8_t> compress(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    BitWriter writer(output);

    // Write block header: BFINAL=1, BTYPE=01 (fixed Huffman)
    writer.write_bit(1);  // BFINAL
    writer.write_bits(1, 2); // BTYPE=01

    const size_t WINDOW_SIZE = 32768; // Maximum window size per deflate spec
    std::vector<uint8_t> window;
    window.reserve(WINDOW_SIZE);
    size_t pos = 0;

    while (pos < input.size()) {
        size_t match_length = 0;
        size_t match_distance = 0;

        // Look for a match in the sliding window
        if (!window.empty()) {
            size_t window_start = (window.size() > WINDOW_SIZE) ? window.size() - WINDOW_SIZE : 0;
            for (size_t i = window_start; i < window.size(); ++i) {
                size_t len = 0;
                while (i + len < window.size() && pos + len < input.size() &&
                       window[i + len] == input[pos + len] && len < 258) {
                    len++;
                }
                if (len >= 3 && len > match_length) {
                    match_length = len;
                    match_distance = window.size() - i;
                }
            }
        }

        if (match_length >= 3) {
            // Encode length-distance pair
            auto [code, extra] = find_length_code(match_length);
            auto [huff_code, huff_len] = get_literal_length_code(code);
            writer.write_bits(huff_code, huff_len);
            uint8_t extra_bits = length_codes[code - 257].extra_bits;
            if (extra_bits > 0) writer.write_bits(extra, extra_bits);

            auto [dist_code, dist_extra] = find_distance_code(match_distance);
            auto [dist_huff_code, dist_huff_len] = get_distance_code(dist_code);
            writer.write_bits(dist_huff_code, dist_huff_len);
            uint8_t dist_extra_bits = distance_codes[dist_code].extra_bits;
            if (dist_extra_bits > 0) writer.write_bits(dist_extra, dist_extra_bits);

            // Update window
            for (size_t i = 0; i < match_length; ++i) {
                window.push_back(input[pos + i]);
                if (window.size() > WINDOW_SIZE) window.erase(window.begin());
            }
            pos += match_length;
        } else {
            // Encode literal byte
            uint8_t byte = input[pos];
            auto [huff_code, huff_len] = get_literal_length_code(byte);
            writer.write_bits(huff_code, huff_len);
            window.push_back(byte);
            if (window.size() > WINDOW_SIZE) window.erase(window.begin());
            pos++;
        }
    }

    // Write end-of-block symbol
    auto [eob_code, eob_len] = get_literal_length_code(256);
    writer.write_bits(eob_code, eob_len);
    writer.flush();

    return output;
}

/**
 * @brief Decompress a raw deflate stream using fixed Huffman codes.
 * @param compressed The compressed data.
 * @return The decompressed data as a byte vector.
 * @throws std::runtime_error if the stream is invalid or uses unsupported compression.
 */
inline std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed) {
    BitReader reader(compressed);
    std::vector<uint8_t> output;

    // Read block header
    bool bfinal = reader.read_bit(); // BFINAL bit
    uint8_t btype = reader.read_bits(2); // BTYPE
    if (btype != 1) throw std::runtime_error("Only fixed Huffman supported");

    while (true) {
        uint16_t symbol = read_literal_length_symbol(reader);
        if (symbol < 256) {
            // Literal byte
            output.push_back(static_cast<uint8_t>(symbol));
        } else if (symbol == 256) {
            // End of block
            break;
        } else {
            // Length-distance pair
            size_t idx = symbol - 257;
            uint16_t length = length_codes[idx].base_value;
            uint8_t extra_bits = length_codes[idx].extra_bits;
            if (extra_bits > 0) length += reader.read_bits(extra_bits);

            uint16_t dist_code = reader.read_bits(5);
            uint32_t distance = distance_codes[dist_code].base_value;
            uint8_t dist_extra_bits = distance_codes[dist_code].extra_bits;
            if (dist_extra_bits > 0) distance += reader.read_bits(dist_extra_bits);

            // Copy bytes from the output buffer
            size_t start = output.size() - distance;
            for (uint16_t i = 0; i < length; ++i) {
                output.push_back(output[start + i]);
            }
        }
    }

    return output;
}

} // namespace deflate

#endif // DEFLATE_HPP