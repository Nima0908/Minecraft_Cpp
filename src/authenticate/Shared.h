#pragma once
#include <iostream>
#include <chrono>
#include <atomic>

constexpr const char *CLIENT_ID = "757bb3b3-b7ca-4bcd-a160-c92e6379c263";
constexpr const char *SERVER_ADDRESS = "mc.hypixel.net";
constexpr const char *SERVER_IP = "172.65.254.166";
constexpr const char *SERVER_PORT_STR = "25565";
constexpr const int TICKS_PER_SECOND = 20;
constexpr int SERVER_PORT = 25565;
constexpr int PROTOCOL_VERSION = 770;
constexpr int LOGIN_STATE = 1;
constexpr const char *TOKEN_FILE = "tokens.json";
constexpr std::chrono::milliseconds TICK_DURATION(1000 / TICKS_PER_SECOND);
std::atomic<bool> should_stop_;

