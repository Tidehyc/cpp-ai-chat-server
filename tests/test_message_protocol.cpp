#include <gtest/gtest.h>
#include "protocol/message.h"

using namespace chat;

TEST(MessageProtocolTest, MessageTypeRoundTrip)
{
    EXPECT_EQ(message_type_from_string("text"), MessageType::TEXT);
    EXPECT_EQ(message_type_from_string("ai_response"), MessageType::AI_RESPONSE);
    EXPECT_EQ(message_type_from_string("unknown"), MessageType::TEXT);
    EXPECT_EQ(message_type_to_string(MessageType::PING), "ping");
}

TEST(MessageProtocolTest, SerializeDeserializeRoundTrip)
{
    Message msg;
    msg.type = "ai_request";
    msg.msg_type = MessageType::AI_REQUEST;
    msg.from = "alice";
    msg.to = "AI";
    msg.room = "general";
    msg.content = "hello";
    msg.is_read = true;
    msg.timestamp = 123456789;

    const std::string payload = serialize_message(msg);
    const Message parsed = deserialize_message(payload);

    EXPECT_EQ(parsed.type, "ai_request");
    EXPECT_EQ(parsed.msg_type, MessageType::AI_REQUEST);
    EXPECT_EQ(parsed.from, "alice");
    EXPECT_EQ(parsed.to, "AI");
    EXPECT_EQ(parsed.room, "general");
    EXPECT_EQ(parsed.content, "hello");
    EXPECT_TRUE(parsed.is_read);
    EXPECT_EQ(parsed.timestamp, 123456789);
}

TEST(MessageProtocolTest, SerializeUsesMessageTypeWhenTypeStringMissing)
{
    Message msg;
    msg.type.clear();
    msg.msg_type = MessageType::SYSTEM;
    msg.content = "maintenance";

    const std::string payload = serialize_message(msg);
    const Message parsed = deserialize_message(payload);

    EXPECT_EQ(parsed.type, "system");
    EXPECT_EQ(parsed.msg_type, MessageType::SYSTEM);
    EXPECT_EQ(parsed.content, "maintenance");
    EXPECT_FALSE(parsed.is_read);
    EXPECT_EQ(parsed.timestamp, 0);
}

TEST(MessageProtocolTest, DeserializeMissingFieldsFallsBackToDefaults)
{
    const Message parsed = deserialize_message(R"({"content":"hello"})");

    EXPECT_EQ(parsed.type, "text");
    EXPECT_EQ(parsed.msg_type, MessageType::TEXT);
    EXPECT_EQ(parsed.from, "");
    EXPECT_EQ(parsed.to, "");
    EXPECT_EQ(parsed.room, "");
    EXPECT_EQ(parsed.content, "hello");
    EXPECT_FALSE(parsed.is_read);
    EXPECT_EQ(parsed.timestamp, 0);
}

TEST(MessageProtocolTest, DeserializeUnknownTypeFallsBackToText)
{
    const Message parsed = deserialize_message(R"({"type":"custom_event","content":"ping"})");

    EXPECT_EQ(parsed.type, "custom_event");
    EXPECT_EQ(parsed.msg_type, MessageType::TEXT);
    EXPECT_EQ(parsed.content, "ping");
}

TEST(MessageProtocolTest, InvalidJsonShouldNotCrash)
{
    const Message parsed = deserialize_message("{invalid_json");
    EXPECT_EQ(parsed.type, "error");
    EXPECT_EQ(parsed.msg_type, MessageType::ERROR);
    EXPECT_EQ(parsed.content, "invalid json");
}
