// Modules/Utils/Utils.h
#ifndef MODULES_UTILS_UTILS_H
#define MODULES_UTILS_UTILS_H

#include <string>       // For std::string
#include <random>       // For std::random_device, std::mt19937
#include <sstream>      // For std::stringstream
#include <iomanip>      // For std::hex, std::setw, std::setfill

// Rút gọn includes
// No specific project-internal includes needed for these basic utilities.

namespace ERP {
    namespace Utils {

        /**
         * @brief The Utils class provides general utility functions.
         */
        class Utils {
        public:
            /**
             * @brief Generates a universally unique identifier (UUID).
             * This implementation creates a UUID-like string (version 4) for use as IDs.
             * @return A string representation of a UUID.
             */
            static std::string generateUUID();

        private:
            // Private constructor/destructor to make it a purely static utility class
            Utils() = delete;
            ~Utils() = delete;
        };

    } // namespace Utils
} // namespace ERP

#endif // MODULES_UTILS_UTILS_H