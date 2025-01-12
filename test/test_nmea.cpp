#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "GNSSParser.h"

const char *ONE_VALID_NMEA_MESSAGE = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
const char *ONE_VALID_NMEA_MESSAGE_WITH_A_PREFIX = "$GPGGA$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
const char *ONE_VALID_ONE_NOT_NMEA_MESSAGES = "$GNGGA,140656.00,4430.39666339,N,02600.98986769,E,7,27,1.0,89.7706,M,35.4899,M,,*42\r\n"
                                              "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";

const char *BULK_NMEA_MESSAGES = "$GNGGA,140656.00,4430.39666339,N,02600.98986769,E,7,17,1.0,89.7706,M,35.4899,M,,*42\r\n"
                                 "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"
                                 "$GNGSA,M,3,24,28,29,31,32,,,,,,,,1.5,1.0,1.1,1*3C\r\n"
                                 "$GNGSA,M,3,67,77,86,87,,,,,,,,,1.5,1.0,1.1,2*3B\r\n"
                                 "$GNGSA,M,3,13,19,21,,,,,,,,,,1.5,1.0,1.1,3*33\r\n"
                                 "$GNGSA,M,3,23,25,28,37,43,,,,,,,,1.5,1.0,1.1,4*32\r\n"
                                 "$GNRMC,140656.00,A,4430.39666339,N,02600.98986769,E,0.006,221.9,070125,6.1,E,M,S*5C\r\n"
                                 "$GPGSV,2,1,06,24,18,168,40,25,72,321,28,28,29,311,35,29,48,242,49,1*67\r\n"
                                 "$GPGSV,2,2,06,32,14,266,25,31,06,321,30,1*64\r\n"
                                 "$GPGSV,2,1,06,24,18,168,35,25,72,321,30,28,29,311,39,29,48,242,46,4*6A\r\n"
                                 "$GPGSV,2,2,06,32,14,266,31,31,06,321,34,4*60\r\n"
                                 "$GPGSV,1,1,03,24,18,168,29,28,29,311,38,32,14,266,30,8*59\r\n"
                                 "$GLGSV,1,1,04,86,75,146,31,87,30,198,34,67,06,296,33,77,33,319,31,1*79\r\n"
                                 "$GLGSV,1,1,01,86,75,146,32,3*45\r\n"
                                 "$GBGSV,2,1,05,37,14,309,43,25,55,142,34,43,67,195,48,23,66,298,33,1*7E\r\n"
                                 "$GBGSV,2,2,05,28,07,217,40,1*4E\r\n"
                                 "$GBGSV,2,1,05,37,14,309,40,25,55,142,36,43,67,195,46,23,66,298,31,3*71\r\n"
                                 "$GBGSV,2,2,05,28,07,217,36,3*4D\r\n"
                                 "$GBGSV,2,1,05,37,14,309,31,25,55,142,31,43,67,195,36,23,66,298,34,5*74\r\n"
                                 "$GBGSV,2,2,05,28,07,217,35,5*48\r\n"
                                 "$GBGSV,1,1,03,37,14,309,38,43,67,195,40,23,66,298,34,6*48\r\n"
                                 "$GBGSV,2,1,05,37,14,309,33,25,55,142,33,43,67,195,40,23,66,298,35,8*79\r\n"
                                 "$GBGSV,2,2,05,28,07,217,36,8*46\r\n"
                                 "$GAGSV,2,1,07,19,18,247,33,21,71,323,30,04,16,305,36,26,10,185,37,1*78\r\n"
                                 "$GAGSV,2,2,07,29,02,207,35,27,46,092,26,13,47,142,33,1*46\r\n"
                                 "$GAGSV,2,1,07,19,18,247,34,21,71,323,24,04,16,305,38,26,10,185,37,2*77\r\n"
                                 "$GAGSV,2,2,07,29,02,207,39,27,46,092,25,13,47,142,33,2*4A\r\n"
                                 "$GAGSV,2,1,05,19,18,247,29,21,71,323,29,26,10,185,31,29,02,207,34,7*72\r\n"
                                 "$GAGSV,2,2,05,13,47,142,38,7*4B\r\n";

const char *INVALID_CHECKSUM = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*XX\r\n";

void setUp(void)
{
    // Called before each test
}

void tearDown(void)
{
    // Called after each test
}

void test_one_valid_nmea_message()
{
    GNSSParser parser;

    for (size_t i = 0; i < strlen(ONE_VALID_NMEA_MESSAGE); i++)
    {
        parser.encode(ONE_VALID_NMEA_MESSAGE[i]);
    }

    TEST_ASSERT_TRUE(parser.available());

    auto msg = parser.getMessage();
    TEST_ASSERT_EQUAL(GNSSParser::Message::Type::NMEA, msg.type);
    TEST_ASSERT_TRUE(msg.valid);
    TEST_ASSERT_EQUAL(strlen(ONE_VALID_NMEA_MESSAGE), msg.length);
}

void test_one_valid_nmea_message_with_prefix()
{
    GNSSParser parser;

    for (size_t i = 0; i < strlen(ONE_VALID_NMEA_MESSAGE_WITH_A_PREFIX); i++)
    {
        parser.encode(ONE_VALID_NMEA_MESSAGE_WITH_A_PREFIX[i]);
    }

    TEST_ASSERT_TRUE(parser.available());

    auto msg = parser.getMessage();
    TEST_ASSERT_EQUAL(GNSSParser::Message::Type::NMEA, msg.type);
    TEST_ASSERT_TRUE(msg.valid);
    TEST_ASSERT_EQUAL(strlen(ONE_VALID_NMEA_MESSAGE), msg.length);
}

void test_two_messages_one_valid_one_not()
{
    GNSSParser parser;

    auto message_counter = 0;

    for (size_t i = 0; i < strlen(ONE_VALID_ONE_NOT_NMEA_MESSAGES); i++)
    {
        parser.encode(ONE_VALID_ONE_NOT_NMEA_MESSAGES[i]);

        if (parser.available())
        {
            auto msg = parser.getMessage();

            char nmea_message[128];
            uint8_t length = std::min(sizeof(nmea_message) - 1, msg.length);
            strncpy(nmea_message, (char *)msg.data, length);
            nmea_message[length] = 0;

            TEST_ASSERT_EQUAL(GNSSParser::Message::Type::NMEA, msg.type);
            TEST_ASSERT_TRUE(msg.valid);

            printf("Found message %d: %s", message_counter, nmea_message);
            message_counter++;
        }
    }

    printf("Messages found %d\n", message_counter);

    TEST_ASSERT_EQUAL(1, message_counter);
}

void test_invalid_checksum()
{
    GNSSParser parser;

    for (size_t i = 0; i < strlen(INVALID_CHECKSUM); i++)
    {
        parser.encode(INVALID_CHECKSUM[i]);
        TEST_ASSERT_FALSE(parser.available());
    }
}

size_t countNewLines(const char *str, size_t maxLength)
{
    size_t count = 0;
    while (str && *str && maxLength--)
        count += (*str++ == '\n');
    return count;
}

void test_parse_nmea_sentences()
{
    GNSSParser parser;

    auto message_counter = 0;

    for (size_t i = 0; i < strlen(BULK_NMEA_MESSAGES); i++)
    {
        parser.encode(BULK_NMEA_MESSAGES[i]);

        if (parser.available())
        {
            auto msg = parser.getMessage();

            char nmea_message[128];
            uint8_t length = std::min(sizeof(nmea_message) - 1, msg.length);
            strncpy(nmea_message, (char *)msg.data, length);
            nmea_message[length] = 0;

            TEST_ASSERT_EQUAL(GNSSParser::Message::Type::NMEA, msg.type);
            TEST_ASSERT_TRUE(msg.valid);

            printf("Found message %d: %s", message_counter, nmea_message);
            message_counter++;
        }
    }

    printf("Messages found %d\n", message_counter);

    TEST_ASSERT_EQUAL(countNewLines(BULK_NMEA_MESSAGES, strlen(BULK_NMEA_MESSAGES) + 1), message_counter);
}

void test_parse_log_file(const char *filename, uint32_t expected_nmea_count, uint32_t expected_rtcm_count)
{
    printf("\nTesting file: %s\n", filename);
    printf("Expected NMEA messages: %u, Expected RTCM messages: %u\n",
           expected_nmea_count, expected_rtcm_count);

    FILE *file = fopen(filename, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(file, "Failed to open test file");

    GNSSParser parser;
    uint8_t buffer[1024];
    size_t bytes_read;

    uint32_t nmea_count = 0;
    uint32_t rtcm_count = 0;

    // Process file in chunks
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        for (size_t i = 0; i < bytes_read; i++)
        {
            parser.encode(buffer[i]);

            // Process any available messages
            while (parser.available())
            {
                auto msg = parser.getMessage();

                if (msg.type == GNSSParser::Message::Type::NMEA)
                {
                    // Print NMEA messages as ASCII
                    printf("NMEA [%s]: ", msg.valid ? "VALID" : "INVALID");
                    fwrite(msg.data, 1, msg.length, stdout);

                    if (!msg.valid && msg.error)
                    {
                        printf("Error: %s\n", msg.error);
                    }

                    if (msg.valid)
                        nmea_count++;
                }
                else if (msg.type == GNSSParser::Message::Type::RTCM3)
                {
                    // Print RTCM messages as hex
                    printf("RTCM3 [%s] Length: %zu Data: ",
                           msg.valid ? "VALID" : "INVALID",
                           msg.length);

                    for (size_t j = 0; j < msg.length; j++)
                    {
                        printf("%02X", msg.data[j]);
                    }
                    printf("\n");

                    if (!msg.valid && msg.error)
                    {
                        printf("Error: %s\n", msg.error);
                    }

                    if (msg.valid)
                        rtcm_count++;
                }
            }
        }
    }

    fclose(file);

    printf("Results:\n");
    printf("Found NMEA messages: %u (expected %u)\n", nmea_count, expected_nmea_count);
    printf("Found RTCM messages: %u (expected %u)\n", rtcm_count, expected_rtcm_count);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_nmea_count, nmea_count,
                                     "Incorrect number of valid NMEA messages");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_rtcm_count, rtcm_count,
                                     "Incorrect number of valid RTCM messages");
}

void test_random_data_parse()
{
    // Use fixed seed for reproducibility
    srand(12345);

    for (int test = 0; test < 1000; test++)
    {
        GNSSParser parser;
        uint8_t random_data[4096];

        srand(time(NULL));

        // Generate random data
        for (size_t i = 0; i < sizeof(random_data); i++)
        {
            random_data[i] = rand() & 0xFF;
        }

        // Feed data to parser
        size_t messages_found = 0;

        for (size_t i = 0; i < sizeof(random_data); i++)
        {
            parser.encode(random_data[i]);

            while (parser.available())
            {
                auto msg = parser.getMessage();
                if (msg.valid)
                {
                    messages_found++;

                    printf("WARNING: Found valid message in random data!\n");
                    printf("Test iteration: %d, Byte position: %zu\n", test, i);
                    printf("Message type: %d, Length: %zu\n", msg.type, msg.length);
                    printf("Message data: ");
                    for (size_t j = 0; j < msg.length; j++)
                    {
                        printf("%02X", msg.data[j]);
                    }
                    printf("\n");
                }
            }
        }

        TEST_ASSERT_EQUAL_MESSAGE(0, messages_found,
                                  "Parser incorrectly identified valid messages in random data");
    }

    printf("Successfully completed 1000 iterations of random data testing\n");
}

void test_parse_log_file_5_2()
{
    test_parse_log_file("test/test-data/test-data-5-2.bin", 5, 2);
}

void test_parse_log_file_56_4()
{
    test_parse_log_file("test/test-data/test-data-56-5.bin", 56, 4);
}

void test_parse_log_file_654_43()
{
    test_parse_log_file("test/test-data/test-data-654-43.bin", 654, 43);
}

void test_parse_log_file_32477_2193()
{
    test_parse_log_file("test/test-data/test-data-32477-2193.bin", 32477, 2193);
}

void process()
{
    UNITY_BEGIN();
    RUN_TEST(test_one_valid_nmea_message);
    RUN_TEST(test_one_valid_nmea_message_with_prefix);
    RUN_TEST(test_two_messages_one_valid_one_not);
    RUN_TEST(test_invalid_checksum);
    RUN_TEST(test_parse_nmea_sentences);
    RUN_TEST(test_parse_log_file_5_2);
    RUN_TEST(test_parse_log_file_56_4);
    RUN_TEST(test_parse_log_file_654_43);
    RUN_TEST(test_parse_log_file_32477_2193);
    RUN_TEST(test_random_data_parse);
    UNITY_END();
}

#ifdef ARDUINO
void setup()
{
    delay(2000);
    process();
}

void loop()
{
    delay(1000);
}
#else

int main(int argc, char **argv)
{
    process();
    return 0;
}
#endif