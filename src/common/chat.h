#pragma once

enum class CustomMessageType : char
{
	Chat = 1,
	Whisper = 2
};

constexpr unsigned char CUSTOM_MESSAGE_INDEX_BIT = 0b10000000;
constexpr unsigned char CUSTOM_MESSAGE_INDEX_MASK = (unsigned char)~CUSTOM_MESSAGE_INDEX_BIT;
