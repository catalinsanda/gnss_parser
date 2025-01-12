
#include "GNSSParser.h"

uint32_t GNSSParser::CRC24Q_TABLE[256];

GNSSParser::GNSSParser()
{
    static bool initialized = false;
    if (!initialized)
    {
        generateCRC24QTable(CRC24Q_TABLE);
        initialized = true;
    }
}

void GNSSParser::generateCRC24QTable(uint32_t table[256])
{
    const uint32_t polynomial = 0x1864CFB;

    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t crc = i << 16;

        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x800000)
            {
                crc = (crc << 1) ^ polynomial;
            }
            else
            {
                crc = crc << 1;
            }
        }

        table[i] = crc & 0xFFFFFF;
    }
}

uint32_t GNSSParser::calculateRTCM3CRC(const uint8_t *data, size_t length)
{
    uint32_t crc = 0;

    for (size_t i = 0; i < length; i++)
    {
        uint8_t index = ((crc >> 16) ^ data[i]) & 0xFF;
        crc = ((crc << 8) & 0xFFFFFF) ^ CRC24Q_TABLE[index];
    }

    return crc & 0xFFFFFF;
}

void GNSSParser::addMessageToQueue(Message::Type type, size_t start, size_t length,
                                   bool valid, const char *error)
{
    if (message_queue_.size() >= MAX_MESSAGES)
    {
        StoredMessage &oldest = message_queue_.front();
        size_t bytes_to_remove = oldest.length;
        read_pos_ = (read_pos_ + bytes_to_remove) % BUFFER_SIZE;
        bytes_available_ -= bytes_to_remove;
        message_queue_.pop();
    }

    message_queue_.push({type, start, length, valid, error});
}

bool GNSSParser::tryParseRTCM3(size_t start_pos, size_t available_bytes, ParseResult &result)
{
    if (available_bytes < 6)
    {
        result = {false, false, 0, "Insufficient bytes"};
        return false;
    }

    if (buffer_[start_pos] != 0xD3)
    {
        return false;
    }

    uint16_t msg_length = ((buffer_[(start_pos + 1) % BUFFER_SIZE] & 0x0F) << 8) |
                          buffer_[(start_pos + 2) % BUFFER_SIZE];

    if (msg_length > 1023)
    {
        result = {false, true, 0, "Invalid length"};
        return true;
    }

    size_t total_length = msg_length + 6; // header(3) + payload + crc(3)

    if (available_bytes < total_length)
    {
        result = {false, false, total_length, "Message incomplete"};
        return true;
    }

    bool is_valid = validateRTCM3Message(start_pos, total_length);
    result = {is_valid, true, total_length, is_valid ? nullptr : "CRC validation failed"};
    return true;
}

bool GNSSParser::tryParseNMEA(size_t start_pos, size_t available_bytes, ParseResult &result)
{
    if (buffer_[start_pos] != '$' && buffer_[start_pos] != '!')
    {
        return false;
    }

    // Look for end of message within max NMEA length
    // Spec says it should be 82, but I see valid NMEA sentences which are longer.
    const size_t max_nmea_length = 96;

    if (available_bytes > max_nmea_length)
    {
        result = {false, true, 0, "Message too long to be a NMEA sentence"};
        return true;
    }

    size_t search_length = std::min(available_bytes, max_nmea_length);
    size_t msg_length = 0;
    bool found_end = false;

    for (size_t i = 0; i < search_length; i++)
    {
        if (buffer_[(start_pos + i) % BUFFER_SIZE] == '\n')
        {
            msg_length = i + 1;
            found_end = true;
            break;
        }
    }

    if (!found_end)
    {
        result = {false, false, 0, "No message end found"};
        return true;
    }

    bool is_valid = validateNMEAMessage(start_pos, msg_length);
    result = {is_valid, true, msg_length, is_valid ? nullptr : "Checksum validation failed"};
    return true;
}

void GNSSParser::scanBuffer()
{
    size_t scan_pos = read_pos_;
    size_t remaining_bytes = bytes_available_;

    while (remaining_bytes >= 6)
    {
        ParseResult result;
        bool message_start_found = false;

        if (tryParseRTCM3(scan_pos, remaining_bytes, result))
        {
            message_start_found = true;
            if (result.valid)
            {
                addMessageToQueue(Message::Type::RTCM3, scan_pos, result.length, true, result.error);
                scan_pos = (scan_pos + result.length) % BUFFER_SIZE;
                remaining_bytes -= result.length;
                continue;
            }
            else if (!result.complete)
            {
                break;
            }
        }

        if (!message_start_found && tryParseNMEA(scan_pos, remaining_bytes, result))
        {
            message_start_found = true;
            if (result.valid)
            {
                addMessageToQueue(Message::Type::NMEA, scan_pos, result.length, true, result.error);
                scan_pos = (scan_pos + result.length) % BUFFER_SIZE;
                remaining_bytes -= result.length;
                continue;
            }
            else if (!result.complete)
            {
                break;
            }
        }

        scan_pos = (scan_pos + 1) % BUFFER_SIZE;
        remaining_bytes--;
    }

    read_pos_ = scan_pos;
    bytes_available_ = remaining_bytes;
}

bool GNSSParser::encode(uint8_t byte)
{
    buffer_[write_pos_] = byte;
    write_pos_ = (write_pos_ + 1) % BUFFER_SIZE;
    bytes_available_++;

    if (bytes_available_ > BUFFER_SIZE)
    {
        read_pos_ = (read_pos_ + 1) % BUFFER_SIZE;
        bytes_available_ = BUFFER_SIZE;
    }

    scanBuffer();

    return !message_queue_.empty();
}

bool GNSSParser::available() const
{
    return !message_queue_.empty();
}

GNSSParser::Message GNSSParser::getMessage()
{
    if (message_queue_.empty())
    {
        return {GNSSParser::Message::Type::UNKNOWN, nullptr, 0, false, "No message available"};
    }

    StoredMessage msg = message_queue_.front();
    message_queue_.pop();

    static uint8_t msg_buffer[BUFFER_SIZE];

    for (size_t i = 0; i < msg.length; i++)
    {
        size_t pos = (msg.start + i) % BUFFER_SIZE;
        msg_buffer[i] = buffer_[pos];
    }

    return {msg.type, msg_buffer, msg.length, msg.valid, msg.error};
}

void GNSSParser::clear()
{
    while (!message_queue_.empty())
    {
        message_queue_.pop();
    }
    write_pos_ = 0;
    read_pos_ = 0;
    bytes_available_ = 0;
}

bool GNSSParser::validateRTCM3Message(size_t start, size_t length)
{
    if (length < 6)
        return false;

    uint16_t msg_length = ((buffer_[(start + 1) % BUFFER_SIZE] & 0x03) << 8) |
                          buffer_[(start + 2) % BUFFER_SIZE];

    uint8_t msg_data[1029]; // Max RTCM message size (1024) + header (3) + CRC (3)

    for (size_t i = 0; i < length - 3; i++)
    { // Exclude CRC bytes
        msg_data[i] = buffer_[(start + i) % BUFFER_SIZE];
    }

    uint32_t calculated_crc = calculateRTCM3CRC(msg_data, length - 3);

    uint32_t received_crc = (static_cast<uint32_t>(buffer_[(start + length - 1) % BUFFER_SIZE])) |
                            (static_cast<uint32_t>(buffer_[(start + length - 2) % BUFFER_SIZE]) << 8) |
                            (static_cast<uint32_t>(buffer_[(start + length - 3) % BUFFER_SIZE]) << 16);

    return calculated_crc == received_crc;
}

uint8_t GNSSParser::calculateNMEAChecksum(size_t start, size_t length)
{
    uint8_t checksum = 0;
    bool started = false;

    for (size_t i = 0; i < length - 5; i++)
    { // Exclude checksum and CRLF
        size_t pos = (start + i) % BUFFER_SIZE;
        char c = buffer_[pos];

        if (c == '$' || c == '!')
        {
            started = true;
            continue;
        }

        if (c == '*')
            break;

        if (started)
        {
            checksum ^= c;
        }
    }

    return checksum;
}

bool GNSSParser::validateNMEAMessage(size_t start, size_t length)
{
    if (length < 7)
        return false; // Minimum valid NMEA message length

    // Find checksum position
    size_t asterisk_pos = 0;
    for (size_t i = 0; i < length; i++)
    {
        if (buffer_[(start + i) % BUFFER_SIZE] == '*')
        {
            asterisk_pos = i;
            break;
        }
    }

    if (asterisk_pos == 0 || asterisk_pos >= length - 3)
    {
        return false; // No checksum or invalid format
    }

    uint8_t calculated_checksum = calculateNMEAChecksum(start, length);

    char checksum_str[3];
    checksum_str[0] = buffer_[(start + asterisk_pos + 1) % BUFFER_SIZE];
    checksum_str[1] = buffer_[(start + asterisk_pos + 2) % BUFFER_SIZE];
    checksum_str[2] = '\0';

    uint8_t received_checksum;
    if (sscanf(checksum_str, "%hhx", &received_checksum) != 1)
    {
        return false; // Invalid checksum format
    }

    return calculated_checksum == received_checksum;
}
