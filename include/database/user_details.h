#pragma once

#include <cinttypes>

namespace balancedbanana {
    namespace database {

        // Encapsulates all details required to create a user object
        struct user_details {
            uint64_t id;

            std::string name;

            std::string email;

            std::string public_key;

        };
    }
}