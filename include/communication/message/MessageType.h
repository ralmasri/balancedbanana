#pragma once

enum MessageType : uint32_t {
    INVALID = 0,
    AUTH_RESULT = 0x534,
    CLIENT_AUTH = 0x223,
    HARDWARE_DETAIL = 3,
    PUBLIC_KEY_AUTH = 4,
    SNAPSHOT = 5,
    TASK = 6,
    WORKER_AUTH = 7

};
