#pragma once
#include "Specs.h"

namespace balancedbanana {
    namespace database {

        //Encapsulates all details required to create Worker object.
        struct worker_details {
            //The id of the Worker.
            uint64_t id;

            std::string name;

            //The specs of the worker.
            Specs specs;

            std::string address;

            std::string public_key;

        };
    }
}