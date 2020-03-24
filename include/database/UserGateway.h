#pragma once

#include "IGateway.h"
#include "user_details.h"
#include <vector>

namespace balancedbanana::database {
    class UserGateway : virtual public IGateway {
    public:

        /**
         * Adds a user to the database.
         * @param user  The user to be added.
         * @return True if the operation was successful, otherwise false.
         */
        static bool addUser(const user_details &user);

        /**
         * Deletes a user with the given id from the database.
         * @param id  The id of the user to be deleted.
         * @return True if the operation was successful, otherwise false.
         */
        static bool removeUser(uint64_t id);

        /**
         * Getter method for the information of a user with the given id.
         * @param id  The id of the user.
         * @return The details of the user.
         */
        static user_details getUser(uint64_t id);

        /**
         * Getter for all the users in the database.
         * @return  Vector of all the users in the database.
         */
        static std::vector<user_details> getUsers();

       /**
        * Getter for a user with a specific name.
        * @param name The name of the user.
        * @return Returns the correct details of the user if found, otherwise return empty details struct with
        * invalid id.
        */
        static user_details getUserByName(const std::string& name);

        /**
         * Updates the given user's fields in the database.
         * @param user The user.
         */
        static void updateUser(const user_details& user);
    };
}